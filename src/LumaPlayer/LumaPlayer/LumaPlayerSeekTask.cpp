#include "LumaPlayerSeekTask.h"
#include "LumaPlayerLogicController.h"

LumaPlayerSeekTask::LumaPlayerSeekTask(LumaPlayerLogicController* controller, const LumaPlayerLogicAction& action) :
LumaPlayerLogicTask(controller, action)
{

}

void LumaPlayerSeekTask::DoTask()
{
    if (!m_exit.load() && m_controller != nullptr)
    {
        m_controller->executeSeek(m_action);
    }
}