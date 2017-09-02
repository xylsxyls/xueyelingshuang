#include "CTask.h"

CTask::CTask(int32_t taskId) :
m_taskId(taskId)
{

}

int32_t CTask::GetTaskId()
{
    return m_taskId;
}