#include "EasyTcpServer.hpp"

const std::string kIp = "127.0.0.1";
const int kPort = 1234;

int main()
{
	EasyTcpServer server;
	server.InitSocket();
	server.Bind(kIp.c_str(), kPort);
	server.Listen(20);
	
	while (server.IsAlive())
	{
		server.OnRun();
	}

	server.Close();

	return 0;
}