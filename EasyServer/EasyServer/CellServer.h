#ifndef __CELL_SERVER_H__
#define __CELL_SERVER_H__

#include "HeaderFile.h"
#include "Client.hpp"

// 网络事件接口
class INetEvent
{
public:
	// 纯虚函数
	virtual void OnJoin(Client* client) = 0;
	virtual void OnLeave(Client* client) = 0;
	virtual void OnNetMsg(Client* client, DataHeader* header) = 0;
	virtual void OnNetRecv(Client* client) = 0;
private:
};

class CellServer
{
public:
	CellServer(SOCKET sock, INetEvent* inet_event);

	virtual ~CellServer();

	void Start();
	void Close();

	int GetClientCount() const;

	bool OnRun();
	bool IsRun();
	int RecvData(Client* client);
	virtual int OnNetMsg(Client* client, DataHeader* header);
	virtual int SendData(DataHeader* data, SOCKET client_sock);
	virtual void SendData(DataHeader* data);
	void AddClient(Client* client);

private:
	SOCKET svr_sock_ = INVALID_SOCKET;
	char recv_data_buffer_[kRecvBufferSize] = {};
	std::unordered_map<SOCKET, Client*> client_map_ = {};
	std::vector<Client*> client_buff_vec_ = {};

	std::mutex client_mutex_;
	std::thread* work_thread_ = nullptr;
	INetEvent* inet_event_ = nullptr;

	fd_set fd_read_backup_;
	bool is_clients_change_ = false;
	SOCKET max_sock_ = 0;
};

#endif // !__CELL_SERVER_H__

