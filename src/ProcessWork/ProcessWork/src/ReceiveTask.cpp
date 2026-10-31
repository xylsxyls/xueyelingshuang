#include "ReceiveTask.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProcessReceiveCallback.h"
#include "Semaphore/SemaphoreAPI.h"
#include <memory>
#include <cstdio>
#include <cstring>

ReceiveTask::ReceiveTask():
m_fullCount(nullptr),
m_callback(nullptr),
m_receiveQueue(nullptr),
m_receiveSemaphore(nullptr),
m_exit(false)
{

}

void ReceiveTask::DoTask()
{
    if (m_callback == nullptr || m_receiveQueue == nullptr || m_receiveSemaphore == nullptr)
    {
        return;
    }
    bool reported = false;
    while (!m_exit)
    {
        m_receiveSemaphore->wait(20);
        if (m_exit)
        {
            break;
        }
        const int32_t full = m_fullCount == nullptr ? 0 : m_fullCount->exchange(0);
        if (full > 0)
        {
            for (size_t i = 0; i < m_callback->size(); ++i)
            {
                if ((*m_callback)[i] != nullptr)
                {
                    try
                    {
                        (*m_callback)[i]->bufferFull(full);
                    }
                    catch (...)
                    {
                        std::fputs("ProcessWork bufferFull callback failed\n", stderr);
                    }
                }
            }
        }
        char* receive = nullptr;
        if (!m_receiveQueue->pop(&receive) || receive == nullptr)
        {
            continue;
        }

        std::unique_ptr<char[]> owned(receive);
        int32_t header[3] = {};
        std::memcpy(header, receive, sizeof(header));
        const int32_t sendPid = header[0];
        const MessageType type = static_cast<MessageType>(header[1]);
        const int32_t length = header[2];
        for (auto itCallback = m_callback->begin(); itCallback != m_callback->end(); ++itCallback)
        {
            if (*itCallback == nullptr)
            {
                continue;
            }
            try
            {
                (*itCallback)->receive(sendPid, length == 0 ? nullptr : receive + 12, length, type);
            }
            catch (...)
            {
                // 一名观察者失败不能终止CTask或阻止其他观察者；正文仍由owned唯一释放。
                if (!reported)
                {
                    std::fputs("ProcessWork receive callback failed; exception isolated\n", stderr);
                    reported = true;
                }
            }
        }
    }
}

void ReceiveTask::StopTask()
{
    m_exit = true;
    m_receiveSemaphore->signal();
}

void ReceiveTask::setParam(std::vector<ProcessReceiveCallback*>* callback,
    LockFreeQueue<char*>* receiveQueue,
    Semaphore* receiveSemaphore, std::atomic<int32_t>* fullCount)
{
    m_fullCount = fullCount;
    m_callback = callback;
    m_receiveQueue = receiveQueue;
    m_receiveSemaphore = receiveSemaphore;
}