#include "CTaskPoolControlTask.h"
#include "CTaskPoolState.h"

CTaskPoolControlTask::CTaskPoolControlTask(CTaskPoolState& state) :
m_state(&state)
{

}

void CTaskPoolControlTask::DoTask()
{
    m_state->run();
}

void CTaskPoolControlTask::StopTask()
{
    m_state->requestStop();
}