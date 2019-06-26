#include "FileTask.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"

FileTask::FileTask():
m_replaceCommand(REPLACE)
{

}

void FileTask::DoTask()
{
	if (m_replaceCommand == NAME)
	{
		std::string oldFilePath = m_filePath;
		CStringManager::Replace(m_filePath, m_oldStr, m_newStr);
		CSystem::rename(oldFilePath.c_str(), m_filePath.c_str());
		return;
	}
	if (!CSystem::fileExist(m_filePath))
	{
		return;
	}
	Ctxt txt(m_filePath);
	txt.LoadTxt(Ctxt::ONE_LINE);
	int32_t lineIndex = -1;
	while (lineIndex++ != txt.m_vectxt.size() - 1)
	{
		switch (m_replaceCommand)
		{
		case REPLACE:
		{
			CStringManager::Replace(txt.m_vectxt[lineIndex][0], m_oldStr, m_newStr);
		}
		break;
		case LINE:
		{
			if (CStringManager::Replace(txt.m_vectxt[lineIndex][0], m_oldStr, m_oldStr) != 0)
			{
				txt.m_vectxt[lineIndex][0] = m_newStr;
			}
		}
		break;
		default:
			break;
		}
	}
	txt.Save();
}

void FileTask::setParam(const std::string& filePath, EnumReplaceCommand replaceCommand, const std::string& oldStr, const std::string& newStr)
{
	m_filePath = filePath;
	m_replaceCommand = replaceCommand;
	m_oldStr = oldStr;
	m_newStr = newStr;
}