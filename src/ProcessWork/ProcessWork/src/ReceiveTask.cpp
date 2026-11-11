#include "ReceiveTask.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProcessReceiveCallback.h"
#include <memory>
#include <cstdio>
#include <cstring>

ReceiveTask::ReceiveTask() :
m_fullCount(nullptr),
m_callback(nullptr),
m_receiveQueue(nullptr),
m_exit(false)
{

}

void ReceiveTask::doTask()
{
    if (m_callback == nullptr || m_receiveQueue == nullptr)
    {
        return;
    }
    bool reported = false;
    while (!m_exit)
    {
        // 空闲标志只读；确有满载通知时才执行写入缓存行的原子交换。
        const int32_t full = m_fullCount == nullptr || m_fullCount->load(std::memory_order_acquire) == 0 ?
            0 : m_fullCount->exchange(0, std::memory_order_acq_rel);
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
        if (!m_receiveQueue->take(&receive, 20) || receive == nullptr)
        {
            continue;
        }

        // 参数为本次尚未交付的资源；按对应所有者规则释放，不传播异常。
        auto release = [this](char* packet)
        {
            m_receiveQueue->release(packet);
        };
        std::unique_ptr<char, decltype(release)> owned(receive, release);
        if (m_exit)
        {
            break;
        }
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

void ReceiveTask::stopTask()
{
    m_exit = true;
    if (m_receiveQueue != nullptr)
    {
        m_receiveQueue->wake();
    }
}

void ReceiveTask::setParam(std::vector<ProcessReceiveCallback*>* callback,
    ProcessWorkReceiveQueue* receiveQueue,
    std::atomic<int32_t>* fullCount)
{
    m_fullCount = fullCount;
    m_callback = callback;
    m_receiveQueue = receiveQueue;
}