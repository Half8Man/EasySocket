#ifndef __CELL_THREAD_HPP__
#define __CELL_THREAD_HPP__

#include "CellSemaphore.hpp"

class CellThread
{
private:
	typedef std::function<void(CellThread *)> EventCall;

public:
	void Start(EventCall on_create = nullptr, EventCall on_run = nullptr, EventCall on_destory = nullptr)
	{
		std::lock_guard<std::mutex> lock(mutex_);

		if (!is_run_)
		{
			if (on_create)
			{
				on_create_ = on_create;
			}

			if (on_run)
			{
				on_run_ = on_run;
			}

			if (on_destory_)
			{
				on_destory_ = on_destory;
			}

			is_run_ = true;
			std::thread work_thread(std::mem_fn(&CellThread::OnWork), this);
			work_thread.detach();
		}
	}

	void Close()
	{
		std::lock_guard<std::mutex> lock(mutex_);

		if (is_run_)
		{
			is_run_ = false;
			sem_.Wait();
		}
	}

	// �ڱ��߳����˳�����Ҫʹ���ź��������ȴ���������������
	void Exit()
	{
		std::lock_guard<std::mutex> lock(mutex_);

		if (is_run_)
		{
			is_run_ = false;
		}
	}

	bool IsRun()
	{
		return is_run_;
	}

protected:
	void OnWork()
	{
		if (on_create_)
		{
			on_create_(this);
		}

		if (on_run_)
		{
			on_run_(this);
		}

		if (on_destory_)
		{
			on_destory_(this);
		}

		sem_.WakeUp();
	}

private:
	EventCall on_create_;
	EventCall on_run_;
	EventCall on_destory_;

	CellSemaphore sem_;
	std::mutex mutex_;
	bool is_run_ = false;
};

#endif // __CELL_THREAD_HPP__