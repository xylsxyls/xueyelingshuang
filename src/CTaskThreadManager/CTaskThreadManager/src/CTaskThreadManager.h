#pragma once
#include "CTask.h"
#include "CTaskThread.h"
#include <map>
#include <memory>

class CTaskThreadManagerAPI CTaskThreadManager
{
private:
    CTaskThreadManager();
    ~CTaskThreadManager();
public:
    static CTaskThreadManager& Instance();

    bool Init(int32_t threadId);

    void Uninit(int32_t threadId);

    void UninitAll();

    /* 在这里取出线程操作指针，不要长久保存该指针，否则即便线程停止也不会释放资源
    */
    std::shared_ptr<CTaskThread> GetThreadInterface(int32_t threadId);

private:

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

    std::map<int32_t, std::shared_ptr<CTaskThread>> m_spThreadMap;

    /* 线程池锁
    */
    std::mutex m_mutex;

#ifdef _MSC_VER
#pragma warning(pop)
#endif
};