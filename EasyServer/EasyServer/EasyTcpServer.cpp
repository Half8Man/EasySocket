#include "EasyTcpServer.h"

EasyTcpServer::EasyTcpServer()
	:svr_sock_(INVALID_SOCKET), recv_count_(0)
{
	client_vec_ = {};
	memset(data_buffer_, 0, sizeof(data_buffer_));
}

EasyTcpServer::~EasyTcpServer()
{
	Close();
}

SOCKET EasyTcpServer::InitSock()
{
	if (svr_sock_ != INVALID_SOCKET)
	{
		printf("关闭旧连接\n");
		Close();
	}

#ifdef _WIN32
	// 启动win socket网络环境
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	(void)WSAStartup(version, &data);
#endif // _Win32
	
	svr_sock_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if (svr_sock_ == INVALID_SOCKET)
	{
		printf("建立socket失败\n");
		Close();
	}
	else
		printf("建立socket成功\n");

	return svr_sock_;
}

int EasyTcpServer::Bind(const char* ip, unsigned short port)
{
	sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sockaddr_in));
	sock_addr.sin_family = PF_INET;
	sock_addr.sin_port = htons(port);

	if (ip)
		sock_addr.sin_addr.s_addr = inet_addr(ip);
	else
		sock_addr.sin_addr.s_addr = INADDR_ANY;

	int ret = bind(svr_sock_, (sockaddr*)&sock_addr, sizeof(sock_addr));

	if (ret == SOCKET_ERROR)
		printf("绑定端口 <%d> 失败\n", ret);
	else
		printf("绑定端口 <%d> 成功\n", ret);

	return ret;
}

int EasyTcpServer::Listen(int count = 16)
{
	int ret = listen(svr_sock_, count);

	if (ret == SOCKET_ERROR)
		printf("监听网络端口失败\n");
	else
		printf("监听网络端口成功\n");

	return ret;
}

SOCKET EasyTcpServer::Accept()
{
	// 接受客户端连接
	sockaddr_in client_addr = {};

	int len = sizeof(client_addr);

	SOCKET client_sock = INVALID_SOCKET;

#ifdef _WIN32
	client_sock = accept(svr_sock_, (sockaddr*)&client_addr, &len);
#else
	client_sock = accept(svr_sock_, (sockaddr*)&client_addr, (socklen_t*)&len);
#endif // _WIN32

	if (client_sock == INVALID_SOCKET)
		printf("接受无效客户端socket\n");
	else
	{
		NewUserJoinData new_user_json_data = {};
		new_user_json_data.sock = int(client_sock);
		(void)SendData(&new_user_json_data);

		printf("新的客户端连接, socket : %d, ip : %s\n", int(client_sock), inet_ntoa(client_addr.sin_addr));
		client_vec_.push_back(new Client(client_sock));
	}
	return client_sock;
}

void EasyTcpServer::Close()
{
	if (IsRun())
	{
#ifdef _WIN32
		// 关闭客户端 socket
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

		// 关闭服务端 socket
		closesocket(svr_sock_);

		// 清楚 win socket 网络环境
		WSACleanup();
#else
		// 关闭客户端 socket
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

		// 关闭服务端 socket
		close(svr_sock_);
#endif // _WIN32
	}
}

bool EasyTcpServer::OnRun()
{
	if (IsRun())
	{
		// 伯克利socket集合
		fd_set fd_read;
		fd_set fd_write;
		fd_set fd_exp;

		// 清空集合
		FD_ZERO(&fd_read);
		FD_ZERO(&fd_write);
		FD_ZERO(&fd_exp);

		FD_SET(svr_sock_, &fd_read);
		FD_SET(svr_sock_, &fd_write);
		FD_SET(svr_sock_, &fd_exp);

		// 计算最大socket
		SOCKET max_sock = svr_sock_;
		for (const auto& client : client_vec_)
		{
			SOCKET sock = client->GetSock();
			FD_SET(sock, &fd_read);
			if (max_sock < sock)
			{
				max_sock = sock;
			}
		}

		// nfds 是一个int，是指fd_set集合中所有socket的范围（最大值加1），而不是数量。
		// windows 中可以传0
		timeval time_val = { 0, 0 };
		int ret = select(int(max_sock) + 1, &fd_read, &fd_write, &fd_exp, &time_val);
		if (ret < 0)
		{
			printf("select < 0, 任务结束\n");
			Close();
			return false;
		}

		if (FD_ISSET(svr_sock_, &fd_read))
		{
			FD_CLR(svr_sock_, &fd_read);

			Accept();
		}

		std::vector<Client*> temp_vec = {};

		for (const auto client : client_vec_)
		{
			auto temp_sock = client->GetSock();
			if (FD_ISSET(temp_sock, &fd_read))
			{
				// 说明客户端退出了
				if (RecvData(client) < 0)
				{
					auto iter = find(client_vec_.begin(), client_vec_.end(), client);
					if (iter != client_vec_.end())
					{
						temp_vec.push_back(client);
						client_vec_.erase(iter);
					}
				}
			}
		}

		for (const auto& client : temp_vec)
		{
			delete client;
		}
		temp_vec.clear();

		return true;
	}

	return false;
}

bool EasyTcpServer::IsRun()
{
	return (svr_sock_ != INVALID_SOCKET);
}

int EasyTcpServer::RecvData(Client* client)
{
	int len = recv(client->GetSock(), data_buffer_, sizeof(DataHeader), 0);
	if (len <= 0)
	{
		printf("客户端已退出，任务结束\n");
		return -1;
	}

	// 将收到的数据拷贝到消息缓冲区
	memcpy(client->GetDataBuffer() + client->GetLastPos(), data_buffer_, len);

	// 缓冲区的数据尾部后移
	client->SetLastPos(client->GetLastPos() + len);

	// 判断消息缓冲区的数据长度是都大于消息头DataHeader的长度
	while (client->GetLastPos() >= sizeof(DataHeader))
	{
		// 这时就可以知道当前消息的长度
		DataHeader* header = (DataHeader*)client->GetDataBuffer();

		// 判断消息缓冲区的数据长度是否大于一条完整消息长度
		if (client->GetLastPos() >= header->data_len)
		{
			// 未处理的消息的长度
			int size = client->GetLastPos() - header->data_len;

			// 处理消息
			OnNetMsg(client->GetSock(), header);

			// 将消息缓冲区的未处理的数据前移
			memcpy(client->GetDataBuffer(), client->GetDataBuffer() + header->data_len, size);

			// 将消息缓冲区数据尾部位置前移
			client->SetLastPos(size);
		}
		else
		{
			// 缓冲区的数据长度不足一条完整消息长度
			break;
		}
	}

	return 0;
}

int EasyTcpServer::OnNetMsg(SOCKET client_sock, DataHeader* header)
{
	recv_count_++;
	auto time_temp = time_stamp_.GetElapsedSecond();
	if (time_temp >= 1.0)
	{
		printf("时间戳 ：%lf, 收到客户端请求次数 ：%d\n", time_temp, recv_count_);
		time_stamp_.Update();
		recv_count_ = 0;
	}

	switch (header->cmd)
	{
	case Cmd::kCmdLogin:
	{
		LoginData* login_data = (LoginData*)header;
		//printf("收到命令，cmd : kCmdLogin, 数据长度 ：%d, user_name : %s, password : %s\n", login_data->data_len, login_data->user_name, login_data->password);

		// 忽略判断账号密码逻辑
		LoginRetData login_ret_data = {};
		(void)SendData(&login_ret_data, client_sock);
	}
	break;

	case Cmd::kCmdLogout:
	{
		LogoutData* logout_data = (LogoutData*)header;
		//printf("收到命令，cmd : kCmdLogout, 数据长度 ：%d, user_name : %s\n", logout_data->data_len, logout_data->user_name);

		// 忽略判断账号密码逻辑
		LogoutRetData logout_ret_data = {};
		(void)SendData(&logout_ret_data, client_sock);
	}
	break;

	default:
	{
		printf("收到错误命令，cmd : %d\n", header->cmd);

		DataHeader temp = {};
		temp.cmd = Cmd::kCmdError;
		temp.data_len = 0;
		(void)SendData(&temp, client_sock);
	}
	break;
	}

	return 0;
}

int EasyTcpServer::SendData(DataHeader* data, SOCKET client_sock)
{
	if (IsRun() && data)
		return send(client_sock, (const char*)data, data->data_len, 0);

	return SOCKET_ERROR;
}

void EasyTcpServer::SendData(DataHeader* data)
{
	if (IsRun() && data)
	{
		for (const auto& client : client_vec_)
		{
			send(client->GetSock(), (const char*)data, data->data_len, 0);
		}
	}
}
