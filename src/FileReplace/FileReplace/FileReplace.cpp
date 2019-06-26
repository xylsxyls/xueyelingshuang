#include "FileReplace.h"
#include <stdint.h>
#include <stdio.h>
#include "CSystem/CSystemAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "FileTask.h"
#include "CGetPath/CGetPathAPI.h"
#include "CStringManager/CStringManagerAPI.h"

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//�ر��˳��¼�
		//RCSend("close FileReplace");
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

void fileReplace(bool isFile, EnumReplaceCommand replaceCommand, const std::string& fileName, const std::string& folder, const std::string& oldStr, const std::string& newStr)
{
	int32_t count = CSystem::GetCPUCoreCount() * 2;
	std::vector<uint32_t> vecThreadId;
	int32_t threadIndex = (isFile ? (count - 2) : (-1));
	while (threadIndex++ != count - 1)
	{
		vecThreadId.push_back(CTaskThreadManager::Instance().Init());
	}
	if (vecThreadId.empty())
	{
		return;
	}

	if (isFile)
	{
		FileTask* fileTask = new FileTask;
		fileTask->setParam(folder + fileName, replaceCommand, oldStr, newStr);
		std::shared_ptr<FileTask> spFileTask;
		spFileTask.reset(fileTask);
		CTaskThreadManager::Instance().GetThreadInterface(vecThreadId[0])->PostTask(spFileTask);
	}
	else
	{
		std::vector<std::string> vecPath = CGetPath::FindFilePath("", folder, 3);
		int32_t index = -1;
		while (index++ != vecPath.size() - 1)
		{
			std::string name = CSystem::GetName(vecPath[index], 1);
			if (CStringManager::Replace(name, fileName, fileName) != 0)
			{
				FileTask* fileTask = new FileTask;
				fileTask->setParam(vecPath[index], replaceCommand, oldStr, newStr);
				std::shared_ptr<FileTask> spFileTask;
				spFileTask.reset(fileTask);
				CTaskThreadManager::Instance().GetThreadInterface(vecThreadId[index % count])->PostTask(spFileTask);
			}
		}
	}
	threadIndex = (isFile ? (count - 2) : (-1));
	while (threadIndex++ != count - 1)
	{
		CTaskThreadManager::Instance().WaitForEnd(vecThreadId[isFile ? 0 : threadIndex]);
	}
}

void getParam(const std::vector<std::string>& vecMainParam, bool& isFile, EnumReplaceCommand& replaceCommand, std::string& fileName, std::string& folder, std::string& oldStr, std::string& newStr)
{
	if (vecMainParam.size() != 7)
	{
		return;
	}
	int32_t index = -1;
	while (index++ != vecMainParam.size() - 1)
	{
		switch (index)
		{
		case 1:
		{
			//�����ļ��滻
			if (CStringManager::MakeLower(vecMainParam[index]) == "-file")
			{
				isFile = true;
			}
			//�ļ������ļ�������ƥ��Ⱥ�滻
			else if (CStringManager::MakeLower(vecMainParam[index]) == "-dir")
			{
				isFile = false;
			}
		}
		break;
		case 2:
		{
			folder = vecMainParam[index];
			if (folder.back() != '\\' && folder.back() != '/')
			{
				folder.push_back('/');
			}
		}
		break;
		case 3:
		{
			fileName = vecMainParam[index];
		}
		break;
		case 4:
		{
			//ƥ��Ⱥ�廻
			if (CStringManager::MakeLower(vecMainParam[index]) == "-replace")
			{
				replaceCommand = REPLACE;
			}
			//���а���ƥ��������Ⱥ�廻
			else if (CStringManager::MakeLower(vecMainParam[index]) == "-line")
			{
				replaceCommand = LINE;
			}
			else if (CStringManager::MakeLower(vecMainParam[index]) == "-name")
			{
				replaceCommand = NAME;
			}
		}
		break;
		case 5:
		{
			oldStr = vecMainParam[index];
		}
		break;
		case 6:
		{
			newStr = vecMainParam[index];
		}
		break;
		default:
			break;
		}
	}
}

int32_t main()
{
	std::vector<std::string> vecMainParam = CSystem::mainParam();
	//�Ƿ����ļ����ļ���
	bool isFile = true;
	//�Ƿ��ǵ��а���ƥ��Ⱥ�廻��ƥ��Ⱥ�廻
	EnumReplaceCommand replaceCommand = REPLACE;
	//�ļ�����ƥ��Ĳ����ļ�����������׺
	std::string fileName;
	//�ļ�·��
	std::string folder;
	//ƥ����ַ���
	std::string oldStr;
	//�������ַ�������
	std::string newStr;

	getParam(vecMainParam, isFile, replaceCommand, fileName, folder, oldStr, newStr);
	fileReplace(isFile, replaceCommand, fileName, folder, oldStr, newStr);

	return 0;
}