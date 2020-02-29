#include "EasyTcpClient.h"

EasyTcpClient::EasyTcpClient()
	:client_sock_(INVALID_SOCKET), last_pos(0)
{
	memset(first_data_buffer_, 0, sizeof(first_data_buffer_));
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
		printf("�رվɵ�����\n");
		Close();
	}

#ifdef _WIN32
	// ����socket���绷��
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	(void)WSAStartup(version, &data);
#endif

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

int EasyTcpClient::Connect(const char* ip, unsigned short port)
{
	if (client_sock_ == INVALID_SOCKET)
		(void)InitSocket();

	// ���ӷ�����
	sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(sockaddr_in));
	client_addr.sin_family = PF_INET;
	client_addr.sin_addr.s_addr = inet_addr(ip);
	client_addr.sin_port = htons(port);

	auto ret = connect(client_sock_, (sockaddr*)&client_addr, sizeof(sockaddr_in));
	if (ret == SOCKET_ERROR)
		printf("���ӷ�����ʧ��\n");
	else
		printf("���ӷ������ɹ�\n");

	return ret;
}

void EasyTcpClient::Close()
{
#ifdef _WIN32
	// �ر��׽���
	closesocket(client_sock_);

	WSACleanup(); // ��ֹ dll ��ʹ��
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
	int len = recv(client_sock_, first_data_buffer_, kBufferSize, 0);
	if (len <= 0)
	{
		printf("���������ӶϿ����������\n");
		return -1;
	}

	// ����һ�����������ݿ������ڶ�������
	memcpy(second_data_buffer_ + last_pos, first_data_buffer_, len);

	// �ڶ���Ϣ����������β��λ�ú���
	last_pos += len;

	// �ж���Ϣ�����������ݳ����Ƿ������ϢͷDataHeader�ĳ���
	while (last_pos >= sizeof(DataHeader))
	{
		// ��ʱ��ͨ��header֪����Ϣ�ܳ���
		DataHeader* header = (DataHeader*)second_data_buffer_;

		// �ж���Ϣ�����������ݳ����Ƿ������Ϣ�ܳ���
		if (last_pos >= header->data_len)
		{
			// ʣ��δ�������Ϣ�������ĳ���
			int len = last_pos - header->data_len;

			// ������Ϣ
			DealMsg(header);

			// δ��������ǰ��
			memcpy(second_data_buffer_, first_data_buffer_ + header->data_len, len);
			last_pos = len;
		}
		else
		{
			// ʣ�����ݲ���һ����������Ϣ
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

	case Cmd::kCmdError:
	{
		printf("�յ��������Ϣ\n");
	}
	break;

	default:
	{
		printf("�յ�δ�������Ϣ\n");
	}
	break;
	}

	return 0;
}

int EasyTcpClient::OnRun()
{
	if (IsRun())
	{
		// ������socket����
		fd_set fd_read;

		// ��ռ���
		FD_ZERO(&fd_read);

		FD_SET(client_sock_, &fd_read);

		timeval t = { 0, 0 };
		int select_ret = select(client_sock_ + 1, &fd_read, nullptr, nullptr, &t);
		if (select_ret < 0)
		{
			printf("select_ret < 0���������\n");
			Close();
			return -1;
		}

		if (FD_ISSET(client_sock_, &fd_read))
		{
			FD_CLR(client_sock_, &fd_read);

			if (OnRecvData() < 0)
			{
				printf("select �������\n");
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
