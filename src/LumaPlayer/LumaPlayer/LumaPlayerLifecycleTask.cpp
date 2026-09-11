#include "LumaPlayerLifecycleTask.h"
#include "LumaPlayerLogicController.h"

LumaPlayerLifecycleTask::LumaPlayerLifecycleTask(LumaPlayerLogicController* controller, bool shutdown) :
CTask(shutdown ? 5001 : 5000),
m_controller(controller),
m_shutdown(shutdown),
m_exit(false)
{

}

void LumaPlayerLifecycleTask::DoTask()
{
    if (m_controller != nullptr)
    {
        m_controller->executeLifecycle(m_shutdown, &m_exit);
    }
}

void LumaPlayerLifecycleTask::StopTask()
{
    m_exit = true;
}