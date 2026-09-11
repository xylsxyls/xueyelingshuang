#include "LumaPlayerPlaybackTask.h"
#include "LumaPlayerLogicController.h"

LumaPlayerPlaybackTask::LumaPlayerPlaybackTask(LumaPlayerLogicController* controller, const LumaPlayerLogicAction& action) :
LumaPlayerLogicTask(controller, action)
{

}

void LumaPlayerPlaybackTask::DoTask()
{
    if (!m_exit.load() && m_controller != nullptr)
    {
        m_controller->executePlayback(m_action);
    }
}