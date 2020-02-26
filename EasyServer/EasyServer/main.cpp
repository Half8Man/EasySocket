#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <string>
#include <vector>
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
	kCmdNewUserJoin,
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

struct NewUserJoinData : public DataHeader
{
	NewUserJoinData()
	{
		data_len = sizeof(NewUserJoinData);
		cmd = Cmd::kCmdNewUserJoin;
	}

	int sock;
};

int processor(SOCKET client_sock)
{
	char buffer[4096] = {};

	int len = recv(client_sock, buffer, sizeof(DataHeader), 0);
	if (len <= 0)
	{
		printf("�ͻ������˳����������\n");
		return -1;
	}

	DataHeader* header = (DataHeader*)buffer;

	switch (header->cmd)
	{
	case Cmd::kCmdLogin:
	{
		recv(client_sock, buffer + sizeof(DataHeader), header->data_len - sizeof(DataHeader), 0);
		LoginData* login_data = (LoginData*)buffer;
		printf("�յ����cmd : kCmdLogin, ���ݳ��� ��%d, user_name : %s, password : %s\n", login_data->data_len, login_data->user_name, login_data->password);

		// �����ж��˺������߼�
		LoginRetData login_ret_data = {};
		send(client_sock, (char*)&login_ret_data, sizeof(LoginRetData), 0);
	}
	break;

	case Cmd::kCmdLogout:
	{
		recv(client_sock, buffer + sizeof(DataHeader), header->data_len - sizeof(DataHeader), 0);
		LogoutData* logout_data = (LogoutData*)buffer;
		printf("�յ����cmd : kCmdLogout, ���ݳ��� ��%d, user_name : %s\n", logout_data->data_len, logout_data->user_name);

		// �����ж��˺������߼�
		LogoutRetData logout_ret_data = {};
		send(client_sock, (char*)&logout_ret_data, sizeof(LogoutRetData), 0);
	}
	break;

	default:
	{
		printf("�յ��������cmd : %d\n", header->cmd);

		header->cmd = Cmd::kCmdError;
		header->data_len = 0;
		send(client_sock, (char*)&header, sizeof(DataHeader), 0);
	}
	break;
	}

	return 0;
}

int main()
{
	// ����socket���绷��
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	(void)WSAStartup(version, &data);

	// ����socket�׽���
	SOCKET svr_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	// ��socket
	sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sockaddr_in));
	sock_addr.sin_family = PF_INET;
	sock_addr.sin_addr.s_addr = inet_addr(kIp.c_str());
	sock_addr.sin_port = htons(kPort);
	if (bind(svr_sock, (sockaddr*)&sock_addr, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		printf("������˿�ʧ��!\n");
	}
	else
	{
		printf("������˿ڳɹ�!\n");
	}

	// ����
	if (listen(svr_sock, 20) == SOCKET_ERROR)
	{
		printf("��������˿�ʧ��!\n");
	}
	else
	{
		printf("��������˿ڳɹ�!\n");
	}

	// ���ڴ�ſͻ���socket
	std::vector<SOCKET> sock_vec = {};

	while (true)
	{
		// ������socket����
		fd_set fd_read;
		fd_set fd_write;
		fd_set fd_exp;

		// ��ռ���
		FD_ZERO(&fd_read);
		FD_ZERO(&fd_write);
		FD_ZERO(&fd_exp);

		FD_SET(svr_sock, &fd_read);
		FD_SET(svr_sock, &fd_write);
		FD_SET(svr_sock, &fd_exp);

		for (const auto& sock : sock_vec)
		{
			FD_SET(sock, &fd_read);
		}

		// nfds ��һ��int�� ��ָfd_set����������socket�ķ�Χ�����ֵ��1����������������
		// windows �п��Դ�0
		timeval time_val = { 0, 0 };
		int select_ret = select(svr_sock + 1, &fd_read, &fd_write, &fd_exp, &time_val);
		if (select_ret < 0)
		{
			printf("select_ret < 0���������\n");
			break;
		}

		if (FD_ISSET(svr_sock, &fd_read))
		{
			// ˵���ͻ��˷���������/����
			FD_CLR(svr_sock, &fd_read);

			// �ȴ��ͻ�������
			sockaddr_in client_addr = {};
			int size = sizeof(sockaddr_in);
			SOCKET client_sock = accept(svr_sock, (sockaddr*)&client_addr, &size);
			if (client_sock == INVALID_SOCKET)
			{
				printf("��Ч�ͻ���socket!\n");
			}
			else
			{
				for (const auto& sock : sock_vec)
				{
					NewUserJoinData new_user_join_data = {};
					new_user_join_data.sock = int(client_sock);
					send(sock, (char*)&new_user_join_data, sizeof(NewUserJoinData), 0);
				}

				printf("�µĿͻ�������, socket : %d, ip : %s\n", int(client_sock), inet_ntoa(client_addr.sin_addr));
				sock_vec.push_back(client_sock);
			}
		}

		for (size_t i = 0; i < fd_read.fd_count; i++)
		{
			// ˵���ͻ����˳���
			auto temp = fd_read.fd_array[i];
			if (processor(temp) < 0)
			{
				auto iter = find(sock_vec.begin(), sock_vec.end(), temp);
				if (iter != sock_vec.end())
				{
					sock_vec.erase(iter);
				}
			}
		}
	}

	// �ر��׽���
	for (const auto& sock : sock_vec)
	{
		closesocket(sock);
	}
	closesocket(svr_sock);

	WSACleanup(); // ��ֹ dll ��ʹ��
	return 0;
}