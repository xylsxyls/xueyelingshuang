#include "LumaPlayerLogicTask.h"
#include "LumaPlayerLogicController.h"

LumaPlayerLogicTask::LumaPlayerLogicTask(LumaPlayerLogicController* controller, const LumaPlayerLogicAction& action) :
CTask(4000 + static_cast<int32_t>(action.m_type)),
m_controller(controller),
m_action(action),
m_exit(false)
{

}

void LumaPlayerLogicTask::DoTask()
{
    if (!m_exit.load() && m_controller != nullptr)
    {
        m_controller->execute(m_action);
    }
}

void LumaPlayerLogicTask::StopTask()
{
    m_exit = true;
}