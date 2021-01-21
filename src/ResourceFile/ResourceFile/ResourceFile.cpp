#include "ResourceFile.h"
#include <stdint.h>
#include <stdio.h>
#include "CSystem/CSystemAPI.h"

#ifdef _MSC_VER
BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//�ر��˳��¼�
		//RCSend("close ResourceFile");
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);
#endif

int32_t main(int argc, char** argv)
{
	std::vector<std::string> vecParam = CSystem::exeParam(argc, argv);
	if (vecParam.size() != 2)
	{
		printf("���϶��ļ���exe\n");
		return -1;
	}
	std::string filePath = vecParam[1];
	std::string fileName = CSystem::GetName(filePath, 3);
	std::string file = CSystem::readFile(filePath);

	if (file.size() <= 2)
	{
		printf("�ļ���С�쳣\n");
		return -1;
	}

	std::string content = "std::string " + fileName + " = {";

	int32_t line = 0;
	int32_t index = -1;
	while (index++ < (int32_t)file.size() - 2)
	{
		std::string end = " ";
		if ((int32_t)content.size() / 100 > line)
		{
			line = content.size() / 100;
#ifdef _MSC_VER
			end = "\r\n";
#elif __unix__
			end = "\n";
#endif
		}
		char c = file[index];
		content.append(std::to_string(c) + "," + end);
	}
	content.append(std::to_string(file[index]) + "};");
	CSystem::saveFile(content, CSystem::GetName(filePath, 4) + fileName + ".txt");
	return 0;
}