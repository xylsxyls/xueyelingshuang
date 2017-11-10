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
	
	//dev路径
	std::string devExe = getenv("DEVENV_EXE");
	//xueyelingshuang路径
	std::string xueyePath = getenv("XUEYELINGSHUANG");
	//工程路径
	std::string projectPath = std::string() + xueyePath + "src\\" + projectName + "\\";

	//准备脚本路径
	std::string beforeBatPath = projectPath + "scripts\\before_" + projectName + ".bat";
	//后期脚本路径
	std::string afterBatPath = projectPath + "scripts\\after_" + projectName + ".bat";
	//32位自动化编译python脚本路径
	std::string pyPath = projectPath + "scripts\\rebuild_" + dlllib + "_" + projectName + ".py";

	std::string srcPath = projectPath + projectName + "\\src\\";
	std::string includePath = xueyePath + "include\\" + projectName + "\\";

	//执行准备脚本（准备脚本中有依赖库的调用）
	system(("call" + SPACE + MARK(beforeBatPath) + SPACE + bit + SPACE + dlllib + SPACE + debugRelease + (allSame == "" ? "" : (SPACE + allSame))).c_str());
	
	//编译工程
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
	//向公共部分提供文件，有的时候可能没有inl文件
	
	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(srcPath + "*.h") + SPACE + MARK(includePath)).c_str());
	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(srcPath + "*.inl") + SPACE + MARK(includePath)).c_str());

	//删除宏文件，并且替换特定的宏文件
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

	//删除临时文件
	system(("del" + SPACE + MARK(projectPath + "@AutomationLog.txt")).c_str());
	system(("del" + SPACE + MARK(projectPath + "scripts\\@AutomationLog.txt")).c_str());
	system(("del" + SPACE + MARK(projectPath + "scripts\\msbuild.log")).c_str());
	std::string d;
	if (debugRelease != "release")
	{
		d = "d";
	}
	system(("del" + SPACE + MARK(xueyePath + "lib\\" + projectName + d + ".ilk")).c_str());
	system(("del" + SPACE + MARK(xueyePath + "lib\\" + projectName + d + ".exp")).c_str());

	//恢复macro文件
	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(projectPath + "scripts\\" + projectName + "Macro.h") + SPACE + MARK(srcPath)).c_str());
	system(("del" + SPACE + MARK(projectPath + "scripts\\" + projectName + "Macro.h")).c_str());
	
	//执行后期脚本
	system(("call" + SPACE + MARK(afterBatPath)).c_str());
	return 0;
}