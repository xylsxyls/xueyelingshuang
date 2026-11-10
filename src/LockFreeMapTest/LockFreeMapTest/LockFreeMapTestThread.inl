#ifndef LOCK_FREE_MAP_TEST_THREAD_INL
#define LOCK_FREE_MAP_TEST_THREAD_INL

template <typename Function>
LockFreeMapTestThread::LockFreeMapTestThread(Function function) :
m_handle(),
m_joinable(false)
{
    std::function<void()>* work = nullptr;
    try
    {
        work = new (std::nothrow) std::function<void()>(function);
    }
    catch (...)
    {
        m_errors.fetch_add(1);
        throw;
    }
    if (work == nullptr || !start(work))
    {
        delete work;
        m_errors.fetch_add(1);
        // 测试基础设施启动失败必须交给用例边界，不能伪造已启动线程
        throw std::runtime_error("test worker startup failed");
    }
}

#endif