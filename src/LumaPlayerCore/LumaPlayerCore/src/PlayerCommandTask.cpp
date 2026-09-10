#include "PlayerCommandTask.h"
#include "PlayerEngine.h"

PlayerCommandTask::PlayerCommandTask(PlayerEngine* engine, const PlayerCommand& command) :
CTask(static_cast<int32_t>(command.m_type) + kTaskIdBase),
m_engine(engine),
m_command(command),
m_exit(false)
{

}

void PlayerCommandTask::DoTask()
{
    if (m_exit.load())
    {
        if (m_command.m_syncResult != nullptr)
        {
            m_command.m_syncResult->m_result = LumaPlayerCoreResultCanceled;
            m_command.m_syncResult->m_semaphore.signal();
        }
        return;
    }
    if (m_engine != nullptr)
    {
        m_engine->executeCommandTask(m_command, &m_exit);
    }
}

void PlayerCommandTask::StopTask()
{
    m_exit.store(true);
}