#include "LumaPlayerViewTask.h"
#include "LumaPlayerLogicController.h"

LumaPlayerViewTask::LumaPlayerViewTask(LumaPlayerLogicController* controller, const LumaPlayerLogicAction& action) :
LumaPlayerLogicTask(controller, action)
{

}

void LumaPlayerViewTask::DoTask()
{
    if (!m_exit.load() && m_controller != nullptr)
    {
        m_controller->executeView(m_action);
    }
}