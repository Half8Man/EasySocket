#include "CellTask.h"

CellTaskServer::CellTaskServer()
	:task_list_({}), task_buffer_list_({}), cell_thread_(nullptr)
{
}

CellTaskServer::~CellTaskServer()
{
	if (cell_thread_)
	{
		delete cell_thread_;
		cell_thread_ = nullptr;
	}
}

void CellTaskServer::AddTask(CellTask cell_task)
{
	std::lock_guard<std::mutex> lock(task_mutex_);

	task_buffer_list_.push_back(cell_task);
}

void CellTaskServer::Start()
{
	if (!cell_thread_)
	{
		cell_thread_ = new CellThread;
		if (cell_thread_)
		{
			cell_thread_->Start(
				nullptr,
				[this](CellThread* cell_thread)
				{
					OnRun(cell_thread);
				},
				nullptr
			);
		}
	} 
}

void CellTaskServer::Close()
{
	printf("%s start\n", __FUNCTION__);

	if (cell_thread_)
	{
		cell_thread_->Close();
	}

	printf("%s end\n", __FUNCTION__);
}


void CellTaskServer::OnRun(CellThread* cell_thread)
{
	while (cell_thread->IsRun())
	{
		// 从缓冲区取出数据
		if (!task_buffer_list_.empty())
		{
			std::lock_guard<std::mutex> lock(task_mutex_);
			for (auto &cell_task : task_buffer_list_)
			{
				task_list_.push_back(cell_task);
			}
			task_buffer_list_.clear();
		}

		// 如果没有任务
		if (task_list_.empty())
		{
			std::chrono::milliseconds delay_time(1);
			std::this_thread::sleep_for(delay_time);

			continue;
		}

		// 处理任务
		for (auto &cell_task : task_list_)
		{
			cell_task();
		}

		// 清空任务
		task_list_.clear();
	}
}
