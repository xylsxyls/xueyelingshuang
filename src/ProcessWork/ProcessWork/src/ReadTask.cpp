#include "ReadTask.h"
#include "ProcessWorkChannel.h"

ReadTask::ReadTask() :
m_channel(nullptr), m_queue(nullptr), m_fullCount(nullptr), m_exit(false)
{

}

void ReadTask::doTask()
{
    if (m_channel == nullptr || m_queue == nullptr || m_fullCount == nullptr)
    {
        return;
    }
    while (!m_exit)
    {
        int32_t copied = 0;
        try
        {
            copied = m_channel->pump(*m_queue, *m_fullCount);
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

void ReadTask::stopTask()
{
    m_exit = true;
    if (m_channel != nullptr)
    {
        m_channel->wake();
    }
}

void ReadTask::setParam(ProcessWorkChannel* channel, ProcessWorkReceiveQueue* queue,
    std::atomic<int32_t>* fullCount)
{
    m_channel = channel;
    m_queue = queue;
    m_fullCount = fullCount;
}