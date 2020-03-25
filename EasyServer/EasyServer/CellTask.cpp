#include "CellTask.h"

CellTask::CellTask()
{
}

CellTask::~CellTask()
{
}

void CellTask::DoTask()
{
}

/**********************************************************************/

CellTaskServer::CellTaskServer()
{
	task_list_ = {};
	task_buffer_list_ = {};
	task_thread_ = nullptr;
}

CellTaskServer::~CellTaskServer()
{
	if (task_thread_)
	{
		delete task_thread_;
		task_thread_ = nullptr;
	}
}

void CellTaskServer::AddTask(std::shared_ptr<CellTask>& cell_task)
{
	std::lock_guard<std::mutex> lock(task_mutex_);

	task_buffer_list_.push_back(cell_task);
}

void CellTaskServer::Start()
{
	task_thread_ = new std::thread(std::mem_fn(&CellTaskServer::OnRun), this);
	task_thread_->detach();
}

void CellTaskServer::OnRun()
{
	while (true)
	{
		// �ӻ�����ȡ������
		if (!task_buffer_list_.empty())
		{
			std::lock_guard<std::mutex> lock(task_mutex_);
			for (auto& cell_task : task_buffer_list_)
			{
				task_list_.push_back(cell_task);
			}
			task_buffer_list_.clear();
		}

		// ���û������
		if (task_list_.empty())
		{
			std::chrono::milliseconds delay_time(1);
			std::this_thread::sleep_for(delay_time);

			continue;
		}

		// ��������
		for (auto& cell_task : task_list_)
		{
			cell_task->DoTask();

		}

		// �������
		task_list_.clear();
	}
}
