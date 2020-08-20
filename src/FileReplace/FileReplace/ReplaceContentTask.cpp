#include "ReplaceContentTask.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"

ReplaceContentTask::ReplaceContentTask()
{

}

void ReplaceContentTask::DoTask()
{
	Ctxt file(m_replaceContentParam.m_filePath);
	file.LoadTxt(Ctxt::ONE_LINE);
	if (m_replaceContentParam.m_beginLine == 0)
	{
		m_replaceContentParam.m_beginLine = 1;
	}
	if (m_replaceContentParam.m_endLine == 0)
	{
		m_replaceContentParam.m_endLine = (int32_t)file.m_vectxt.size();
	}
	int32_t index = m_replaceContentParam.m_beginLine - 2;
	switch (m_replaceContentParam.m_replaceCommand)
	{
	case REPLACE:
	{
		while (index++ != m_replaceContentParam.m_endLine - 1)
		{
			std::string& line = file.m_vectxt[index][0];
			CStringManager::Replace(line, m_replaceContentParam.m_oldStr, m_replaceContentParam.m_newStr);
		}
	}
	break;
	case LINE:
	{
		while (index++ != m_replaceContentParam.m_endLine - 1)
		{
			std::string& line = file.m_vectxt[index][0];
			if (CStringManager::Find(line, m_replaceContentParam.m_oldPart) != -1)
			{
				line = m_replaceContentParam.m_newLine;
			}
		}
	}
	break;
	case PART:
	{
		while (index++ != m_replaceContentParam.m_endLine - 1)
		{
			std::string& line = file.m_vectxt[index][0];
			if (CStringManager::Find(line, m_replaceContentParam.m_oldPart) != -1)
			{
				CStringManager::Replace(line, m_replaceContentParam.m_oldStr, m_replaceContentParam.m_newStr);
			}
		}
	}
	break;
	default:
		break;
	}
	file.Save();
}

void ReplaceContentTask::setParam(const ReplaceContentParam& replaceContentParam)
{
	m_replaceContentParam = replaceContentParam;
}