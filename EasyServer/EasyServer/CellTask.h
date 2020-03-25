#ifndef __CELL_TASK_H__
#define __CELL_TASK_H__

#include "HeaderFile.h"

// �������
class CellTask
{
public:
	CellTask();
	virtual ~CellTask();

	// ִ������
	virtual void DoTask();
private:
};

// ִ������ķ�����
class CellTaskServer
{
public:
	CellTaskServer();
	virtual ~CellTaskServer();

	void AddTask(std::shared_ptr<CellTask>& cell_task);

	void Start();

	void OnRun();

private:
	// ��������
	std::list<std::shared_ptr<CellTask>> task_list_ = {};

	// �������ݻ�����
	std::list<std::shared_ptr<CellTask>> task_buffer_list_ = {};

	// �ı��������ݻ�����ʱ��Ҫ����
	std::mutex task_mutex_;

	// �����߳�
	std::thread* task_thread_ = nullptr;
};


#endif // !__CELL_TASK_H__