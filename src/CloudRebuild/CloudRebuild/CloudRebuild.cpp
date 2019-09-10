#include "CloudRebuild.h"
#include <string>

#define SPACE std::string(" ")
#define MARK(src) ("\"" + src + "\"")

int main()
{
	std::string projectName;
	std::string bit;
	std::string dlllib;
	std::string debugRelease;
	std::string allSame;

	if (__argc == 5)
	{
		projectName = __argv[1];
		bit = __argv[2];
		dlllib = __argv[3];
		debugRelease = __argv[4];
	}
	else if (__argc == 6)
	{
		projectName = __argv[1];
		bit = __argv[2];
		dlllib = __argv[3];
		debugRelease = __argv[4];
		allSame = __argv[5];
	}
	
	//dev·��
	std::string devExe = getenv("DEVENV_EXE");
	//xueyelingshuang·��
	std::string xueyePath = getenv("XUEYELINGSHUANG");
	//����·��
	std::string projectPath = std::string() + xueyePath + "src\\" + projectName + "\\";

	//׼���ű�·��
	std::string beforeBatPath = projectPath + "scripts\\before_" + projectName + ".bat";
	//���ڽű�·��
	std::string afterBatPath = projectPath + "scripts\\after_" + projectName + ".bat";
	//32λ�Զ�������python�ű�·��
	std::string pyPath = projectPath + "scripts\\rebuild_" + dlllib + "_" + projectName + ".py";

	std::string srcPath = projectPath + projectName + "\\src\\";
	std::string includePath = xueyePath + "include\\" + projectName + "\\";

	if (allSame != "only")
	{
		//ִ��׼���ű���׼���ű�����������ĵ��ã�
		system(("call" + SPACE + MARK(beforeBatPath) + SPACE + bit + SPACE + dlllib + SPACE + debugRelease + (allSame == "" ? "" : (SPACE + allSame))).c_str());
	}
	
	//���빤��
	if (bit == "32")
	{
		system(("call" + SPACE + MARK(pyPath) + SPACE + debugRelease).c_str());
	}
	else if (bit == "64")
	{
		std::string hasLib;
		if (dlllib == "lib")
		{
			hasLib = "_lib";
		}
		system(("call" + SPACE + MARK(devExe) + SPACE + MARK(projectPath + projectName + hasLib + ".sln") + SPACE + " /rebuild" + SPACE + MARK(debugRelease + "|x64")).c_str());
	}
	//�򹫹������ṩ�ļ����е�ʱ�����û��inl�ļ�
	
	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(srcPath + "*.h") + SPACE + MARK(includePath)).c_str());
	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(srcPath + "*.inl") + SPACE + MARK(includePath)).c_str());

	//ɾ�����ļ��������滻�ض��ĺ��ļ�
	if (dlllib == "lib")
	{
		system(("del" + SPACE + MARK(includePath + projectName + "Macro.h")).c_str());
		system(("del" + SPACE + MARK(includePath + projectName + "MacroDll.h")).c_str());
		rename((includePath + projectName + "MacroLib.h").c_str(), (includePath + projectName + "Macro.h").c_str());
	}
	else if (dlllib == "dll")
	{
		system(("del" + SPACE + MARK(includePath + projectName + "Macro.h")).c_str());
		system(("del" + SPACE + MARK(includePath + projectName + "MacroLib.h")).c_str());
		rename((includePath + projectName + "MacroDll.h").c_str(), (includePath + projectName + "Macro.h").c_str());
	}

	//ɾ����ʱ�ļ�
	system(("del" + SPACE + MARK(projectPath + "@AutomationLog.txt")).c_str());
	system(("del" + SPACE + "@AutomationLog.txt").c_str());
	system(("del" + SPACE + MARK(projectPath + "scripts\\@AutomationLog.txt")).c_str());
	system(("del" + SPACE + MARK(projectPath + "scripts\\msbuild.log")).c_str());
	std::string d;
	if (debugRelease != "release")
	{
		d = "d";
	}
	system(("del" + SPACE + MARK(xueyePath + "lib\\" + projectName + d + ".ilk")).c_str());
	system(("del" + SPACE + MARK(xueyePath + "lib\\" + projectName + d + ".exp")).c_str());

	//�ָ�macro�ļ�
	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(projectPath + "scripts\\" + projectName + "Macro.h") + SPACE + MARK(srcPath)).c_str());
	system(("del" + SPACE + MARK(projectPath + "scripts\\" + projectName + "Macro.h")).c_str());
	
	//ִ�к��ڽű�
	system(("call" + SPACE + MARK(afterBatPath)).c_str());
	return 0;
}