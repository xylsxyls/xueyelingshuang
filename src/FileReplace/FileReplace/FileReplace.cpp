#include "FileReplace.h"
#include <stdint.h>
#include <stdio.h>
#include "CSystem/CSystemAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "ReplaceType.h"
#include "ReplaceNameTask.h"
#include "ReplaceContentTask.h"

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close FileReplace");
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

void getParam(const std::vector<std::string>& vecMainParam, ReplaceParam& replaceParam)
{
	std::vector<std::string> vecParam = vecMainParam;

	if (CStringManager::MakeLower(vecParam[0]) == "-file")
	{
		replaceParam.m_replaceType = REPLACE_FILE;
		replaceParam.m_filePath = vecParam[1];
		vecParam.pop_back();
		vecParam.pop_back();
	}
	else if (CStringManager::MakeLower(vecParam[0]) == "-dir")
	{
		replaceParam.m_replaceType = REPLACE_DIR;
		replaceParam.m_folder = vecParam[1];
		if (replaceParam.m_folder.back() != '\\')
		{
			replaceParam.m_folder.push_back('\\');
		}
		vecParam.pop_back();
		vecParam.pop_back();
	}
	else if (CStringManager::MakeLower(vecParam[0]) == "-files")
	{
		replaceParam.m_replaceType = REPLACE_FILES;
		replaceParam.m_folder = vecParam[1];
		if (replaceParam.m_folder.back() != '\\')
		{
			replaceParam.m_folder.push_back('\\');
		}
		replaceParam.m_fileName = vecParam[2];
		replaceParam.m_isMatchCase = (vecParam[3] != "0");
		vecParam.pop_back();
		vecParam.pop_back();
		vecParam.pop_back();
		vecParam.pop_back();
	}

	if (CStringManager::MakeLower(vecParam[0]) == "-replace")
	{
		replaceParam.m_replaceCommand = REPLACE;
		replaceParam.m_oldStr = vecParam[1];
		replaceParam.m_newStr = vecParam[2];
		vecParam.pop_back();
		vecParam.pop_back();
		vecParam.pop_back();
	}
	else if (CStringManager::MakeLower(vecParam[0]) == "-line")
	{
		replaceParam.m_replaceCommand = LINE;
		replaceParam.m_oldPart = vecParam[1];
		replaceParam.m_newLine = vecParam[2];
		vecParam.pop_back();
		vecParam.pop_back();
		vecParam.pop_back();
	}
	else if (CStringManager::MakeLower(vecParam[0]) == "-name")
	{
		replaceParam.m_replaceCommand = NAME;
		replaceParam.m_oldName = vecParam[1];
		replaceParam.m_newName = vecParam[2];
		replaceParam.m_isNameMatchCase = (vecParam[3] != "0");
		vecParam.pop_back();
		vecParam.pop_back();
		vecParam.pop_back();
		vecParam.pop_back();
	}
	else if (CStringManager::MakeLower(vecParam[0]) == "-part")
	{
		replaceParam.m_replaceCommand = PART;
		replaceParam.m_oldPart = vecParam[1];
		replaceParam.m_oldStr = vecParam[2];
		replaceParam.m_newStr = vecParam[3];
		vecParam.pop_back();
		vecParam.pop_back();
		vecParam.pop_back();
		vecParam.pop_back();
	}

	if (vecParam.size() == 2)
	{
		replaceParam.m_beginLine = atoi(vecParam[0].c_str());
		replaceParam.m_endLine = atoi(vecParam[1].c_str());
	}
}

std::vector<std::shared_ptr<CTask>> AnalyzeParam(const ReplaceParam& replaceParam)
{
	std::vector<std::string> vecFiles;
	switch (replaceParam.m_replaceType)
	{
	case REPLACE_FILE:
	{
		vecFiles.push_back(replaceParam.m_filePath);
	}
	break;
	case REPLACE_DIR:
	{
		vecFiles = CSystem::findFilePath(replaceParam.m_folder);
	}
	break;
	case REPLACE_FILES:
	{
		std::vector<std::string> vecAllFiles = CSystem::findFilePath(replaceParam.m_folder);
		int32_t index = -1;
		while (index++ != vecAllFiles.size() - 1)
		{
			const std::string& path = vecAllFiles[index];
			std::string name = CSystem::GetName(path, 1);
			if (replaceParam.m_isMatchCase)
			{
				if (CStringManager::Find(name, replaceParam.m_fileName) != -1)
				{
					vecFiles.push_back(path);
				}
			}
			else
			{
				if (CStringManager::Find(CStringManager::MakeLower(name), CStringManager::MakeLower(replaceParam.m_fileName)) != -1)
				{
					vecFiles.push_back(path);
				}
			}
		}
	}
	break;
	default:
		break;
	}

	std::vector<std::shared_ptr<CTask>> vecTask;
	if (replaceParam.m_replaceCommand == NAME)
	{
		int32_t index = -1;
		while (index++ != vecFiles.size() - 1)
		{
			ReplaceNameParam replaceNameParam;
			replaceNameParam.m_filePath = vecFiles[index];
			replaceNameParam.m_isNameMatchCase = replaceParam.m_isNameMatchCase;
			replaceNameParam.m_oldName = replaceParam.m_oldName;
			replaceNameParam.m_newName = replaceParam.m_newName;
			std::shared_ptr<ReplaceNameTask> spReplaceNameTask(new ReplaceNameTask);
			spReplaceNameTask->setParam(replaceNameParam);
			vecTask.push_back(spReplaceNameTask);
		}
	}
	else
	{
		int32_t index = -1;
		while (index++ != vecFiles.size() - 1)
		{
			ReplaceContentParam replaceContentParam;
			replaceContentParam.m_replaceCommand = replaceParam.m_replaceCommand;
			replaceContentParam.m_filePath = vecFiles[index];
			replaceContentParam.m_oldStr = replaceParam.m_oldStr;
			replaceContentParam.m_newStr = replaceParam.m_newStr;
			replaceContentParam.m_newLine = replaceParam.m_newLine;
			replaceContentParam.m_oldPart = replaceParam.m_oldPart;
			replaceContentParam.m_beginLine = replaceParam.m_beginLine;
			replaceContentParam.m_endLine = replaceParam.m_endLine;
			std::shared_ptr<ReplaceContentTask> spReplaceContentTask(new ReplaceContentTask);
			spReplaceContentTask->setParam(replaceContentParam);
			vecTask.push_back(spReplaceContentTask);
		}
	}
	return vecTask;
}

int32_t main()
{
	std::vector<std::string> vecMainParam = CSystem::mainParam();
	ReplaceParam replaceParam;

	getParam(vecMainParam, replaceParam);
	std::vector<std::shared_ptr<CTask>> vecTask = AnalyzeParam(replaceParam);
	if (vecTask.empty())
	{
		return 0;
	}
	size_t threadCount = CSystem::GetCPUCoreCount() * 2;
	if (vecTask.size() < threadCount)
	{
		threadCount = vecTask.size();
	}
	std::vector<uint32_t> vecThread;
	int32_t index = -1;
	while (index++ != threadCount - 1)
	{
		vecThread.push_back(CTaskThreadManager::Instance().Init());
	}
	index = -1;
	while (index++ != vecTask.size() - 1)
	{
		CTaskThreadManager::Instance().GetThreadInterface(vecThread[index % threadCount])->PostTask(vecTask[index]);
	}
	index = -1;
	while (index++ != vecThread.size() - 1)
	{
		CTaskThreadManager::Instance().WaitForEnd(vecThread[index]);
	}
	return 0;
}