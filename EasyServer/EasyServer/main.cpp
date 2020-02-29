#include "EasyTcpServer.h"

const char* kIp = "127.0.0.1";
const int kPort = 1234;

int main()
{
	EasyTcpServer server;

	server.InitSock();
	server.Bind(kIp, kPort);
	server.Listen(20);

	while (server.IsRun())
		server.OnRun();

	server.Close();

	return 0;
}