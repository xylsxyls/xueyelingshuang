#include "SendTask.h"
#include "Semaphore/SemaphoreAPI.h"
#include "ProcessWork.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "CSystem/CSystemAPI.h"
#include <cstdio>
#include <memory>
#include <cstring>

#ifdef _MSC_VER
static __declspec(thread) const std::atomic<bool>* s_currentStopFlag = nullptr;
#else
static thread_local const std::atomic<bool>* s_currentStopFlag = nullptr;
#endif

SendTask::SendTask():
m_owner(nullptr),
m_waitEndPost(nullptr),
m_postQueue(nullptr),
m_postSemaphore(nullptr),
m_exit(false),
m_ready(true)
{

}

void SendTask::DoTask()
{
    const std::atomic<bool>* previous = setCurrentStopFlag(&m_exit);
    // 借用标记，退出作用域时恢复嵌套调用前的线程状态；不释放任务成员。
    auto restore = [previous](const std::atomic<bool>*)
    {
        setCurrentStopFlag(previous);
    };
    std::unique_ptr<const std::atomic<bool>, decltype(restore)> scope(&m_exit, restore);
    while (!m_exit && !m_ready)
    {
        CSystem::Sleep(1);
    }
    bool reported = false;
    while (!m_exit)
    {
        m_postSemaphore->wait(20);
        if (m_exit || (*m_waitEndPost && m_postQueue->empty()))
        {
            break;
        }

        char* postBuffer = nullptr;
        if (!m_postQueue->pop(&postBuffer) || postBuffer == nullptr)
        {
            continue;
        }
        std::unique_ptr<char[]> owned(postBuffer);
        while (!m_exit)
        {
            try
            {
                int32_t header[4] = {};
                std::memcpy(header, postBuffer, sizeof(header));
                ProcessWork& owner = m_owner == nullptr ? ProcessWork::instance() : *m_owner;
                const char* data = header[2] == 0 ? nullptr : postBuffer + 16 + header[3];
                if (header[3] == 0)
                {
                    owner.send(header[0], data, header[2], static_cast<MessageType>(header[1]));
                }
                else
                {
                    owner.send(std::string(postBuffer + 16, header[3]), data, header[2], static_cast<MessageType>(header[1]));
                }
                break;
            }
            catch (...)
            {
                if (!reported)
                {
                    std::fputs("ProcessWork post send failed; retaining current message for retry\n", stderr);
                    reported = true;
                }
            }
            // 保留当前消息及顺序，资源恢复后重试；只有显式停止才取消待发工作。
            CSystem::Sleep(1);
        }
    }
}

void SendTask::StopTask()
{
    m_exit = true;
    m_postSemaphore->signal();
}

const std::atomic<bool>* SendTask::currentStopFlag()
{
    return s_currentStopFlag;
}

const std::atomic<bool>* SendTask::setCurrentStopFlag(const std::atomic<bool>* flag)
{
    const std::atomic<bool>* previous = s_currentStopFlag;
    s_currentStopFlag = flag;
    return previous;
}

void SendTask::start()
{
    m_ready = true;
}

void SendTask::setParam(std::atomic<bool>* waitEndPost, LockFreeQueue<char*>* postQueue, Semaphore* postSemaphore, bool ready, ProcessWork* owner)
{
    m_waitEndPost = waitEndPost;
    m_postQueue = postQueue;
    m_postSemaphore = postSemaphore;
    m_ready = ready;
    m_owner = owner;
}