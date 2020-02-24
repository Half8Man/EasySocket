#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <string>
#include <windows.h>
#include <WinSock2.h>

const std::string kIp = "127.0.0.1";
const int kPort = 1234;

int main()
{
	// ����socket���绷��
	WORD version = MAKEWORD(2, 2);
	WSADATA data;
	(void)WSAStartup(version, &data);

	// ����socket�׽���
	SOCKET client_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	// ���ӷ�����
	sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = PF_INET;
	client_addr.sin_addr.s_addr = inet_addr(kIp.c_str());
	client_addr.sin_port = htons(kPort);
	connect(client_sock, (sockaddr*)&client_addr, sizeof(client_addr));

	// ���շ��������
	char buffer[MAXBYTE] = { 0 };
	recv(client_sock, buffer, MAXBYTE, NULL);
	std::cout << buffer << std::endl;

	// �ر��׽���
	closesocket(client_sock);

	WSACleanup(); // ��ֹ dll ��ʹ��

	system("pause");
	return 0;
}