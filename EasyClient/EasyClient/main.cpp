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

int processor(SOCKET sock)
{
	DataHeader header = {};
	int len = recv(sock, (char*)&header, sizeof(DataHeader), 0);
	if (len <= 0)
	{
		printf("���������ӶϿ����������\n");
		return -1;
	}

	switch (header.cmd)
	{
	case Cmd::kCmdLoginRet:
	{
		LoginRetData login_ret_data = {};
		recv(sock, (char*)&login_ret_data + sizeof(DataHeader), sizeof(LoginRetData) - sizeof(DataHeader), 0);
		printf("��¼���: %d \n", login_ret_data.ret);
	}
	break;

	case Cmd::kCmdLogoutRet:
	{
		LogoutRetData logout_ret_data = {};
		recv(sock, (char*)&logout_ret_data + sizeof(DataHeader), sizeof(LogoutRetData) - sizeof(DataHeader), 0);
		printf("�ǳ����: %d \n", logout_ret_data.ret);
	}
	break;

	case Cmd::kCmdNewUserJoin:
	{
		NewUserJoinData new_user_join_data;
		recv(sock, (char*)&new_user_join_data + sizeof(DataHeader), sizeof(NewUserJoinData) - sizeof(DataHeader), 0);
		printf("���û�����, socket : %d \n", new_user_join_data.sock);
	}
	break;

	default:
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
	SOCKET client_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client_sock == INVALID_SOCKET)
	{
		printf("����socketʧ��\n");
	}
	else
	{
		printf("����socket�ɹ��� socket : %d\n", int(socket));
	}

	// ���ӷ�����
	sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(sockaddr_in));
	client_addr.sin_family = PF_INET;
	client_addr.sin_addr.s_addr = inet_addr(kIp.c_str());
	client_addr.sin_port = htons(kPort);
	if (connect(client_sock, (sockaddr*)&client_addr, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		printf("���ӷ�����ʧ��\n");
	}
	else
	{
		printf("���ӷ������ɹ�\n");
	}

	while (true)
	{
		// ������socket����
		fd_set fd_read;

		// ��ռ���
		FD_ZERO(&fd_read);

		FD_SET(client_sock, &fd_read);

		int select_ret = select(client_sock, &fd_read, nullptr, nullptr, nullptr);
		if (select_ret < 0)
		{
			printf("select_ret < 0���������\n");
			break;
		}

		if (FD_ISSET(client_sock, &fd_read))
		{
			FD_CLR(client_sock, &fd_read);

			if (processor(client_sock) < 0)
			{
				printf("select �������\n");
				break;
			}
		}
	}

	/*while (true)
	{
		std::string input;
		std::cout << "���������" << std::endl;
		std::cin >> input;

		if (input == "login")
		{
			LoginData login_data;
			strcpy(login_data.user_name, "wangjunhe");
			strcpy(login_data.password, "123456");
			send(client_sock, (char*)&login_data, sizeof(LoginData), 0);

			LoginRetData login_ret_data = {};
			recv(client_sock, (char*)&login_ret_data, sizeof(LoginRetData), 0);
			printf("��¼���: %d \n", login_ret_data.ret);
		}
		else if(input == "logout")
		{
			LogoutData logout_data;
			strcpy(logout_data.user_name, "wangjunhe");
			send(client_sock, (char*)&logout_data, sizeof(LogoutData), 0);

			LogoutRetData logout_ret_data = {};
			recv(client_sock, (char*)&logout_ret_data, sizeof(LogoutRetData), 0);
			printf("�ǳ����: %d \n", logout_ret_data.ret);
		}
		else if (input == "exit")
		{
			printf("�������\n");
			break;
		}
		else
		{
			printf("������������������\n");
		}
	}*/

	// �ر��׽���
	closesocket(client_sock);

	WSACleanup(); // ��ֹ dll ��ʹ��

	return 0;
}