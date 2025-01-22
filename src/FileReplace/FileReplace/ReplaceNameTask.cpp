#include "ReplaceNameTask.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"

ReplaceNameTask::ReplaceNameTask()
{

}

void ReplaceNameTask::DoTask()
{
	std::string oldFileName = CSystem::GetName(m_replaceNameParam.m_filePath, 1);
	std::string newFileName = oldFileName;
	if (m_replaceNameParam.m_isNameMatchCase)
	{
		CStringManager::Replace(newFileName, m_replaceNameParam.m_oldName, m_replaceNameParam.m_newName);
	}
	else
	{
		size_t findIndex = CStringManager::Find(CStringManager::MakeLower(newFileName), CStringManager::MakeLower(m_replaceNameParam.m_oldName));
		if (findIndex != -1)
		{
			int32_t index = -1;
			while (index++ != m_replaceNameParam.m_newName.size() - 1)
			{
				newFileName[findIndex++] = m_replaceNameParam.m_newName[index];
			}
		}
	}
	if (newFileName == oldFileName)
	{
		return;
	}
	CSystem::rename(m_replaceNameParam.m_filePath, CSystem::GetName(m_replaceNameParam.m_filePath, 4) + newFileName);
}

void ReplaceNameTask::setParam(const ReplaceNameParam& replaceNameParam)
{
	m_replaceNameParam = replaceNameParam;
}