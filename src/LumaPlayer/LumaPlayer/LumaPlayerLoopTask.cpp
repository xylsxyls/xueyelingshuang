#include "LumaPlayerLoopTask.h"
#include "LumaPlayerLogicController.h"

LumaPlayerLoopTask::LumaPlayerLoopTask(LumaPlayerLogicController* controller, const LumaPlayerLogicAction& action) :
LumaPlayerLogicTask(controller, action)
{

}

void LumaPlayerLoopTask::DoTask()
{
    if (!m_exit.load() && m_controller != nullptr)
    {
        m_controller->executeLoop(m_action);
    }
}