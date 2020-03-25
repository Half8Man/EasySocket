#ifndef __HEADER_FILE_H__
#define __HEADER_FILE_H__

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

// cpp
#include <unordered_map>
#include <vector>
#include <list>
#include <thread>
#include <chrono>
#include <mutex>
#include <functional>
#include <atomic>
#include <memory>

// c
#include <stdio.h>

#endif // !__HEADER_FILE_H__

