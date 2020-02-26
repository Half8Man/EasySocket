#ifndef __EASY_TCP_CLIENT_HPP__
#define __EASY_TCP_CLIENT_HPP__

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <string>
#include <thread>
#include <stdio.h>
#include <windows.h>
#include <WinSock2.h>

#include "Message.hpp"

class EasyTcpClient
{
public:
	EasyTcpClient()
		:client_sock_(INVALID_SOCKET)
	{

	}

	// 虚析构函数
	virtual ~EasyTcpClient()
	{
		if (client_sock_ != INVALID_SOCKET)
		{
			Close();

			client_sock_ = INVALID_SOCKET;
		}
	}

	// 初始化socket
	int InitSocket()
	{
		if (client_sock_ != INVALID_SOCKET)
		{
			printf("关闭旧的连接\n");
			Close();
		}

		// 启动socket网络环境
		WORD version = MAKEWORD(2, 2);
		WSADATA data;
		(void)WSAStartup(version, &data);

		// 创建socket套接字
		client_sock_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (client_sock_ == INVALID_SOCKET)
		{
			printf("创建socket失败\n");
			return -1;
		}

		printf("创建socket成功， socket : %d\n", int(socket));
		return 0;
	}

	// 连接服务器
	int Connect(const char* ip, unsigned short port)
	{
		if (client_sock_ == INVALID_SOCKET)
		{
			(void)InitSocket();
		}

		// 连接服务器
		sockaddr_in client_addr;
		memset(&client_addr, 0, sizeof(sockaddr_in));
		client_addr.sin_family = PF_INET;
		client_addr.sin_addr.s_addr = inet_addr(ip);
		client_addr.sin_port = htons(port);

		auto ret = connect(client_sock_, (sockaddr*)&client_addr, sizeof(sockaddr_in));
		if (ret == SOCKET_ERROR)
		{
			printf("连接服务器失败\n");
		}
		else
		{
			printf("连接服务器成功\n");
		}

		return ret;
	}

	// 关闭socket
	void Close()
	{
		// 关闭套接字
		closesocket(client_sock_);

		WSACleanup(); // 终止 dll 的使用
	}

	// 发送数据
	int SendData(DataHeader* data, int len)
	{
		if (IsAlive() && data)
		{
			return send(client_sock_, (const char*)data, len, 0);
		}

		return -1;
	}

	// 接受数据，处理粘包，拆分包
	int OnRecvData(SOCKET sock)
	{
		char buffer[4096] = { };
		int len = recv(sock, buffer, sizeof(DataHeader), 0);
		if (len <= 0)
		{
			printf("与服务端连接断开，任务结束\n");
			return -1;
		}

		DataHeader* header = (DataHeader*)buffer;

		recv(sock, buffer + sizeof(DataHeader), header->data_len - sizeof(DataHeader), 0);
		return DealMsg(sock, header);
	}

	int DealMsg(SOCKET sock, DataHeader* header)
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

		default:
			break;
		}

		return 0;
	}

	// 处理网络消息
	int OnRequest()
	{
		if (IsAlive())
		{
			// 伯克利socket集合
			fd_set fd_read;

			// 清空集合
			FD_ZERO(&fd_read);

			FD_SET(client_sock_, &fd_read);

			timeval t = { 1,0 };
			int select_ret = select(client_sock_, &fd_read, nullptr, nullptr, &t);
			if (select_ret < 0)
			{
				printf("select_ret < 0，任务结束\n");
				return -1;
			}

			if (FD_ISSET(client_sock_, &fd_read))
			{
				FD_CLR(client_sock_, &fd_read);

				if (OnRecvData(client_sock_) < 0)
				{
					printf("select 任务结束\n");
					return -1;
				}
			}

			return 0;
		}

		return -1;
	}

	// 是否工作
	bool IsAlive()
	{
		return client_sock_ != INVALID_SOCKET;
	}
private:
	SOCKET client_sock_;
};

#endif