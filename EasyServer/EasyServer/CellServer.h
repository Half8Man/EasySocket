#ifndef __CELL_SERVER_H__
#define __CELL_SERVER_H__

#include "HeaderFile.h"
#include "Client.hpp"
#include "CellTask.h"

class CellServer;

// 网络事件接口
class INetEvent
{
public:
	// 纯虚函数
	virtual void OnJoin(std::shared_ptr<Client>& client) = 0;
	virtual void OnLeave(std::shared_ptr<Client>& client) = 0;
	virtual void OnNetMsg(CellServer* cell_svr, std::shared_ptr<Client>& client, DataHeader* header) = 0;
	virtual void OnNetRecv(std::shared_ptr<Client>& client) = 0;
private:
};

class CellSendMsg2ClientTask :public CellTask
{
public:
	CellSendMsg2ClientTask(std::shared_ptr<Client> client, DataHeader* data);
	~CellSendMsg2ClientTask();

	// 执行任务
	void DoTask();
private:
	std::shared_ptr<Client> client_;
	DataHeader* data_;
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
	int RecvData(std::shared_ptr<Client> client);
	virtual int OnNetMsg(CellServer* cell_svr, std::shared_ptr<Client> client, DataHeader* header);
	virtual int SendData(DataHeader* data, SOCKET client_sock);
	virtual void SendData(DataHeader* data);
	void AddClient(std::shared_ptr<Client> client);

	void AddSendTask(std::shared_ptr<Client> client, DataHeader* data);

private:
	SOCKET svr_sock_ = INVALID_SOCKET;	// socket
	std::unordered_map<SOCKET, std::shared_ptr<Client>> client_map_ = {}; // 正式客户端map
	std::vector<std::shared_ptr<Client>> client_buff_vec_ = {}; // 缓冲客户端vector

	std::mutex client_mutex_; // 缓冲队列的锁
	std::thread* work_thread_ = nullptr; // 工作线程
	INetEvent* inet_event_ = nullptr; // 网络事件对象

	fd_set fd_read_backup_;
	bool is_clients_change_ = false;
	SOCKET max_sock_ = 0;

	CellTaskServer* cell_task_svr_ = nullptr;
};

#endif // !__CELL_SERVER_H__

