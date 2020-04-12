﻿#ifndef __CELL_TASK_H__
#define __CELL_TASK_H__

#include "HeaderFile.h"
#include "CellSemaphore.hpp"

// 执行任务的服务类
class CellTaskServer
{
	typedef std::function<void()> CellTask;

public:
	CellTaskServer();
	virtual ~CellTaskServer();

	void AddTask(CellTask cell_task);

	void Start();
	void Close();

	void OnRun();

private:
	// 任务数据
	std::list<CellTask> task_list_ = {};

	// 任务数据缓冲区
	std::list<CellTask> task_buffer_list_ = {};

	// 改变任务数据缓冲区时需要加锁
	std::mutex task_mutex_;

	// 工作线程
	std::thread *task_thread_ = nullptr;

	bool is_run_ = false;
	CellSemaphore cell_sem_;
};

#endif // !__CELL_TASK_H__