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
		// 关闭客户端 socket
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
		// 关闭服务端 socket
		closesocket(svr_sock_);
#else
		// 关闭服务端 socket
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

		// 如果没有需要处理的客户端，跳过
		if (client_map_.empty())
		{
			// 休眠一毫秒
			std::chrono::milliseconds sleep_time(1);
			std::this_thread::sleep_for(sleep_time);
			continue;
		}

		// 伯克利socket集合
		fd_set fd_read;

		// 清空集合
		FD_ZERO(&fd_read);

		if (is_clients_change_)
		{
			is_clients_change_ = false;

			// 计算最大socket
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

		// nfds 是一个int，是指fd_set集合中所有socket的范围（最大值加1），而不是数量。
		// windows 中可以传0
		timeval time_val = { 0, 0 };
		int ret = select(int(max_sock_) + 1, &fd_read, nullptr, nullptr, &time_val);
		if (ret < 0)
		{
			printf("select < 0, 任务结束\n");
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
				// 说明客户端退出了
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
		printf("客户端已退出，任务结束\n");
		return -1;
	}

	// 将收到的数据拷贝到消息缓冲区
	memcpy(client->GetRecvDataBuffer() + client->GetRecvLastPos(), recv_data_buffer_, len);

	// 缓冲区的数据尾部后移
	client->SetRecvLastPos(client->GetRecvLastPos() + len);

	// 判断消息缓冲区的数据长度是都大于消息头DataHeader的长度
	while (client->GetRecvLastPos() >= sizeof(DataHeader))
	{
		// 这时就可以知道当前消息的长度
		DataHeader* header = (DataHeader*)client->GetRecvDataBuffer();

		// 判断消息缓冲区的数据长度是否大于一条完整消息长度
		if (client->GetRecvLastPos() >= header->data_len)
		{
			// 未处理的消息的长度
			int size = client->GetRecvLastPos() - header->data_len;

			// 处理消息
			OnNetMsg(client, header);

			// 将消息缓冲区的未处理的数据前移
			memcpy(client->GetRecvDataBuffer(), client->GetRecvDataBuffer() + header->data_len, size);

			// 将消息缓冲区数据尾部位置前移
			client->SetRecvLastPos(size);
		}
		else
		{
			// 缓冲区的数据长度不足一条完整消息长度
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
		//printf("收到命令，cmd : kCmdLogin, 数据长度 ：%d, user_name : %s, password : %s\n", login_data->data_len, login_data->user_name, login_data->password);

		// 忽略判断账号密码逻辑
		LoginRetData login_ret_data = {};
		(void)client->SendData(&login_ret_data);
	}
	break;

	case Cmd::kCmdLogout:
	{
		LogoutData* logout_data = (LogoutData*)header;
		//printf("收到命令，cmd : kCmdLogout, 数据长度 ：%d, user_name : %s\n", logout_data->data_len, logout_data->user_name);

		// 忽略判断账号密码逻辑
		LogoutRetData logout_ret_data = {};
		(void)client->SendData(&logout_ret_data);
	}
	break;

	default:
	{
		printf("收到错误命令，cmd : %d\n", header->cmd);

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
