#ifndef __CELL_TASK_H__
#define __CELL_TASK_H__

#include "HeaderFile.h"

// 任务基类
class CellTask
{
public:
	CellTask();
	virtual ~CellTask();

	// 执行任务
	virtual void DoTask();
private:
};

// 执行任务的服务类
class CellTaskServer
{
public:
	CellTaskServer();
	virtual ~CellTaskServer();

	void AddTask(std::shared_ptr<CellTask>& cell_task);

	void Start();

	void OnRun();

private:
	// 任务数据
	std::list<std::shared_ptr<CellTask>> task_list_ = {};

	// 任务数据缓冲区
	std::list<std::shared_ptr<CellTask>> task_buffer_list_ = {};

	// 改变任务数据缓冲区时需要加锁
	std::mutex task_mutex_;

	// 工作线程
	std::thread* task_thread_ = nullptr;
};


#endif // !__CELL_TASK_H__