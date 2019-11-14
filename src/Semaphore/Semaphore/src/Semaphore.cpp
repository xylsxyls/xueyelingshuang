#include "Semaphore.h"

Semaphore::Semaphore():
m_count(0)
{

}

void Semaphore::notify()
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
	m_count--;
}
