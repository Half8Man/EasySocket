#ifndef __CELL_TIME_STAMP_H__
#define __CELL_TIME_STAMP_H__

#include "HeaderFile.h"

using namespace std::chrono;

class CellTime
{
public:
	// 获取当前时间戳 毫秒
	static time_t GetCurTimeMilliSec()
	{
		auto now = high_resolution_clock::now();

		return duration_cast<milliseconds>(now.time_since_epoch()).count();
	}
};

class CELLTimeStamp
{
public:
	CELLTimeStamp()
	{
		Update();
	}

	virtual ~CELLTimeStamp()
	{
	}

	// 更新
	void Update()
	{
		begin_ = high_resolution_clock::now();
	}

	// 获取当前秒
	double GetElapsedSecond()
	{
		return GetElapsedTimeInMicroSec() * 0.000001;
	}

	// 获取当前毫秒
	double GetElapsedTimeInMilliSec()
	{
		return GetElapsedTimeInMicroSec() * 0.001;
	}

	// 获取当前微秒
	long long GetElapsedTimeInMicroSec()
	{
		auto temp = high_resolution_clock::now() - begin_;
		return duration_cast<microseconds>(temp).count();
	}

private:
	time_point<high_resolution_clock> begin_; // 高分辨率时钟
};

#endif // __CELL_TIME_STAMP_H__
