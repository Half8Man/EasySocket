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
	SOCKET svr_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	// ��socket
	sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin_family = PF_INET;
	sock_addr.sin_addr.s_addr = inet_addr(kIp.c_str());
	sock_addr.sin_port = htons(kPort);
	bind(svr_sock, (sockaddr*)&sock_addr, sizeof(sock_addr));

	// ����
	listen(svr_sock, 20);

	// ���տͻ�������
	sockaddr client_addr;
	int size = sizeof(client_addr);
	SOCKET client_sock = accept(svr_sock, (sockaddr*)&client_addr, &size);

	// ��ͻ��˷�������
	std::string str = "wangjunhe";
	send(client_sock, str.c_str(), strlen(str.c_str()) + sizeof(char), NULL);

	// �ر��׽���
	closesocket(client_sock);
	closesocket(svr_sock);

	WSACleanup(); // ��ֹ dll ��ʹ��
	return 0;
}