#include "PlayerCompletionTask.h"

PlayerCompletionTask::PlayerCompletionTask() :
CTask(2003),
m_exit(false)
{

}

bool PlayerCompletionTask::reserve(const std::shared_ptr<PlayerCompletionDelivery>& delivery)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_exit.load() || m_deliveries.size() >= kCapacity)
    {
        return false;
    }
    m_deliveries.push_back(delivery);
    return true;
}

void PlayerCompletionTask::publish(const std::shared_ptr<PlayerCompletionDelivery>& delivery)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        delivery->m_ready.store(true);
    }
    m_ready.notify_one();
}

void PlayerCompletionTask::DoTask()
{
    for (;;)
    {
        std::shared_ptr<PlayerCompletionDelivery> delivery;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            for (;;)
            {
                for (std::list<std::shared_ptr<PlayerCompletionDelivery>>::iterator it = m_deliveries.begin();
                    it != m_deliveries.end(); ++it)
                {
                    if ((*it)->m_ready.load())
                    {
                        delivery = *it;
                        m_deliveries.erase(it);
                        break;
                    }
                }
                if (delivery != nullptr)
                {
                    break;
                }
                if (m_exit.load())
                {
                    return;
                }
                m_ready.wait(lock);
            }
        }
        if (!delivery->m_accepted)
        {
            continue;
        }
        try
        {
            delivery->m_callback(delivery->m_completion);
        }
        catch (...)
        {
            // 宿主异常不影响其他请求的终态回报
        }
    }
}

void PlayerCompletionTask::StopTask()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_exit = true;
    }
    m_ready.notify_one();
}