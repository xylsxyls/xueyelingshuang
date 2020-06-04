#include "Semaphore.h"

Semaphore::Semaphore():
#ifdef _MSC_VER
m_processHandle(nullptr),
#endif
m_count(0)
{

}

void Semaphore::signal()
{
	std::unique_lock<std::mutex> lock(m_mtx);
	++m_count;
	m_cv.notify_one();
}

void Semaphore::wait()
{
	std::unique_lock<std::mutex> lock(m_mtx);
	while (m_count == 0)
	{
		m_cv.wait(lock);
	}
	--m_count;
}

#ifdef _MSC_VER

void Semaphore::createProcessSemaphore(const std::string& name, int32_t signalCount)
{
	if (m_processHandle != nullptr)
	{
		return;
	}
	m_processHandle = ::CreateSemaphore(nullptr, 0, signalCount, name.c_str());
}

void Semaphore::openProcessSemaphore(const std::string& name)
{
	if (m_processHandle != nullptr)
	{
		return;
	}
	m_processHandle = ::OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, name.c_str());
}

void Semaphore::closeProcessSemaphore()
{
	if (m_processHandle == nullptr)
	{
		return;
	}
	::CloseHandle(m_processHandle);
	m_processHandle = nullptr;
}

void Semaphore::processSignal()
{
	if (m_processHandle == nullptr)
	{
		return;
	}
	::ReleaseSemaphore(m_processHandle, 1, nullptr);
}

void Semaphore::processWait()
{
	if (m_processHandle == nullptr)
	{
		return;
	}
	::WaitForSingleObject(m_processHandle, INFINITE);
}

#endif

void Semaphore::event()
{
	std::unique_lock<std::mutex> lock(m_mtx);
	m_cv.notify_one();
}

void Semaphore::eventAll()
{
	std::unique_lock<std::mutex> lock(m_mtx);
	m_cv.notify_all();
}

void Semaphore::eventWait()
{
	std::unique_lock<std::mutex> lock(m_mtx);
	m_cv.wait(lock);
}

void Semaphore::eventWait(int32_t timeOut)
{
	std::unique_lock<std::mutex> lock(m_mtx);
	m_cv.wait_for(lock, std::chrono::milliseconds(timeOut));
}