#include "Semaphore.h"
#ifdef __unix__
#include <fcntl.h>
#endif

Semaphore::Semaphore():
m_processSemaphore(nullptr),
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

void Semaphore::createProcessSemaphore(const std::string& name, int32_t signalCount)
{
	if (m_processSemaphore != nullptr)
	{
		return;
	}
#ifdef _MSC_VER
	m_processSemaphore = ::CreateSemaphore(nullptr, 0, signalCount, name.c_str());
#elif __unix__
	m_processSemaphore = ::sem_open(name.c_str(), O_CREAT, 0644, 0);
	m_name = name;
#endif
}

void Semaphore::openProcessSemaphore(const std::string& name)
{
	if (m_processSemaphore != nullptr)
	{
		return;
	}
#ifdef _MSC_VER
	m_processSemaphore = ::OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, name.c_str());
#elif __unix__
	m_processSemaphore = ::sem_open(name.c_str(), O_CREAT, 0644, 0);
#endif
}

void Semaphore::closeProcessSemaphore()
{
	if (m_processSemaphore != nullptr)
	{
		return;
	}
#ifdef _MSC_VER
	::CloseHandle(m_processSemaphore);
#elif __unix__
	::sem_close(m_processSemaphore);
	if (!m_name.empty())
	{
		::sem_unlink(m_name.c_str());
		m_name.clear();
	}
#endif
	m_processSemaphore = nullptr;
}

void Semaphore::processSignal()
{
	if (m_processSemaphore == nullptr)
	{
		return;
	}
#ifdef _MSC_VER
	::ReleaseSemaphore(m_processSemaphore, 1, nullptr);
#elif __unix__
	::sem_post(m_processSemaphore);
#endif
	
}

void Semaphore::processWait()
{
	if (m_processSemaphore == nullptr)
	{
		return;
	}
#ifdef _MSC_VER
	::WaitForSingleObject(m_processSemaphore, INFINITE);
#elif __unix__
	::sem_wait(m_processSemaphore);
#endif
}

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