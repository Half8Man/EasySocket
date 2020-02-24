#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <string>
#include <windows.h>
#include <WinSock2.h>

const std::string kIp = "127.0.0.1";
const int kPort = 1234;

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
	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin_family = PF_INET;
	sock_addr.sin_addr.s_addr = inet_addr(kIp.c_str());
	sock_addr.sin_port = htons(kPort);
	bind(svr_sock, (sockaddr*)&sock_addr, sizeof(sock_addr));

	// 监听
	listen(svr_sock, 20);

	// 接收客户端请求
	sockaddr client_addr;
	int size = sizeof(client_addr);
	SOCKET client_sock = accept(svr_sock, (sockaddr*)&client_addr, &size);

	// 向客户端发送数据
	std::string str = "wangjunhe";
	send(client_sock, str.c_str(), strlen(str.c_str()) + sizeof(char), NULL);

	// 关闭套接字
	closesocket(client_sock);
	closesocket(svr_sock);

	WSACleanup(); // 终止 dll 的使用
	return 0;
}