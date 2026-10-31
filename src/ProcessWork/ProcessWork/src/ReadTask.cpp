#include "ReadTask.h"
#include "ProcessWorkChannel.h"

ReadTask::ReadTask(): m_channel(nullptr), m_queue(nullptr), m_notification(nullptr), m_fullCount(nullptr), m_exit(false)
{
}

void ReadTask::DoTask()
{
    if (m_channel == nullptr || m_queue == nullptr || m_notification == nullptr || m_fullCount == nullptr)
    {
        return;
    }
    while (!m_exit)
    {
        int32_t copied = 0;
        try
        {
            copied = m_channel->pump(*m_queue, *m_notification, *m_fullCount);
        }
        catch (...)
        {
            copied = 0;
        }
        if (copied == 0 && !m_exit)
        {
            m_channel->waitForData();
        }
    }
}

void ReadTask::StopTask()
{
    m_exit = true;
    if (m_channel != nullptr)
    {
        m_channel->wake();
    }
}

void ReadTask::setParam(ProcessWorkChannel* channel, LockFreeQueue<char*>* queue,
    Semaphore* notification, std::atomic<int32_t>* fullCount)
{
    m_channel = channel;
    m_queue = queue;
    m_notification = notification;
    m_fullCount = fullCount;
}