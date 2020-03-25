#include "CellServer.h"

CellServer::CellServer(SOCKET sock, INetEvent* inet_event)
	:svr_sock_(sock),
	inet_event_(inet_event),
	work_thread_(nullptr),
	is_clients_change_(false),
	max_sock_(0),
	cell_task_svr_(nullptr)
{
	FD_ZERO(&fd_read_backup_);
}

CellServer::~CellServer()
{
	if (work_thread_)
	{
		delete work_thread_;
		work_thread_ = nullptr;
	}
	if (cell_task_svr_)
	{
		delete cell_task_svr_;
		cell_task_svr_ = nullptr;
	}

	Close();
	svr_sock_ = INVALID_SOCKET;
}

void CellServer::Start()
{
	work_thread_ = new std::thread(std::mem_fn(&CellServer::OnRun), this);

	cell_task_svr_ = new CellTaskServer();
	if (cell_task_svr_)
	{
		cell_task_svr_->Start();
	}
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
		int ret = select(int(max_sock_) + 1, &fd_read, nullptr, nullptr, nullptr);
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

		std::vector<std::shared_ptr<Client>> client_vec_temp = {};

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

		for (const auto& client : client_vec_temp)
		{
			if (client)
			{
				client_map_.erase(client->GetSock());
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

int CellServer::RecvData(std::shared_ptr<Client> client)
{
	char* recv_data_buffer = client->GetRecvDataBuffer() + client->GetRecvLastPos();
	int len = recv(client->GetSock(), recv_data_buffer, kRecvBufferSize - client->GetRecvLastPos(), 0);
	inet_event_->OnNetRecv(client);
	if (len <= 0)
	{
		printf("�ͻ������˳����������\n");
		return -1;
	}

	// ���յ������ݿ�������Ϣ������
	//memcpy(client->GetRecvDataBuffer() + client->GetRecvLastPos(), recv_data_buffer, len);

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
			OnNetMsg(this, client, header);

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

int CellServer::OnNetMsg(CellServer* cell_svr, std::shared_ptr<Client> client, DataHeader* header)
{
	inet_event_->OnNetMsg(this, client, header);

	switch (header->cmd)
	{
	case Cmd::kCmdLogin:
	{
		LoginData* login_data = (LoginData*)header;
		//printf("�յ����cmd : kCmdLogin, ���ݳ��� ��%d, user_name : %s, password : %s\n", login_data->data_len, login_data->user_name, login_data->password);

		// �����ж��˺������߼�
		//LoginRetData login_ret_data = {};
		//(void)client->SendData(&login_ret_data);

		LoginRetData* login_ret_data = new LoginRetData();
		cell_svr->AddSendTask(client, login_ret_data);
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

void CellServer::AddClient(std::shared_ptr<Client> client)
{
	if (client)
	{
		std::lock_guard<std::mutex> lock(client_mutex_);
		client_buff_vec_.push_back(client);
	}

	inet_event_->OnJoin(client);
}

void CellServer::AddSendTask(std::shared_ptr<Client> client, DataHeader* data)
{
	auto task = std::make_shared<CellSendMsg2ClientTask>(client, data);
	cell_task_svr_->AddTask((std::shared_ptr<CellTask>&)task);
}

CellSendMsg2ClientTask::CellSendMsg2ClientTask(std::shared_ptr<Client> client, DataHeader* data)
{
	client_ = client;
	data_ = data;
}

CellSendMsg2ClientTask::~CellSendMsg2ClientTask()
{
}

void CellSendMsg2ClientTask::DoTask()
{
	if (data_)
	{
		client_->SendData(data_);
		delete data_;
		data_ = nullptr;
	}
}
