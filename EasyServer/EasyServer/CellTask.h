#ifndef __CELL_TASK_H__
#define __CELL_TASK_H__

#include "HeaderFile.h"

// ִ������ķ�����
class CellTaskServer
{
	typedef std::function<void()> CellTask;
public:
	CellTaskServer();
	virtual ~CellTaskServer();

	void AddTask(CellTask cell_task);

	void Start();

	void OnRun();

private:
	// ��������
	std::list<CellTask> task_list_ = {};

	// �������ݻ�����
	std::list<CellTask> task_buffer_list_ = {};

	// �ı��������ݻ�����ʱ��Ҫ����
	std::mutex task_mutex_;

	// �����߳�
	std::thread* task_thread_ = nullptr;
};


#endif // !__CELL_TASK_H__