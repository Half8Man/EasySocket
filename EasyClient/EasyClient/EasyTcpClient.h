#ifndef __EASY_TCP_CLIENT__
#define __EASY_TCP_CLIENT__

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	//#define _CRT_SECURE_NO_WARNINGS
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

#include <iostream>
#include <string>
#include <stdio.h>

#include "CommonDef.h"

class EasyTcpClient
{
public:
	EasyTcpClient();
	virtual ~EasyTcpClient();

	// 初始化socket
	SOCKET InitSocket();

	// 连接服务端
	int Connect(const char* ip, unsigned short port);

	// 关闭socket;
	void Close();

	// 发送数据
	int SendData(DataHeader* data);

	// 接受数据，处理粘包，拆分包
	int OnRecvData();

	int DealMsg(DataHeader* header);

	// 处理网络消息
	int OnRun();

	// 是否工作
	bool IsRun();
private:
	SOCKET client_sock_;

	// 数据缓冲区
	char second_data_buffer_[kBufferSize] = {};

	int last_pos = 0;
};

#endif // __EASY_TCP_CLIENT__

