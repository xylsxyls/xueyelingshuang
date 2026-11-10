#include "LockFreeHashMapThreadControl.h"
#include "LockFreeHashMapTestThread.h"
#include <thread>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#if defined(_WIN32)
#include <Windows.h>
#include <TlHelp32.h>
#endif

bool LockFreeHashMapThreadControl::s_tracking = false;
LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<long long> LockFreeHashMapThreadControl::s_live(0);

/** 追踪本测试进程的实际申请，故障注入只属于测试程序
@param [in] size 请求字节数，先检查额外计账空间的溢出
@return 正确对齐的申请地址，失败抛 bad_alloc 供调用边界处理
*/
void* operator new(size_t size)
{
    if (size > (std::numeric_limits<size_t>::max)() - 32)
    {
        throw std::bad_alloc();
    }
    unsigned char* allocation = static_cast<unsigned char*>(std::malloc(size + 32));
    if (allocation == nullptr)
    {
        throw std::bad_alloc();
    }
    allocation[0] = LockFreeHashMapThreadControl::s_tracking ? 1 : 0;
    if (allocation[0] != 0)
    {
        LockFreeHashMapThreadControl::s_live.fetch_add(1);
    }
    return allocation + 32;
}

/** 释放配对申请并精确撤销计账
@param [in] pointer 对应测试申请地址，可为空，载荷必须已析构
*/
void operator delete(void* pointer) throw()
{
    if (pointer != nullptr)
    {
        unsigned char* allocation = static_cast<unsigned char*>(pointer) - 32;
        if (allocation[0] != 0)
        {
            LockFreeHashMapThreadControl::s_live.fetch_sub(1);
        }
        std::free(allocation);
    }
}

/** 追踪本测试进程的实际申请，故障注入只属于测试程序
@param [in] size 请求字节数，先检查额外计账空间的溢出
@return 正确对齐的申请地址，失败抛 bad_alloc 供调用边界处理
*/
void* operator new[](size_t size)
{
    return ::operator new(size);
}

/** 释放配对申请并精确撤销计账
@param [in] pointer 对应测试申请地址，可为空，载荷必须已析构
*/
void operator delete[](void* pointer) throw()
{
    ::operator delete(pointer);
}

void LockFreeHashMapThreadControl::worker(bool standard)
{
    for (int i = 0; i < 16; ++i)
    {
#if defined(_WIN32)
        if (!standard)
        {
            SwitchToThread();
            continue;
        }
#endif
        std::this_thread::yield();
    }
#if defined(_WIN32)
    if (!standard)
    {
        Sleep(2);
        return;
    }
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
}

/** 执行独立测试入口，参数错误和未预期异常均返回非零
@param [in] argc 参数数量
@param [in] argv 参数数组，具体选择见测试文档
@return 0 完成并通过，非零表示输入、校验或设施失败
*/
int main(int argc, char** argv)
{
    std::cout << "THREAD_CONTROL warm_numeric=" << 0.0 << 0LL << 0UL << std::endl;
    LockFreeHashMapThreadControl::s_tracking = true;
    bool standard = argc > 1 && std::strcmp(argv[1], "standard") == 0;
    if (argc > 2 || (argc == 2 && !standard))
    {
        return 1;
    }
    try
    {
#if defined(_WIN32)
        for (int round = 0; round <= 10; ++round)
        {
            if (round != 0)
            {
                std::vector<LockFreeHashMapTestThread> threads;
                for (int i = 0; i < 8; ++i)
                {
                    // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
                    threads.push_back(LockFreeHashMapTestThread([standard]()
                    {
                        LockFreeHashMapThreadControl::worker(standard);
                    }));
                }
                for (size_t i = 0; i < threads.size(); ++i)
                {
                    threads[i].join();
                }
            }
            DWORD handles = 0;
            if (!GetProcessHandleCount(GetCurrentProcess(), &handles))
            {
                return 2;
            }
            DWORD count = 0;
            HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (snapshot == INVALID_HANDLE_VALUE)
            {
                return 3;
            }
            THREADENTRY32 entry = {};
            entry.dwSize = sizeof(entry);
            if (Thread32First(snapshot, &entry))
            {
                do
                {
                    count += entry.th32OwnerProcessID == GetCurrentProcessId() ? 1 : 0;
                } while (Thread32Next(snapshot, &entry));
            }
            CloseHandle(snapshot);
            std::cout << "THREAD_CONTROL standard=" << standard << " round=" << round << " handles=" << handles
                << " threads=" << count << " live_allocations=" << LockFreeHashMapThreadControl::s_live.load() << std::endl;
        }
#endif
    }
    catch (...)
    {
        std::cerr << "THREAD_CONTROL unexpected exception" << std::endl;
        return 4;
    }
    return LockFreeHashMapTestThread::failures() == 0 ? 0 : 4;
}