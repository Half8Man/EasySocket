#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <string>
#include <stdio.h>
#include <windows.h>
#include <WinSock2.h>

const std::string kIp = "127.0.0.1";
const int kPort = 1234;

enum Cmd
{
	kCmdLogin,
	kCmdLoginRet,
	kCmdLogout,
	kCmdLogoutRet,
	kCmdError
};

struct DataHeader
{
	int data_len;
	int cmd;
};

// DataPackage
struct LoginData : public DataHeader
{
	LoginData()
	{
		data_len = sizeof(LoginData);
		cmd = Cmd::kCmdLogin;
	}

	char user_name[64];
	char password[64];
};

struct LoginRetData : public DataHeader
{
	LoginRetData()
	{
		data_len = sizeof(LoginRetData);
		cmd = Cmd::kCmdLoginRet;
		ret = 0;
	}

	int ret;
};

struct LogoutData : public DataHeader
{
	LogoutData()
	{
		data_len = sizeof(data_len);
		cmd = Cmd::kCmdLogout;
	}

	char user_name[64];
};

struct LogoutRetData : public DataHeader
{
	LogoutRetData()
	{
		data_len = sizeof(LogoutRetData);
		cmd = Cmd::kCmdLogoutRet;
		ret = 0;
	}

	int ret;
};

int main()
{
	// 启动socket网络环境
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	(void)WSAStartup(version, &data);

	// 创建socket套接字
	SOCKET svr_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	// 绑定socket
	sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sockaddr_in));
	sock_addr.sin_family = PF_INET;
	sock_addr.sin_addr.s_addr = inet_addr(kIp.c_str());
	sock_addr.sin_port = htons(kPort);
	if (bind(svr_sock, (sockaddr*)&sock_addr, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		printf("绑定网络端口失败!\n");
	}
	else
	{
		printf("绑定网络端口成功!\n");
	}

	// 监听
	if (listen(svr_sock, 20) == SOCKET_ERROR)
	{
		printf("监听网络端口失败!\n");
	}
	else
	{
		printf("监听网络端口成功!\n");
	}

	// 等待客户端连接
	sockaddr_in client_addr = {};
	int size = sizeof(sockaddr_in);
	SOCKET client_sock = accept(svr_sock, (sockaddr*)&client_addr, &size);
	if (client_sock == INVALID_SOCKET)
	{
		printf("无效客户端socket!\n");
	}
	else
	{
		printf("新的客户端连接, socket : %d, ip : %s\n", int(client_sock), inet_ntoa(client_addr.sin_addr));
	}

	while (true)
	{
		DataHeader header = {};

		int len = recv(client_sock, (char*)&header, sizeof(DataHeader), 0);
		if (len <= 0)
		{
			printf("客户端已退出，任务结束");
			break;
		}

		switch (header.cmd)
		{
		case Cmd::kCmdLogin:
		{
			LoginData login_data = {};
			recv(client_sock, (char*)&login_data + sizeof(DataHeader), sizeof(LoginData) - sizeof(DataHeader), 0);
			printf("收到命令，cmd : kCmdLogin, 数据长度 ：%d, user_name : %s, password : %s\n", login_data.data_len, login_data.user_name, login_data.password);

			// 忽略判断账号密码逻辑
			LoginRetData login_ret_data = {};
			send(client_sock, (char*)&login_ret_data, sizeof(LoginRetData), 0);
			break;
		}
		case Cmd::kCmdLogout:
		{
			LogoutData logout_data = {};
			recv(client_sock, (char*)&logout_data + sizeof(DataHeader), sizeof(LogoutData) - sizeof(DataHeader), 0);
			printf("收到命令，cmd : kCmdLogout, 数据长度 ：%d, user_name : %s\n", logout_data.data_len, logout_data.user_name);

			// 忽略判断账号密码逻辑
			LogoutRetData logout_ret_data = {};
			send(client_sock, (char*)&logout_ret_data, sizeof(LogoutRetData), 0);
			break;
		}
		default:
		{
			printf("收到错误命令，cmd : %d\n", header.cmd);

			header.cmd = Cmd::kCmdError;
			header.data_len = 0;
			send(client_sock, (char*)&header, sizeof(DataHeader), 0);
			break;
		}
		}
	}

	// 关闭套接字
	closesocket(client_sock);
	closesocket(svr_sock);

	WSACleanup(); // 终止 dll 的使用
	return 0;
}