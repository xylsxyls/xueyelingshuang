#include "LumaPlayerResetTask.h"
#include "LumaPlayerLogicController.h"

LumaPlayerResetTask::LumaPlayerResetTask(LumaPlayerLogicController* controller, const LumaPlayerLogicAction& action) :
LumaPlayerLogicTask(controller, action)
{

}

void LumaPlayerResetTask::DoTask()
{
    if (!m_exit.load() && m_controller != nullptr)
    {
        m_controller->executeReset(m_action);
    }
}