﻿#ifndef __EASY_TCP_SERVER_H__
#define __EASY_TCP_SERVER_H__

#include "HeaderFile.h"
#include "CommonDef.h"
#include "Client.hpp"
#include "CellTimeStamp.hpp"
#include "CellServer.h"

class Client;
class CellTimeStamp;
class CellServer;

class EasyTcpServer : public INetEvent
{
public:
	EasyTcpServer();
	virtual ~EasyTcpServer();

	SOCKET InitSock();
	int Bind(const char *ip, unsigned short port);
	int Listen(int count);
	SOCKET Accept();
	void Start(int cell_server_count);
	void AddClient2CellServer(Client *client);
	void Close();

	void OnRun(CellThread *cell_thread);
	bool IsRun();

	void Time4Pkg();
	virtual void OnJoin(Client *client);
	virtual void OnLeave(Client *client);
	virtual void OnNetMsg(CellServer *cell_svr, Client *client, DataHeader *header);
	virtual void OnNetRecv(Client *client);

private:
	SOCKET svr_sock_ = INVALID_SOCKET;
	std::vector<CellServer *> cell_server_vec_ = {};
	std::atomic_int msg_count_;
	std::atomic_int recv_count_;
	std::atomic_int client_count_;
	CellTimeStamp time_stamp_;

	CellThread *cell_thread_ = nullptr;
};

#endif // !__EASY_TCP_SERVER_H__
