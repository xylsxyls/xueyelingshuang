#include "LumaPlayerWindowTask.h"
#include "LumaPlayerLogicController.h"

LumaPlayerWindowTask::LumaPlayerWindowTask(LumaPlayerLogicController* controller, const LumaPlayerLogicAction& action) :
LumaPlayerLogicTask(controller, action)
{

}

void LumaPlayerWindowTask::DoTask()
{
    if (!m_exit.load() && m_controller != nullptr)
    {
        m_controller->executeWindow(m_action);
    }
}