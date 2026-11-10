#include "LockFreeMapTestThread.h"

std::atomic<size_t> LockFreeMapTestThread::m_errors(0);

LockFreeMapTestThread::LockFreeMapTestThread() :
m_handle(),
m_joinable(false)
{

}

LockFreeMapTestThread::LockFreeMapTestThread(LockFreeMapTestThread&& other) throw() :
m_handle(other.m_handle),
m_joinable(other.m_joinable)
{
    other.m_joinable = false;
}

LockFreeMapTestThread& LockFreeMapTestThread::operator=(LockFreeMapTestThread&& other) throw()
{
    if (this != &other)
    {
        join();
        m_handle = other.m_handle;
        m_joinable = other.m_joinable;
        other.m_joinable = false;
    }
    return *this;
}

LockFreeMapTestThread::~LockFreeMapTestThread()
{
    join();
}

bool LockFreeMapTestThread::start(std::function<void()>* function)
{
#ifdef _WIN32
    m_handle = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, &entry, function, 0, nullptr));
    m_joinable = m_handle != nullptr;
#else
    m_joinable = pthread_create(&m_handle, nullptr, &entry, function) == 0;
#endif
    return m_joinable;
}

#ifdef _WIN32
unsigned __stdcall LockFreeMapTestThread::entry(void* context)
#else
void* LockFreeMapTestThread::entry(void* context)
#endif
{
    std::function<void()>* function = static_cast<std::function<void()>*>(context);
    try
    {
        (*function)();
    }
    catch (...)
    {
        m_errors.fetch_add(1);
    }
    delete function;
    return 0;
}

bool LockFreeMapTestThread::join()
{
    if (!m_joinable)
    {
        return true;
    }
#ifdef _WIN32
    const bool waited = WaitForSingleObject(m_handle, INFINITE) == WAIT_OBJECT_0;
    const bool closed = CloseHandle(m_handle) != 0;
    const bool passed = waited && closed;
#else
    const bool passed = pthread_join(m_handle, nullptr) == 0;
#endif
    m_joinable = false;
    if (!passed)
    {
        m_errors.fetch_add(1);
    }
    return passed;
}

LockFreeMapTestThreadGroup::LockFreeMapTestThreadGroup(std::vector<LockFreeMapTestThread>& threads, std::atomic<bool>* gate) :
m_threads(&threads),
m_gate(gate)
{

}

LockFreeMapTestThreadGroup::~LockFreeMapTestThreadGroup()
{
    if (m_gate != nullptr)
    {
        m_gate->store(true);
    }
    for (size_t index = 0; index < m_threads->size(); ++index)
    {
        (*m_threads)[index].join();
    }
}