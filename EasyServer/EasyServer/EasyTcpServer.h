#ifndef __EASY_TCP_SERVER_H__
#define __EASY_TCP_SERVER_H__

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define FD_SETSIZE 1024

	#include<windows.h>
	#include<WinSock2.h>
	#pragma comment(lib,"ws2_32.lib")
#else
	#include<unistd.h>
	#include<arpa/inet.h>
	#include<string.h>

	typedef int SOCKET
	#define INVALID_SOCKET (int)(~0)
	#define SOCKET_ERROR (-1)
#endif

#include <stdio.h>
#include <vector>

#include "CommonDef.h"
#include "Client.hpp"

class Client;

class EasyTcpServer
{
public:
	EasyTcpServer();
	virtual ~EasyTcpServer();
	
	SOCKET InitSock();
	int Bind(const char* ip, unsigned short port);
	int Listen(int count);
	SOCKET Accept();
	void Close();

	bool OnRun();
	bool IsRun();
	int RecvData(Client* client);
	virtual int OnNetMsg(SOCKET client_sock, DataHeader* header);
	virtual int SendData(DataHeader *data, SOCKET client_sock);
	virtual void SendData(DataHeader* data);

private:
	SOCKET svr_sock_ = INVALID_SOCKET;
	char data_buffer_[kBufferSize] = {};
	std::vector<Client*> client_vec_ = {};
};

#endif // !__EASY_TCP_SERVER_H__
