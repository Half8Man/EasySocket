#include "EasyTcpClient.h"

EasyTcpClient::EasyTcpClient()
	:client_sock_(INVALID_SOCKET), last_pos(0)
{
	//memset(first_data_buffer_, 0, sizeof(first_data_buffer_));
	memset(second_data_buffer_, 0, sizeof(second_data_buffer_));
}

EasyTcpClient::~EasyTcpClient()
{
	if (client_sock_ != INVALID_SOCKET)
	{
		Close();

		client_sock_ = INVALID_SOCKET;
	}
}

SOCKET EasyTcpClient::InitSocket()
{
	if (client_sock_ != INVALID_SOCKET)
	{
		printf("关闭旧的连接\n");
		Close();
	}

#ifdef _WIN32
	// 启动socket网络环境
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	(void)WSAStartup(version, &data);
#endif

	// 创建socket套接字
	client_sock_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client_sock_ == INVALID_SOCKET)
	{
		printf("创建socket失败\n");
		return -1;
	}

	//printf("创建socket成功， socket : %d\n", int(socket));
	return 0;
}

int EasyTcpClient::Connect(const char* ip, unsigned short port)
{
	if (client_sock_ == INVALID_SOCKET)
		(void)InitSocket();

	// 连接服务器
	sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(sockaddr_in));
	client_addr.sin_family = PF_INET;
	client_addr.sin_addr.s_addr = inet_addr(ip);
	client_addr.sin_port = htons(port);

	auto ret = connect(client_sock_, (sockaddr*)&client_addr, sizeof(sockaddr_in));
	//if (ret == SOCKET_ERROR)
	//	printf("连接服务器失败\n");
	//else
	//	printf("连接服务器成功\n");

	return ret;
}

void EasyTcpClient::Close()
{
#ifdef _WIN32
	// 关闭套接字
	closesocket(client_sock_);

	WSACleanup(); // 终止 dll 的使用
#else
	close(client_sock_);
#endif

	client_sock_ = INVALID_SOCKET;
}

int EasyTcpClient::SendData(DataHeader* data)
{
	if (IsRun() && data)
		return send(client_sock_, (const char*)data, data->data_len, 0);

	return -1;
}

int EasyTcpClient::OnRecvData()
{
	char* first_data_buffer_ = second_data_buffer_ + last_pos;
	int len = recv(client_sock_, first_data_buffer_, kBufferSize - last_pos, 0);
	if (len <= 0)
	{
		printf("与服务端连接断开，任务结束\n");
		return -1;
	}

	//// 将第一缓冲区的数据拷贝到第二缓冲区
	//memcpy(second_data_buffer_ + last_pos, first_data_buffer_, len);

	// 第二消息缓冲区数据尾部位置后移
	last_pos += len;

	// 判断消息缓冲区的数据长度是否大于消息头DataHeader的长度
	while (last_pos >= sizeof(DataHeader))
	{
		// 此时可通过header知道消息总长度
		DataHeader* header = (DataHeader*)second_data_buffer_;

		// 判断消息缓冲区的数据长度是否大于消息总长度
		if (last_pos >= header->data_len)
		{
			// 剩余未处理的消息缓冲区的长度
			int len = last_pos - header->data_len;

			// 处理消息
			DealMsg(header);

			// 未处理数据前移
			int a = sizeof(header);
			memcpy(second_data_buffer_, second_data_buffer_ + header->data_len, len);
			last_pos = len;
		}
		else
		{
			// 剩余数据不足一条完整的消息
			break;
		}
	}

	return 0;
}

int EasyTcpClient::DealMsg(DataHeader* header)
{
	switch (header->cmd)
	{
	case Cmd::kCmdLoginRet:
	{
		LoginRetData* login_ret_data = (LoginRetData*)header;
		printf("登录结果: %d \n", login_ret_data->ret);
	}
	break;

	case Cmd::kCmdLogoutRet:
	{
		LogoutRetData* logout_ret_data = (LogoutRetData*)header;
		printf("登出结果: %d \n", logout_ret_data->ret);
	}
	break;

	case Cmd::kCmdNewUserJoin:
	{
		NewUserJoinData* new_user_join_data = (NewUserJoinData*)header;
		printf("新用户加入, socket : %d \n", new_user_join_data->sock);
	}
	break;

	case Cmd::kCmdError:
	{
		printf("收到错误的消息\n");
	}
	break;

	default:
	{
		printf("收到未定义的消息\n");
	}
	break;
	}

	return 0;
}

int EasyTcpClient::OnRun()
{
	if (IsRun())
	{
		// 伯克利socket集合
		fd_set fd_read;

		// 清空集合
		FD_ZERO(&fd_read);

		FD_SET(client_sock_, &fd_read);

		timeval t = { 0, 0 };
		int select_ret = select(client_sock_ + 1, &fd_read, nullptr, nullptr, &t);
		if (select_ret < 0)
		{
			printf("select_ret < 0，任务结束\n");
			Close();
			return -1;
		}

		if (FD_ISSET(client_sock_, &fd_read))
		{
			FD_CLR(client_sock_, &fd_read);

			if (OnRecvData() < 0)
			{
				printf("select 任务结束\n");
				Close();
				return -1;
			}
		}

		return 0;
	}

	return -1;
}

bool EasyTcpClient::IsRun()
{
	return client_sock_ != INVALID_SOCKET;
}
