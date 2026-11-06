#include "CTask.h"

CTask::CTask() :
m_propertyId(1)
{

}

CTask::CTask(int32_t propertyId) :
m_propertyId(propertyId)
{

}

CTask::~CTask()
{

}

void CTask::StopTask()
{

}

bool CTask::ReExecute()
{
    return false;
}

CTask* CTask::Clone()
{
    return nullptr;
}

int32_t CTask::GetPropertyId()
{
    return m_propertyId;
}