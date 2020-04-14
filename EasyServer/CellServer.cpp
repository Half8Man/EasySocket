#include "CellServer.h"

CellServer::CellServer(int id, INetEvent *inet_event)
	: client_map_({}),
	  client_buff_vec_({}),
	  inet_event_(inet_event),
	  cell_task_svr_(nullptr),
	  cell_thread_(nullptr),
	  max_sock_(0),
	  id_(id),
	  is_clients_change_(false)
{
	FD_ZERO(&fd_read_backup_);
}

CellServer::~CellServer()
{
	Close();

	if (cell_thread_)
	{
		delete cell_thread_;
		cell_thread_ = nullptr;
	}

	if (cell_task_svr_)
	{
		delete cell_task_svr_;
		cell_task_svr_ = nullptr;
	}
}

void CellServer::Start()
{
	if (!cell_task_svr_)
	{
		cell_task_svr_ = new CellTaskServer();
		if (cell_task_svr_)
		{
			cell_task_svr_->Start();
		}
	}

	if (!cell_thread_)
	{
		cell_thread_ = new CellThread();
		if (cell_thread_)
		{
			cell_thread_->Start(
				nullptr,
				[this](CellThread *cell_thread) {
					OnRun(cell_thread);
				},
				[this](CellThread *cell_thread) {
					ClearClient();
				});
		}
	}
}

void CellServer::Close()
{
	printf("%s %d start\n", __FUNCTION__, id_);

	if (cell_task_svr_)
	{
		cell_task_svr_->Close();
	}

	if (cell_thread_)
	{
		cell_thread_->Close();
	}

	printf("%s %d end\n", __FUNCTION__, id_);
}

int CellServer::GetClientCount() const
{
	return client_map_.size() + client_buff_vec_.size();
}

bool CellServer::OnRun(CellThread *cell_thread)
{
	is_clients_change_ = true;
	while (cell_thread->IsRun())
	{
		if (!client_buff_vec_.empty())
		{
			std::lock_guard<std::mutex> lock(client_mutex_);
			for (const auto &client : client_buff_vec_)
			{
				client_map_[client->GetSock()] = client;
			}
			client_buff_vec_.clear();
			is_clients_change_ = true;
		}

		// 如果没有需要处理的客户端，跳过
		if (client_map_.empty())
		{
			old_time_ = CellTime::GetCurTimeMilliSec();

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
			for (const auto &client_pair : client_map_)
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
		timeval time_val = {0, 1};
		int ret = select(int(max_sock_) + 1, &fd_read, nullptr, nullptr, &time_val);
		if (ret < 0)
		{
			printf("CellServer OnRun select < 0, 任务结束\n");
			cell_thread_->Exit();
			break;
		}
		//else if (ret == 0)
		//{
		//	continue;
		//}

		ReadData(fd_read);
		CheckTime();
	}
	return false;
}

int CellServer::RecvData(Client *client)
{
	char *recv_data_buffer = client->GetRecvDataBuffer() + client->GetRecvLastPos();
	int len = recv(client->GetSock(), recv_data_buffer, kRecvBufferSize - client->GetRecvLastPos(), 0);
	inet_event_->OnNetRecv(client);
	if (len <= 0)
	{
		printf("客户端已退出，任务结束\n");
		return -1;
	}

	// 将收到的数据拷贝到消息缓冲区
	//memcpy(client->GetRecvDataBuffer() + client->GetRecvLastPos(), recv_data_buffer, len);

	// 缓冲区的数据尾部后移
	client->SetRecvLastPos(client->GetRecvLastPos() + len);

	// 判断消息缓冲区的数据长度是都大于消息头DataHeader的长度
	while (client->GetRecvLastPos() >= sizeof(DataHeader))
	{
		// 这时就可以知道当前消息的长度
		DataHeader *header = (DataHeader *)client->GetRecvDataBuffer();

		// 判断消息缓冲区的数据长度是否大于一条完整消息长度
		if (client->GetRecvLastPos() >= header->data_len)
		{
			// 未处理的消息的长度
			int size = client->GetRecvLastPos() - header->data_len;

			// 处理消息
			OnNetMsg(this, client, header);

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

void CellServer::ReadData(fd_set &fd_read)
{
	std::vector<Client *> client_vec_temp = {};

#ifdef _WIN32
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
#else
	for (auto &iter : client_map_)
	{
		if (FD_ISSET(iter.second->GetSock(), &fd_read))
		{
			auto client = iter.second;
			// 说明客户端退出了
			if (RecvData(client) < 0)
			{
				is_clients_change_ = true;
				client_vec_temp.push_back(client);
				inet_event_->OnLeave(client);
			}
		}
	}
#endif // _WIN32

	for (const auto *client : client_vec_temp)
	{
		if (client)
		{
			client_map_.erase(client->GetSock());
			delete client;
		}
	}
	client_vec_temp.clear();
}

void CellServer::CheckTime()
{
	auto now = CellTime::GetCurTimeMilliSec();
	auto dt = now - old_time_;
	old_time_ = now;

	std::vector<Client *> client_vec_temp = {};

	for (auto &iter : client_map_)
	{
		auto client = iter.second;

		// 心跳检测
		if (client->CheckHeart(dt))
		{
			printf("remove client\n");
			is_clients_change_ = true;
			client_vec_temp.push_back(client);
			inet_event_->OnLeave(client);
		}

		// 定时发送检测
		client->CheckSend(dt);
	}

	for (const auto *client : client_vec_temp)
	{
		if (client)
		{
			client_map_.erase(client->GetSock());
			delete client;
		}
	}
	client_vec_temp.clear();
}

int CellServer::OnNetMsg(CellServer *cell_svr, Client *client, DataHeader *header)
{
	inet_event_->OnNetMsg(this, client, header);

	switch (header->cmd)
	{
	case Cmd::kCmdHeartBeatC2s:
	{
		client->ResetHeartBeatDelay();

		HeartBeatS2cData data = {};
		(void)client->SendData(&data);
	}
	break;

	case Cmd::kCmdLogin:
	{
		LoginData *login_data = (LoginData *)header;
		//printf("收到命令，cmd : kCmdLogin, 数据长度 ：%d, user_name : %s, password : %s\n", login_data->data_len, login_data->user_name, login_data->password);

		// 忽略判断账号密码逻辑
		LoginRetData login_ret_data = {};
		(void)client->SendData(&login_ret_data);

		//LoginRetData *login_ret_data = new LoginRetData();
		//cell_svr->AddSendTask(client, login_ret_data);
	}
	break;

	case Cmd::kCmdLogout:
	{
		LogoutData *logout_data = (LogoutData *)header;
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

int CellServer::SendData(DataHeader *data, SOCKET client_sock)
{
	if (data)
	{
		return send(client_sock, (const char *)data, data->data_len, 0);
	}

	return SOCKET_ERROR;
}

void CellServer::SendData(DataHeader *data)
{
	if (data)
	{
		for (const auto &client_pair : client_map_)
		{
			auto client = client_pair.second;
			send(client->GetSock(), (const char *)data, data->data_len, 0);
		}
	}
}

void CellServer::AddClient(Client *client)
{
	if (client)
	{
		std::lock_guard<std::mutex> lock(client_mutex_);
		client_buff_vec_.push_back(client);
	}

	inet_event_->OnJoin(client);
}

void CellServer::AddSendTask(Client *client, DataHeader *data)
{
	cell_task_svr_->AddTask(
		[client, data]() {
			client->SendData(data);
			delete data;
		});
}

void CellServer::ClearClient()
{
	for (const auto &client_pair : client_map_)
	{
		auto client = client_pair.second;
		if (client)
		{
			delete client;
		}
	}
	client_map_.clear();

	for (const auto &client : client_buff_vec_)
	{
		if (client)
		{
			delete client;
		}
	}
	client_buff_vec_.clear();
}

CellSendMsg2ClientTask::CellSendMsg2ClientTask(Client *client, DataHeader *data)
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
