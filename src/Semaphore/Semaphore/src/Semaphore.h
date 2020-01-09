#pragma once
#include <condition_variable>
#include <mutex>
#include "SemaphoreMacro.h"

/** 信号类，包含信号量和事件
*/
class SemaphoreAPI Semaphore
{
public:
	/** 构造函数
	*/
	Semaphore();

public:
	/** 信号量的通知，通知几次wait允许通过几次，wait之前通知一次则wait执行时允许通过，一对一，不允许一对多
	*/
	void signal();

	/** 信号量的等待
	*/
	void wait();

	/** 事件通知，多个事件同时等待只会通知一个，必须进入了eventWait，通知才有效果，一对一，允许一对多
	*/
	void event();

	/** 事件通知，同时通知多个事件，必须进入了eventWait，通知才有效果，一对多
	*/
	void eventAll();

	/** 事件的等待
	*/
	void eventWait();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::mutex m_mtx;
	std::condition_variable m_cv;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_count;
};