#include "ReplaceNameTask.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"

ReplaceNameTask::ReplaceNameTask()
{

}

void ReplaceNameTask::DoTask()
{
	std::string oldName = CSystem::GetName(m_replaceNameParam.m_filePath, 1);
	std::string newName = oldName;
	if (m_replaceNameParam.m_isNameMatchCase)
	{
		CStringManager::Replace(newName, m_replaceNameParam.m_oldName, m_replaceNameParam.m_newName);
	}
	else
	{
		size_t findIndex = CStringManager::Find(CStringManager::MakeLower(newName), CStringManager::MakeLower(m_replaceNameParam.m_oldName));
		if (findIndex != -1)
		{
			int32_t index = -1;
			while (index++ != m_replaceNameParam.m_newName.size() - 1)
			{
				newName[findIndex++] = m_replaceNameParam.m_newName[index];
			}
		}
	}
	if (newName == oldName)
	{
		return;
	}
	CSystem::rename(oldName, newName);
}

void ReplaceNameTask::setParam(const ReplaceNameParam& replaceNameParam)
{
	m_replaceNameParam = replaceNameParam;
}