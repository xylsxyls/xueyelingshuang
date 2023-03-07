#include "AssignTask.h"

AssignTask::AssignTask():
m_accountIndex(0)
{

}

AssignTask* AssignTask::copy()
{
	return nullptr;
}

void AssignTask::setAccountIndex(int32_t accountIndex)
{
	m_accountIndex = accountIndex;
}