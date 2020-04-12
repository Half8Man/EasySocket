#include "EasyTcpServer.h"

const char *kIp = "127.0.0.1";
const int kPort = 2234;

class EasyTcpServer;

bool is_run = true;
void CmdThread()
{
	char input;
	std::cin >> input;
	is_run = false;
}

int main()
{
	EasyTcpServer server;

	server.InitSock();
	server.Bind(kIp, kPort);
	server.Listen(20);
	server.Start(kCellServerCount);

	std::thread input_thread(CmdThread);
	input_thread.detach();

	while (is_run)
		server.OnRun();

	server.Close();

	return 0;
}