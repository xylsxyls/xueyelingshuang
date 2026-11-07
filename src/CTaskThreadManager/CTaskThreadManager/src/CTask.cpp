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

int32_t CTask::GetPropertyId()
{
    return m_propertyId;
}

bool CTask::Clone(const std::function<CTask*()>& factory)
{
    if (!factory)
    {
        m_cloneFactory.reset();
        return true;
    }
    try
    {
        std::shared_ptr<const std::function<CTask*()>> replacement = std::make_shared<std::function<CTask*()>>(factory);
        m_cloneFactory.swap(replacement);
        return true;
    }
    catch (...)
    {
        return false;
    }
}