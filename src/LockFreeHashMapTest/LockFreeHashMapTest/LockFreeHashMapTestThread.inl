#pragma once

inline LockFreeHashMapTestResume::LockFreeHashMapTestResume(std::atomic<bool>& resume, LockFreeHashMapTestThread& thread) :
m_resume(resume),
m_thread(thread)
{

}

inline LockFreeHashMapTestResume::~LockFreeHashMapTestResume()
{
    m_resume.store(true);
    m_thread.join();
}

template <typename FunctionType>
LockFreeHashMapTestThread::LockFreeHashMapTestThread(const FunctionType& function) :
// 无参数工作函数使用独立副本；捕获所有执行异常，完成后由父用例检查累计失败数
m_thread([function]()
{
    try
    {
        function();
    }
    catch (...)
    {
        s_failures.fetch_add(1);
    }
})
{

}