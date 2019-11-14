#pragma once
#include <condition_variable>
#include <mutex>
#include "SemaphoreMacro.h"

class SemaphoreAPI Semaphore
{
public:
	Semaphore();

public:
	void notify();

	void wait();

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