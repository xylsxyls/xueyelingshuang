#include "LumaPlayerLoadTask.h"
#include "LumaPlayerLogicController.h"

LumaPlayerLoadTask::LumaPlayerLoadTask(LumaPlayerLogicController* controller, const LumaPlayerLogicAction& action) :
LumaPlayerLogicTask(controller, action)
{

}

void LumaPlayerLoadTask::DoTask()
{
    if (!m_exit.load() && m_controller != nullptr)
    {
        m_controller->executeLoad(m_action);
    }
}