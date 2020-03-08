#include "CellServer.h"

CellServer::CellServer(SOCKET sock, INetEvent* inet_event)
{
	svr_sock_ = sock;
	inet_event_ = inet_event;
	work_thread_ = nullptr;
}

CellServer::~CellServer()
{
	delete work_thread_;
	Close();
	svr_sock_ = INVALID_SOCKET;
}

void CellServer::Start()
{
	work_thread_ = new std::thread(std::mem_fn(&CellServer::OnRun), this);
}

void CellServer::Close()
{
	if (IsRun())
	{
#ifdef _WIN32
		// �رտͻ��� socket
		for (const auto& client : client_vec_)
		{
			if (client)
			{
				SOCKET sock = client->GetSock();
				closesocket(sock);
				delete client;
			}
		}
		client_vec_.clear();

		// �رշ���� socket
		closesocket(svr_sock_);
#else
		// �رտͻ��� socket
		for (const auto& client : client_vec_)
		{
			if (client)
			{
				int sock = client->GetSock();
				close(sock);
				delete client;
			}
		}
		client_vec_.clear();

		// �رշ���� socket
		close(svr_sock_);
#endif // _WIN32
	}
}

int CellServer::GetClientCount() const
{
	return client_vec_.size() + client_buff_vec_.size();
}

bool CellServer::OnRun()
{
	while (IsRun())
	{
		if (!client_buff_vec_.empty())
		{
			std::lock_guard<std::mutex> lock(client_mutex_);
			for (const auto& client : client_buff_vec_)
			{
				client_vec_.push_back(client);
			}
			client_buff_vec_.clear();
		}

		// ���û����Ҫ����Ŀͻ��ˣ�����
		if (client_vec_.empty())
		{
			// ����һ����
			std::chrono::milliseconds sleep_time(1);
			std::this_thread::sleep_for(sleep_time);
			continue;
		}

		// ������socket����
		fd_set fd_read;
		fd_set fd_write;
		fd_set fd_exp;

		// ��ռ���
		FD_ZERO(&fd_read);
		FD_ZERO(&fd_write);
		FD_ZERO(&fd_exp);

		// �������socket
		SOCKET max_sock = client_vec_[0]->GetSock();
		for (const auto& client : client_vec_)
		{
			SOCKET sock = client->GetSock();
			FD_SET(sock, &fd_read);
			if (max_sock < sock)
			{
				max_sock = sock;
			}
		}

		// nfds ��һ��int����ָfd_set����������socket�ķ�Χ�����ֵ��1����������������
		// windows �п��Դ�0
		timeval time_val = { 0, 0 };
		int ret = select(int(max_sock) + 1, &fd_read, &fd_write, &fd_exp, nullptr );
		if (ret < 0)
		{
			printf("select < 0, �������\n");
			Close();
			return false;
		}

		std::vector<Client*> client_vec_temp = {};

		for (const auto& client : client_vec_)
		{
			auto temp_sock = client->GetSock();
			if (FD_ISSET(temp_sock, &fd_read))
			{
				// ˵���ͻ����˳���
				if (RecvData(client) < 0)
				{
					auto iter = std::find(client_vec_.begin(), client_vec_.end(), client);
					if (iter != client_vec_.end())
					{
						client_vec_temp.push_back(client);
						inet_event_->OnLeave(client);
						client_vec_.erase(iter);
					}
				}
			}
		}

		for (const auto* client : client_vec_temp)
		{
			if (client)
			{
				delete client;
			}
		}
		client_vec_temp.clear();
	}

	return false;
}

bool CellServer::IsRun()
{
	return (svr_sock_ != INVALID_SOCKET);
}

int CellServer::RecvData(Client* client)
{
	int len = recv(client->GetSock(), data_buffer_, sizeof(DataHeader), 0);
	if (len <= 0)
	{
		printf("�ͻ������˳����������\n");
		return -1;
	}

	// ���յ������ݿ�������Ϣ������
	memcpy(client->GetDataBuffer() + client->GetLastPos(), data_buffer_, len);

	// ������������β������
	client->SetLastPos(client->GetLastPos() + len);

	// �ж���Ϣ�����������ݳ����Ƕ�������ϢͷDataHeader�ĳ���
	while (client->GetLastPos() >= sizeof(DataHeader))
	{
		// ��ʱ�Ϳ���֪����ǰ��Ϣ�ĳ���
		DataHeader* header = (DataHeader*)client->GetDataBuffer();

		// �ж���Ϣ�����������ݳ����Ƿ����һ��������Ϣ����
		if (client->GetLastPos() >= header->data_len)
		{
			// δ�������Ϣ�ĳ���
			int size = client->GetLastPos() - header->data_len;

			// ������Ϣ
			OnNetMsg(client, header);

			// ����Ϣ��������δ���������ǰ��
			memcpy(client->GetDataBuffer(), client->GetDataBuffer() + header->data_len, size);

			// ����Ϣ����������β��λ��ǰ��
			client->SetLastPos(size);
		}
		else
		{
			// �����������ݳ��Ȳ���һ��������Ϣ����
			break;
		}
	}

	return 0;
}

int CellServer::OnNetMsg(Client* client, DataHeader* header)
{
	inet_event_->OnNetMsg(client, header);

	switch (header->cmd)
	{
	case Cmd::kCmdLogin:
	{
		LoginData* login_data = (LoginData*)header;
		//printf("�յ����cmd : kCmdLogin, ���ݳ��� ��%d, user_name : %s, password : %s\n", login_data->data_len, login_data->user_name, login_data->password);

		// �����ж��˺������߼�
		LoginRetData login_ret_data = {};
		(void)client->SendData(&login_ret_data);
	}
	break;

	case Cmd::kCmdLogout:
	{
		LogoutData* logout_data = (LogoutData*)header;
		//printf("�յ����cmd : kCmdLogout, ���ݳ��� ��%d, user_name : %s\n", logout_data->data_len, logout_data->user_name);

		// �����ж��˺������߼�
		LogoutRetData logout_ret_data = {};
		(void)client->SendData(&logout_ret_data);
	}
	break;

	default:
	{
		printf("�յ��������cmd : %d\n", header->cmd);

		DataHeader temp = {};
		temp.cmd = Cmd::kCmdError;
		temp.data_len = 0;
		(void)client->SendData(&temp);
	}
	break;
	}

	return 0;
}

int CellServer::SendData(DataHeader* data, SOCKET client_sock)
{
	if (IsRun() && data)
		return send(client_sock, (const char*)data, data->data_len, 0);

	return SOCKET_ERROR;
}

void CellServer::SendData(DataHeader* data)
{
	if (IsRun() && data)
	{
		for (const auto& client : client_vec_)
		{
			send(client->GetSock(), (const char*)data, data->data_len, 0);
		}
	}
}

void CellServer::AddClient(Client* client)
{
	if (client)
	{
		std::lock_guard<std::mutex> lock(client_mutex_);
		client_buff_vec_.push_back(client);
	}

	inet_event_->OnJoin(client);
}
