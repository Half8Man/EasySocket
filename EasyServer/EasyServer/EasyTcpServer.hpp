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

	// ��ʼ��socket
	SOCKET InitSocket()
	{
		if (svr_sock_ != INVALID_SOCKET)
		{
			printf("�رվɵ�����\n");
			Close();
		}

		// ����socket���绷��
		WORD version = MAKEWORD(2, 2);
		WSADATA data;
		(void)WSAStartup(version, &data);

		// ����socket�׽���
		svr_sock_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (svr_sock_ == INVALID_SOCKET)
		{
			printf("����socketʧ��\n");
			Close();
		}
		else
		{
			printf("����socket�ɹ�\n");
		}

		return svr_sock_;
	}

	// ��IP���˿ں�
	int Bind(const char* ip, unsigned short port)
	{
		// ��socket
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
			printf("������˿� <%d> ʧ��!\n", ret);
		}
		else
		{
			printf("������˿� <%d> �ɹ�!\n", ret);
		}

		return ret;
	}

	// �����˿ں�
	int Listen(int count = 20)
	{
		// ����
		int ret = listen(svr_sock_, count);
		if (ret == SOCKET_ERROR)
		{
			printf("��������˿�ʧ��!\n");
		}
		else
		{
			printf("��������˿ڳɹ�!\n");
		}

		return ret;
	}

	// ���ܿͻ�������
	SOCKET Accept()
	{
		// �ȴ��ͻ�������
		sockaddr_in client_addr = {};
		int size = sizeof(sockaddr_in);
		SOCKET client_sock = accept(svr_sock_, (sockaddr*)&client_addr, &size);
		if (client_sock == INVALID_SOCKET)
		{
			printf("��Ч�ͻ���socket!\n");
		}
		else
		{
			NewUserJoinData new_user_join_data = {};
			new_user_join_data.sock = int(client_sock);
			SendData(&new_user_join_data, new_user_join_data.data_len);

			printf("�µĿͻ�������, socket : %d, ip : %s\n", int(client_sock), inet_ntoa(client_addr.sin_addr));
			sock_vec_.push_back(client_sock);
		}

		return client_sock;
	}

	// �ر�socket
	void Close()
	{
		if (svr_sock_ != INVALID_SOCKET)
		{
			// �ر��׽���
			for (const auto& sock : sock_vec_)
			{
				closesocket(sock);
			}
			closesocket(svr_sock_);

			WSACleanup(); // ��ֹ dll ��ʹ��
		}
	}

	// ����������Ϣ
	bool OnRun()
	{
		if (IsAlive())
		{
			// ������socket����
			fd_set fd_read;
			fd_set fd_write;
			fd_set fd_exp;

			// ��ռ���
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

			// nfds ��һ��int�� ��ָfd_set����������socket�ķ�Χ�����ֵ��1����������������
			// windows �п��Դ�0
			timeval time_val = { 0, 0 };
			int select_ret = select(svr_sock_ + 1, &fd_read, &fd_write, &fd_exp, &time_val);
			if (select_ret < 0)
			{
				printf("select_ret < 0���������\n");
				Close();
				return false;
			}

			if (FD_ISSET(svr_sock_, &fd_read))
			{
				// ˵���ͻ��˷���������/����
				FD_CLR(svr_sock_, &fd_read);

				Accept();
			}

			for (size_t i = 0; i < fd_read.fd_count; i++)
			{
				// ˵���ͻ����˳���
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

	// �Ƿ���
	bool IsAlive()
	{
		return svr_sock_ != INVALID_SOCKET;
	}

	// �������ݡ�����ճ������ְ�
	int RecvData(SOCKET client_sock)
	{
		char buffer[4096] = {};

		int len = recv(client_sock, buffer, sizeof(DataHeader), 0);
		if (len <= 0)
		{
			printf("�ͻ������˳����������\n");
			return -1;
		}

		DataHeader* header = (DataHeader*)buffer;

		recv(client_sock, buffer + sizeof(DataHeader), header->data_len - sizeof(DataHeader), 0);
		onNetMsg(client_sock, header);

		return 0;
	}

	// ��Ӧ������Ϣ
	virtual void onNetMsg(SOCKET client_sock, DataHeader* header)
	{
		switch (header->cmd)
		{
		case Cmd::kCmdLogin:
		{
			
			LoginData* login_data = (LoginData*)header;
			printf("�յ����cmd : kCmdLogin, ���ݳ��� ��%d, user_name : %s, password : %s\n", login_data->data_len, login_data->user_name, login_data->password);

			// �����ж��˺������߼�
			LoginRetData login_ret_data = {};
			(void)SendData(&login_ret_data, login_ret_data.data_len, client_sock);
		}
		break;

		case Cmd::kCmdLogout:
		{
			LogoutData* logout_data = (LogoutData*)header;
			printf("�յ����cmd : kCmdLogout, ���ݳ��� ��%d, user_name : %s\n", logout_data->data_len, logout_data->user_name);

			// �����ж��˺������߼�
			LogoutRetData logout_ret_data = {};
			(void)SendData(&logout_ret_data, logout_ret_data.data_len, client_sock);
		}
		break;

		default:
		{
			printf("�յ��������cmd : %d\n", header->cmd);

			DataHeader* temp;
			temp->cmd = Cmd::kCmdError;
			temp->data_len = 0;
			(void)SendData(temp, temp->data_len, client_sock);
		}
		break;
		}
	}

	// ��������
	int SendData(DataHeader* data, int len, SOCKET client_sock)
	{
		if (IsAlive() && data)
		{
			return send(client_sock, (const char*)data, len, 0);
		}

		return -1;
	}

	// ��������
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
