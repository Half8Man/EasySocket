#include "EasyTcpServer.h"

EasyTcpServer::EasyTcpServer()
	: svr_sock_(INVALID_SOCKET), msg_count_(0), recv_count_(0), client_count_(0)
{
	cell_server_vec_ = {};
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
	{
		printf("建立socket成功\n");
	}

	return svr_sock_;
}

int EasyTcpServer::Bind(const char *ip, unsigned short port)
{
	sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sockaddr_in));
	sock_addr.sin_family = PF_INET;
	sock_addr.sin_port = htons(port);

	if (ip)
	{
		sock_addr.sin_addr.s_addr = inet_addr(ip);
	}
	else
	{
		sock_addr.sin_addr.s_addr = INADDR_ANY;
	}

	int ret = bind(svr_sock_, (sockaddr *)&sock_addr, sizeof(sock_addr));

	if (ret == SOCKET_ERROR)
	{
		printf("绑定端口 <%d> 失败\n", ret);
	}
	else
	{
		printf("绑定端口 <%d> 成功\n", ret);
	}

	return ret;
}

int EasyTcpServer::Listen(int count = 16)
{
	int ret = listen(svr_sock_, count);

	if (ret == SOCKET_ERROR)
	{
		printf("监听网络端口失败\n");
	}
	else
	{
		printf("监听网络端口成功\n");
	}

	return ret;
}

SOCKET EasyTcpServer::Accept()
{
	// 接受客户端连接
	sockaddr_in client_addr = {};

	int len = sizeof(client_addr);

	SOCKET client_sock = INVALID_SOCKET;

#ifdef _WIN32
	client_sock = accept(svr_sock_, (sockaddr *)&client_addr, &len);
#else
	client_sock = accept(svr_sock_, (sockaddr *)&client_addr, (socklen_t *)&len);
#endif // _WIN32

	if (client_sock == INVALID_SOCKET)
	{
		printf("接受无效客户端socket\n");
	}
	else
	{
		//printf("新的客户端连接, socket : %d, ip : %s\n", int(client_sock), inet_ntoa(client_addr.sin_addr));
		AddClient2CellServer(new Client(client_sock));
	}
	return client_sock;
}

void EasyTcpServer::Start(int cell_server_count)
{
	for (int i = 0; i < cell_server_count; i++)
	{
		CellServer *cell_server = new CellServer(i + 1, this);
		cell_server_vec_.push_back(cell_server);
		cell_server->Start();
	}
}

void EasyTcpServer::AddClient2CellServer(Client *client)
{
	if (client)
	{
		// 查找客户端数量最小的CellServer
		auto min_server = cell_server_vec_[0];
		for (auto cell_server : cell_server_vec_)
		{
			if (cell_server->GetClientCount() < min_server->GetClientCount())
			{
				min_server = cell_server;
			}
		}
		min_server->AddClient(client);
	}
}

void EasyTcpServer::Close()
{
	printf("%s start\n", __FUNCTION__);

	if (IsRun())
	{
		for (const auto& cell_server : cell_server_vec_)
		{
			if (cell_server)
			{
				delete cell_server;
			}
		}
		cell_server_vec_.clear();

#ifdef _WIN32
		// 关闭服务端 socket
		closesocket(svr_sock_);

		// 清楚 win socket 网络环境
		WSACleanup();
#else
		// 关闭服务端 socket
		close(svr_sock_);
#endif // _WIN32
	}

	printf("%s end\n", __FUNCTION__);
}

bool EasyTcpServer::OnRun()
{
	if (IsRun())
	{
		Time4Pkg();

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

		// nfds 是一个int，是指fd_set集合中所有socket的范围（最大值加1），而不是数量。
		// windows 中可以传0
		timeval time_val = {0, 10};
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

		return true;
	}

	return false;
}

bool EasyTcpServer::IsRun()
{
	return (svr_sock_ != INVALID_SOCKET);
}

void EasyTcpServer::Time4Pkg()
{
	auto time_temp = time_stamp_.GetElapsedSecond();
	if (time_temp >= 1.0)
	{
		printf("时间 : %lf, svr_sock : %lld, 客户端数量 : %d, recv次数 : %d, msg数量 : %d\n", time_temp, svr_sock_, int(client_count_), int(recv_count_ / time_temp), int(msg_count_ / time_temp));
		time_stamp_.Update();
		recv_count_ = 0;
		msg_count_ = 0;
	}
}

// 线程不安全
void EasyTcpServer::OnJoin(Client *client)
{
	client_count_++;
}

// 线程不安全
void EasyTcpServer::OnLeave(Client *client)
{
	client_count_--;
}

// 线程不安全
void EasyTcpServer::OnNetMsg(CellServer *cell_svr, Client *client, DataHeader *header)
{
	msg_count_++;
}

void EasyTcpServer::OnNetRecv(Client *client)
{
	recv_count_++;
}
