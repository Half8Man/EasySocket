#ifndef __EASY_TCP_SERVER_HPP__
#define __EASY_TCP_SERVER_HPP__

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <windows.h>
#include <WinSock2.h>

#include "Message.hpp"

class EasyTcpServer
{
public:
	EasyTcpServer()
		:svr_sock_(INVALID_SOCKET)
	{
		sock_vec_ = {};
	}

	virtual ~EasyTcpServer()
	{

	}

	// 初始化socket
	SOCKET InitSocket()
	{
		if (svr_sock_ != INVALID_SOCKET)
		{
			printf("关闭旧的连接\n");
			Close();
		}

		// 启动socket网络环境
		WORD version = MAKEWORD(2, 2);
		WSADATA data;
		(void)WSAStartup(version, &data);

		// 创建socket套接字
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

	// 绑定IP、端口号
	int Bind(const char* ip, unsigned short port)
	{
		// 绑定socket
		sockaddr_in sock_addr;
		memset(&sock_addr, 0, sizeof(sockaddr_in));
		sock_addr.sin_family = PF_INET;

		if (ip)
		{
			sock_addr.sin_addr.s_addr = inet_addr(ip);
		}
		else
		{
			sock_addr.sin_addr.s_addr = INADDR_ANY;
		}

		sock_addr.sin_port = htons(port);

		auto ret = bind(svr_sock_, (sockaddr*)&sock_addr, sizeof(sockaddr_in));
		if (ret == SOCKET_ERROR)
		{
			printf("绑定网络端口 <%d> 失败!\n", ret);
		}
		else
		{
			printf("绑定网络端口 <%d> 成功!\n", ret);
		}

		return ret;
	}

	// 监听端口号
	int Listen(int count = 20)
	{
		// 监听
		int ret = listen(svr_sock_, count);
		if (ret == SOCKET_ERROR)
		{
			printf("监听网络端口失败!\n");
		}
		else
		{
			printf("监听网络端口成功!\n");
		}

		return ret;
	}

	// 接受客户端连接
	SOCKET Accept()
	{
		// 等待客户端连接
		sockaddr_in client_addr = {};
		int size = sizeof(sockaddr_in);
		SOCKET client_sock = accept(svr_sock_, (sockaddr*)&client_addr, &size);
		if (client_sock == INVALID_SOCKET)
		{
			printf("无效客户端socket!\n");
		}
		else
		{
			NewUserJoinData new_user_join_data = {};
			new_user_join_data.sock = int(client_sock);
			SendData(&new_user_join_data, new_user_join_data.data_len);

			printf("新的客户端连接, socket : %d, ip : %s\n", int(client_sock), inet_ntoa(client_addr.sin_addr));
			sock_vec_.push_back(client_sock);
		}

		return client_sock;
	}

	// 关闭socket
	void Close()
	{
		if (svr_sock_ != INVALID_SOCKET)
		{
			// 关闭套接字
			for (const auto& sock : sock_vec_)
			{
				closesocket(sock);
			}
			closesocket(svr_sock_);

			WSACleanup(); // 终止 dll 的使用
		}
	}

	// 处理网络消息
	bool OnRun()
	{
		if (IsAlive())
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

			for (const auto& sock : sock_vec_)
			{
				FD_SET(sock, &fd_read);
			}

			// nfds 是一个int， 是指fd_set集合中所有socket的范围（最大值加1），而不是数量。
			// windows 中可以传0
			timeval time_val = { 0, 0 };
			int select_ret = select(svr_sock_ + 1, &fd_read, &fd_write, &fd_exp, &time_val);
			if (select_ret < 0)
			{
				printf("select_ret < 0，任务结束\n");
				Close();
				return false;
			}

			if (FD_ISSET(svr_sock_, &fd_read))
			{
				// 说明客户端发送了数据/连接
				FD_CLR(svr_sock_, &fd_read);

				Accept();
			}

			for (size_t i = 0; i < fd_read.fd_count; i++)
			{
				// 说明客户端退出了
				auto temp = fd_read.fd_array[i];
				if (RecvData(temp) < 0)
				{
					auto iter = find(sock_vec_.begin(), sock_vec_.end(), temp);
					if (iter != sock_vec_.end())
					{
						sock_vec_.erase(iter);
					}
				}
			}

			return true;
		}

		return false;
	}

	// 是否工作
	bool IsAlive()
	{
		return svr_sock_ != INVALID_SOCKET;
	}

	// 接受数据、处理粘包、拆分包
	int RecvData(SOCKET client_sock)
	{
		char buffer[4096] = {};

		int len = recv(client_sock, buffer, sizeof(DataHeader), 0);
		if (len <= 0)
		{
			printf("客户端已退出，任务结束\n");
			return -1;
		}

		DataHeader* header = (DataHeader*)buffer;

		recv(client_sock, buffer + sizeof(DataHeader), header->data_len - sizeof(DataHeader), 0);
		onNetMsg(client_sock, header);

		return 0;
	}

	// 响应网络消息
	virtual void onNetMsg(SOCKET client_sock, DataHeader* header)
	{
		switch (header->cmd)
		{
		case Cmd::kCmdLogin:
		{
			
			LoginData* login_data = (LoginData*)header;
			printf("收到命令，cmd : kCmdLogin, 数据长度 ：%d, user_name : %s, password : %s\n", login_data->data_len, login_data->user_name, login_data->password);

			// 忽略判断账号密码逻辑
			LoginRetData login_ret_data = {};
			(void)SendData(&login_ret_data, login_ret_data.data_len, client_sock);
		}
		break;

		case Cmd::kCmdLogout:
		{
			LogoutData* logout_data = (LogoutData*)header;
			printf("收到命令，cmd : kCmdLogout, 数据长度 ：%d, user_name : %s\n", logout_data->data_len, logout_data->user_name);

			// 忽略判断账号密码逻辑
			LogoutRetData logout_ret_data = {};
			(void)SendData(&logout_ret_data, logout_ret_data.data_len, client_sock);
		}
		break;

		default:
		{
			printf("收到错误命令，cmd : %d\n", header->cmd);

			DataHeader* temp;
			temp->cmd = Cmd::kCmdError;
			temp->data_len = 0;
			(void)SendData(temp, temp->data_len, client_sock);
		}
		break;
		}
	}

	// 发送数据
	int SendData(DataHeader* data, int len, SOCKET client_sock)
	{
		if (IsAlive() && data)
		{
			return send(client_sock, (const char*)data, len, 0);
		}

		return -1;
	}

	// 发送数据
	int SendData(DataHeader* data, int len)
	{
		if (IsAlive() && data)
		{
			for (const auto& client_sock : sock_vec_)
			{
				return send(client_sock, (const char*)data, len, 0);
			}
		}

		return -1;
	}

private:
	SOCKET svr_sock_;
	std::vector<SOCKET> sock_vec_;
};

#endif // !__EASY_TCP_SERVER_HPP__
