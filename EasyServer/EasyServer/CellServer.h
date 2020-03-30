#ifndef __CELL_SERVER_H__
#define __CELL_SERVER_H__

#include "HeaderFile.h"
#include "Client.hpp"
#include "CellTask.h"

class CellServer;

// �����¼��ӿ�
class INetEvent
{
public:
	// ���麯��
	virtual void OnJoin(Client* client) = 0;
	virtual void OnLeave(Client* client) = 0;
	virtual void OnNetMsg(CellServer* cell_svr, Client* client, DataHeader* header) = 0;
	virtual void OnNetRecv(Client* client) = 0;
private:
};

class CellSendMsg2ClientTask :public CellTask
{
public:
	CellSendMsg2ClientTask(Client* client, DataHeader* data);
	~CellSendMsg2ClientTask();

	// ִ������
	void DoTask();
private:
	Client* client_;
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
	int RecvData(Client* client);
	virtual int OnNetMsg(CellServer* cell_svr, Client* client, DataHeader* header);
	virtual int SendData(DataHeader* data, SOCKET client_sock);
	virtual void SendData(DataHeader* data);
	void AddClient(Client* client);

	void AddSendTask(Client* client, DataHeader* data);

private:
	SOCKET svr_sock_ = INVALID_SOCKET;	// socket
	std::unordered_map<SOCKET, Client*> client_map_ = {}; // ��ʽ�ͻ���map
	std::vector<Client*> client_buff_vec_ = {}; // ����ͻ���vector

	std::mutex client_mutex_; // ������е���
	std::thread* work_thread_ = nullptr; // �����߳�
	INetEvent* inet_event_ = nullptr; // �����¼�����

	fd_set fd_read_backup_;
	bool is_clients_change_ = false;
	SOCKET max_sock_ = 0;

	CellTaskServer* cell_task_svr_ = nullptr;
};

#endif // !__CELL_SERVER_H__

