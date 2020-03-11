#include "CellServer.h"

CellServer::CellServer(SOCKET sock, INetEvent* inet_event)
	:svr_sock_(sock), inet_event_(inet_event), work_thread_(nullptr), is_clients_change_(false), max_sock_(0)
{
	FD_ZERO(&fd_read_backup_);
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
		// �رտͻ��� socket
		for (const auto& client_pair : client_map_)
		{
			auto client = client_pair.second;
			if (client)
			{
				SOCKET sock = client->GetSock();
				closesocket(sock);
				delete client;
			}
		}
		client_map_.clear();

#ifdef _WIN32
		// �رշ���� socket
		closesocket(svr_sock_);
#else
		// �رշ���� socket
		close(svr_sock_);
#endif // _WIN32
	}
}

int CellServer::GetClientCount() const
{
	return client_map_.size() + client_buff_vec_.size();
}

bool CellServer::OnRun()
{
	is_clients_change_ = true;
	while (IsRun())
	{
		if (!client_buff_vec_.empty())
		{
			std::lock_guard<std::mutex> lock(client_mutex_);
			for (const auto& client : client_buff_vec_)
			{
				client_map_[client->GetSock()] = client;
			}
			client_buff_vec_.clear();
			is_clients_change_ = true;
		}

		// ���û����Ҫ����Ŀͻ��ˣ�����
		if (client_map_.empty())
		{
			// ����һ����
			std::chrono::milliseconds sleep_time(1);
			std::this_thread::sleep_for(sleep_time);
			continue;
		}

		// ������socket����
		fd_set fd_read;

		// ��ռ���
		FD_ZERO(&fd_read);

		if (is_clients_change_)
		{
			is_clients_change_ = false;

			// �������socket
			max_sock_ = client_map_.begin()->second->GetSock();
			for (const auto& client_pair : client_map_)
			{
				auto client = client_pair.second;
				SOCKET sock = client->GetSock();
				FD_SET(sock, &fd_read);
				if (max_sock_ < sock)
				{
					max_sock_ = sock;
				}
			}
			memcpy(&fd_read_backup_, &fd_read, sizeof(fd_set));
		}
		else
		{
			memcpy(&fd_read, &fd_read_backup_, sizeof(fd_set));
		}

		// nfds ��һ��int����ָfd_set����������socket�ķ�Χ�����ֵ��1����������������
		// windows �п��Դ�0
		timeval time_val = { 0, 0 };
		int ret = select(int(max_sock_) + 1, &fd_read, nullptr, nullptr, &time_val);
		if (ret < 0)
		{
			printf("select < 0, �������\n");
			Close();
			return false;
		}
		else if(ret == 0)
		{
			continue;
		}

		std::vector<Client*> client_vec_temp = {};

		for (int i = 0; i < int(fd_read.fd_count); i++)
		{
			auto iter = client_map_.find(fd_read.fd_array[i]);
			if (iter != client_map_.end())
			{
				auto client = iter->second;
				// ˵���ͻ����˳���
				if (RecvData(client) < 0)
				{
					is_clients_change_ = true;
					client_vec_temp.push_back(client);
					inet_event_->OnLeave(client);
				}
			}
			else
			{
				printf("error. if (iter != _clients.end())\n");
			}
		}

		for (const auto* client : client_vec_temp)
		{
			if (client)
			{
				client_map_.erase(client->GetSock());
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
	int len = recv(client->GetSock(), recv_data_buffer_, kRecvBufferSize, 0);
	inet_event_->OnNetRecv(client);
	if (len <= 0)
	{
		printf("�ͻ������˳����������\n");
		return -1;
	}

	// ���յ������ݿ�������Ϣ������
	memcpy(client->GetRecvDataBuffer() + client->GetRecvLastPos(), recv_data_buffer_, len);

	// ������������β������
	client->SetRecvLastPos(client->GetRecvLastPos() + len);

	// �ж���Ϣ�����������ݳ����Ƕ�������ϢͷDataHeader�ĳ���
	while (client->GetRecvLastPos() >= sizeof(DataHeader))
	{
		// ��ʱ�Ϳ���֪����ǰ��Ϣ�ĳ���
		DataHeader* header = (DataHeader*)client->GetRecvDataBuffer();

		// �ж���Ϣ�����������ݳ����Ƿ����һ��������Ϣ����
		if (client->GetRecvLastPos() >= header->data_len)
		{
			// δ�������Ϣ�ĳ���
			int size = client->GetRecvLastPos() - header->data_len;

			// ������Ϣ
			OnNetMsg(client, header);

			// ����Ϣ��������δ���������ǰ��
			memcpy(client->GetRecvDataBuffer(), client->GetRecvDataBuffer() + header->data_len, size);

			// ����Ϣ����������β��λ��ǰ��
			client->SetRecvLastPos(size);
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
	{
		return send(client_sock, (const char*)data, data->data_len, 0);
	}

	return SOCKET_ERROR;
}

void CellServer::SendData(DataHeader* data)
{
	if (IsRun() && data)
	{
		for (const auto& client_pair : client_map_)
		{
			auto client = client_pair.second;
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
