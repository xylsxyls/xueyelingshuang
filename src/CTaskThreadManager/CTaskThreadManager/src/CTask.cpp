#include "CTask.h"

CTask::CTask(__int32 taskId) :
m_taskId(taskId)
{

}

__int32 CTask::GetTaskId()
{
    return m_taskId;
}