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
	char data_buffer_[kBufferSize] = {};
	std::vector<Client*> client_vec_ = {};
	std::vector<Client*> client_buff_vec_ = {};

	std::mutex client_mutex_;
	std::thread* work_thread_ = nullptr;
	INetEvent* inet_event_ = nullptr;
};

#endif // !__CELL_SERVER_H__

