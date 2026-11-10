#include "CTaskPoolControlTask.h"
#include "CTaskPoolState.h"

CTaskPoolControlTask::CTaskPoolControlTask(CTaskPoolState& state) :
m_state(&state)
{

}

void CTaskPoolControlTask::doTask()
{
    m_state->run();
}

void CTaskPoolControlTask::stopTask()
{
    m_state->requestStop();
}