#ifndef __EASY_TCP_SERVER_H__
#define __EASY_TCP_SERVER_H__

#include "HeaderFile.h"
#include "CommonDef.h"
#include "Client.hpp"
#include "CELLTimeStamp.hpp"
#include "CellServer.h"

class Client;
class CELLTimeStamp;
class CellServer;

class EasyTcpServer : public INetEvent
{
public:
	EasyTcpServer();
	virtual ~EasyTcpServer();
	
	SOCKET InitSock();
	int Bind(const char* ip, unsigned short port);
	int Listen(int count);
	SOCKET Accept();
	void Start(int cell_server_count);
	void AddClient2CellServer(std::shared_ptr<Client> client);
	void Close();

	bool OnRun();
	bool IsRun();

	void Time4Pkg();
	virtual void OnJoin(std::shared_ptr<Client>& client);
	virtual void OnLeave(std::shared_ptr<Client>& client);
	virtual void OnNetMsg(CellServer* cell_svr, std::shared_ptr<Client>& client, DataHeader* header);
	virtual void OnNetRecv(std::shared_ptr<Client>& client);

private:
	SOCKET svr_sock_ = INVALID_SOCKET;
	std::vector<std::shared_ptr<CellServer>> cell_server_vec_ = {};
	std::atomic_int msg_count_ = 0;
	std::atomic_int recv_count_ = 0;
	std::atomic_int client_count_ = 0;
	CELLTimeStamp time_stamp_;
};

#endif // !__EASY_TCP_SERVER_H__
