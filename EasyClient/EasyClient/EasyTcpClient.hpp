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

	// ����������
	virtual ~EasyTcpClient()
	{
		if (client_sock_ != INVALID_SOCKET)
		{
			Close();

			client_sock_ = INVALID_SOCKET;
		}
	}

	// ��ʼ��socket
	int InitSocket()
	{
		if (client_sock_ != INVALID_SOCKET)
		{
			printf("�رվɵ�����\n");
			Close();
		}

		// ����socket���绷��
		WORD version = MAKEWORD(2, 2);
		WSADATA data;
		(void)WSAStartup(version, &data);

		// ����socket�׽���
		client_sock_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (client_sock_ == INVALID_SOCKET)
		{
			printf("����socketʧ��\n");
			return -1;
		}

		printf("����socket�ɹ��� socket : %d\n", int(socket));
		return 0;
	}

	// ���ӷ�����
	int Connect(const char* ip, unsigned short port)
	{
		if (client_sock_ == INVALID_SOCKET)
		{
			(void)InitSocket();
		}

		// ���ӷ�����
		sockaddr_in client_addr;
		memset(&client_addr, 0, sizeof(sockaddr_in));
		client_addr.sin_family = PF_INET;
		client_addr.sin_addr.s_addr = inet_addr(ip);
		client_addr.sin_port = htons(port);

		auto ret = connect(client_sock_, (sockaddr*)&client_addr, sizeof(sockaddr_in));
		if (ret == SOCKET_ERROR)
		{
			printf("���ӷ�����ʧ��\n");
		}
		else
		{
			printf("���ӷ������ɹ�\n");
		}

		return ret;
	}

	// �ر�socket
	void Close()
	{
		// �ر��׽���
		closesocket(client_sock_);

		WSACleanup(); // ��ֹ dll ��ʹ��
	}

	// ��������
	int SendData(DataHeader* data, int len)
	{
		if (IsAlive() && data)
		{
			return send(client_sock_, (const char*)data, len, 0);
		}

		return -1;
	}

	// �������ݣ�����ճ������ְ�
	int OnRecvData(SOCKET sock)
	{
		char buffer[4096] = { };
		int len = recv(sock, buffer, sizeof(DataHeader), 0);
		if (len <= 0)
		{
			printf("���������ӶϿ����������\n");
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
			printf("��¼���: %d \n", login_ret_data->ret);
		}
		break;

		case Cmd::kCmdLogoutRet:
		{
			LogoutRetData* logout_ret_data = (LogoutRetData*)header;
			printf("�ǳ����: %d \n", logout_ret_data->ret);
		}
		break;

		case Cmd::kCmdNewUserJoin:
		{
			NewUserJoinData* new_user_join_data = (NewUserJoinData*)header;
			printf("���û�����, socket : %d \n", new_user_join_data->sock);
		}
		break;

		default:
			break;
		}

		return 0;
	}

	// ����������Ϣ
	int OnRequest()
	{
		if (IsAlive())
		{
			// ������socket����
			fd_set fd_read;

			// ��ռ���
			FD_ZERO(&fd_read);

			FD_SET(client_sock_, &fd_read);

			timeval t = { 1,0 };
			int select_ret = select(client_sock_, &fd_read, nullptr, nullptr, &t);
			if (select_ret < 0)
			{
				printf("select_ret < 0���������\n");
				return -1;
			}

			if (FD_ISSET(client_sock_, &fd_read))
			{
				FD_CLR(client_sock_, &fd_read);

				if (OnRecvData(client_sock_) < 0)
				{
					printf("select �������\n");
					return -1;
				}
			}

			return 0;
		}

		return -1;
	}

	// �Ƿ���
	bool IsAlive()
	{
		return client_sock_ != INVALID_SOCKET;
	}
private:
	SOCKET client_sock_;
};

#endif