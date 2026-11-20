#include "LockFreeHashMapTestThread.h"
#if defined(_WIN32)
#include <Windows.h>
#include <TlHelp32.h>
#endif

std::atomic<int> LockFreeHashMapTestThread::s_failures(0);

LockFreeHashMapTestThread::LockFreeHashMapTestThread()
{

}

LockFreeHashMapTestThread::LockFreeHashMapTestThread(LockFreeHashMapTestThread&& other) throw() :
m_thread(std::move(other.m_thread))
{

}

LockFreeHashMapTestThread& LockFreeHashMapTestThread::operator=(LockFreeHashMapTestThread&& other) throw()
{
    if (this != &other)
    {
        join();
        m_thread = std::move(other.m_thread);
    }
    return *this;
}

LockFreeHashMapTestThread::~LockFreeHashMapTestThread()
{
    join();
}

void LockFreeHashMapTestThread::join() throw()
{
    while (m_thread.joinable())
    {
        try
        {
            m_thread.join();
        }
        catch (...)
        {
            s_failures.fetch_add(1);
            // 不析构仍可连接的线程，也不放任引用夹具的线程脱离所有者
            // 正常外部所有者不会到此分支，系统异常由外层子进程超时判失败
            std::this_thread::yield();
        }
    }
}

int LockFreeHashMapTestThread::failures()
{
    return s_failures.load();
}

#if defined(_WIN32)
bool LockFreeHashMapTestThread::processThreadCount(unsigned long& count)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    const DWORD process = GetCurrentProcessId();
    THREADENTRY32 entry = {};
    entry.dwSize = sizeof(entry);
    unsigned long found = 0;
    bool complete = false;
    if (Thread32First(snapshot, &entry))
    {
        do
        {
            if (entry.th32OwnerProcessID == process)
            {
                ++found;
            }
            entry.dwSize = sizeof(entry);
        } while (Thread32Next(snapshot, &entry));
        complete = GetLastError() == ERROR_NO_MORE_FILES;
    }
    if (!CloseHandle(snapshot) || !complete || found == 0)
    {
        return false;
    }
    count = found;
    return true;
}
#endif

bool LockFreeHashMapTestThread::joinable() const
{
    return m_thread.joinable();
}