#include "Semaphore.h"
#ifdef __unix__
#include <sys/sem.h>
#include <thread>
#endif

#ifdef __unix__
union semun
{
	int val;
	struct semid_ds* buf;
	unsigned short* arry;
};
#endif

Semaphore::Semaphore():
#ifdef _MSC_VER
m_processSemaphore(nullptr),
#elif __unix__
m_processSemaphoreId(-1),
#endif
m_count(0)
{

}

Semaphore::~Semaphore()
{
	closeProcessSemaphore();
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

bool Semaphore::wait(int32_t timeout)
{
	std::unique_lock<std::mutex> lock(m_mtx);
	while (m_count == 0)
	{
		if (m_cv.wait_for(lock, std::chrono::milliseconds(timeout)) == std::cv_status::timeout)
		{
			return false;
		}
	}
	--m_count;
	return true;
}

void Semaphore::createProcessSemaphore(const std::string& name, int32_t signalCount)
{
#ifdef _MSC_VER
	if (m_processSemaphore != nullptr)
	{
		return;
	}
	m_processSemaphore = ::CreateSemaphore(nullptr, 0, signalCount, name.c_str());
#elif __unix__
	if (m_processSemaphoreId != -1)
	{
		return;
	}
	std::string fileName = "/dev/shm/Semaphore." + name;
	system(("touch \"" + fileName + "\"").c_str());
	m_processSemaphoreId = semget(ftok(fileName.c_str(), 0), 1, 0644 | IPC_CREAT);
	union semun sem_union;
	sem_union.val = 0;
	if(semctl(m_processSemaphoreId, 0, SETVAL, sem_union) == -1)
	{
		printf("create semphore failed, name = %s, Id = %d\n", name.c_str(), m_processSemaphoreId);
	}
	m_name = name;
#endif
}

void Semaphore::openProcessSemaphore(const std::string& name)
{
#ifdef _MSC_VER
	if (m_processSemaphore != nullptr)
	{
		return;
	}
	m_processSemaphore = ::OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, name.c_str());
#elif __unix__
	if (m_processSemaphoreId != -1)
	{
		return;
	}
	key_t key = ftok(("/dev/shm/Semaphore." + name).c_str(), 0);
	if (key == -1)
	{
		return;
	}
	m_processSemaphoreId = semget(key, 1, 0644 | IPC_CREAT);
#endif
}

void Semaphore::closeProcessSemaphore()
{
#ifdef _MSC_VER
	if (m_processSemaphore == nullptr)
	{
		return;
	}
	::CloseHandle(m_processSemaphore);
	m_processSemaphore = nullptr;
#elif __unix__
	if (m_name.empty())
	{
		m_processSemaphoreId = -1;
		return;
	}
	if (m_processSemaphoreId == -1)
	{
		return;
	}
	//删除信号量
	union semun sem_union;
	if(semctl(m_processSemaphoreId, 0, IPC_RMID, sem_union) == -1)
	{
		printf("Failed to delete semaphore, name = %s, Id = %d\n", m_name.c_str(), m_processSemaphoreId);
	}
	m_processSemaphoreId = -1;
	system(("rm \"/dev/shm/Semaphore." + m_name + "\"").c_str());
#endif
}

void Semaphore::processSignal()
{
#ifdef _MSC_VER
	if (m_processSemaphore == nullptr)
	{
		return;
	}
	::ReleaseSemaphore(m_processSemaphore, 1, nullptr);
#elif __unix__
	if (m_processSemaphoreId == -1)
	{
		return;
	}
	struct sembuf sem;
	sem.sem_num = 0;
	sem.sem_op = 1;
	sem.sem_flg = 0;
	if(semop(m_processSemaphoreId, &sem, 1) == -1)
	{
		std::thread::id threadId = std::this_thread::get_id();
		int32_t currentThreadId = (int32_t)(uint32_t)(*(__gthread_t*)(char*)(&threadId));
		printf("processSignal failed, errno = %d, name = %s, Id = %d, threadId = %d\n", errno, m_name.c_str(), m_processSemaphoreId, currentThreadId);
	}
#endif
}

void Semaphore::processWait()
{
#ifdef _MSC_VER
	if (m_processSemaphore == nullptr)
	{
		return;
	}
	::WaitForSingleObject(m_processSemaphore, INFINITE);
#elif __unix__
	if (m_processSemaphoreId == -1)
	{
		return;
	}
	struct sembuf sem;
	sem.sem_num = 0;
	sem.sem_op = -1;
	sem.sem_flg = 0;
	int32_t result = 0;
	while((result = semop(m_processSemaphoreId, &sem, 1)) == -1 && errno == EINTR);
	if (result == 0)
	{
		return;
	}
	std::thread::id threadId = std::this_thread::get_id();
	int32_t currentThreadId = (int32_t)(uint32_t)(*(__gthread_t*)(char*)(&threadId));
	printf("processWait failed, result = %d, errno = %d, name = %s, Id = %d, threadId = %d\n", result, errno, m_name.c_str(), m_processSemaphoreId, currentThreadId);
#endif
}

bool Semaphore::processWait(int32_t timeout)
{
#ifdef _MSC_VER
	if (m_processSemaphore == nullptr)
	{
		return false;
	}
	DWORD dwResult = ::WaitForSingleObject(m_processSemaphore, timeout);
	if (dwResult == WAIT_OBJECT_0)
	{
		return true;
	}
	else if (dwResult == WAIT_TIMEOUT)
	{
		return false;
	}
	return false;
#elif __unix__
	if (m_processSemaphoreId == -1)
	{
		return false;
	}
	struct sembuf sem;
	sem.sem_num = 0;
	sem.sem_op = -1;
	sem.sem_flg = 0;
	struct timespec ts;
	ts.tv_sec = timeout / 1000;
	ts.tv_nsec = (timeout % 1000) * 1000 * 1000;
	int32_t result = 0;
	while ((result = semtimedop(m_processSemaphoreId, &sem, 1, &ts)) == -1 && errno == EINTR);
	//errno只有在错误时才会更新
	if (result == 0)
	{
		return true;
	}
	//超时
	else if (result == -1 && errno == EAGAIN)
	{
		return false;
	}
	std::thread::id threadId = std::this_thread::get_id();
	int32_t currentThreadId = (int32_t)(uint32_t)(*(__gthread_t*)(char*)(&threadId));
	printf("semtimedop failed, result = %d, errno = %d, name = %s, Id = %d, threadId = %d\n", result, errno, m_name.c_str(), m_processSemaphoreId, currentThreadId);
	return false;
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

bool Semaphore::eventWait(int32_t timeout)
{
	std::unique_lock<std::mutex> lock(m_mtx);
	if (m_cv.wait_for(lock, std::chrono::milliseconds(timeout)) == std::cv_status::timeout)
	{
		return false;
	}
	return true;
}