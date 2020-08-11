#include "CloudRebuild.h"
#include <string>

#define SPACE std::string(" ")
#define MARK(src) ("\"" + src + "\"")
#ifdef _WIN32
#define PATHLEVEL std::string("\\")
#elif __linux__
#define PATHLEVEL std::string("/")
#endif

int main(int argc, char** argv)
{
	std::string projectName;
	std::string bit;
	std::string dlllib;
	std::string debugRelease;
	std::string allSame;

	if (argc != 6)
	{
		return 0;
	}
	projectName = argv[1];
	bit = argv[2];
	dlllib = argv[3];
	debugRelease = argv[4];
	allSame = argv[5];
	
	//xueyelingshuang路径
	std::string xueyePath = getenv("XUEYELINGSHUANG");
	//工程路径
	std::string projectPath = xueyePath + "src" + PATHLEVEL + projectName + PATHLEVEL;

	//准备脚本路径
	std::string beforeBatPath = projectPath + "scripts" + PATHLEVEL + "before_" + projectName + ".sh";
	//后期脚本路径
	std::string afterBatPath = projectPath + "scripts" + PATHLEVEL + "after_" + projectName + ".sh";

#ifdef _WIN32
	//32位自动化编译python脚本路径
	std::string pyPath = projectPath + "scripts" + PATHLEVEL + "rebuild_" + projectName + ".py";
#elif __linux__
	//linux下CMakeLists文件路径
	std::string cmakelistsPath = projectPath + "scripts" + PATHLEVEL + "CMakeLists.txt";
	//linux下build目录
	std::string buildPath = projectPath + "scripts" + PATHLEVEL + "build" + PATHLEVEL;
#endif

	std::string srcPath = projectPath + projectName + PATHLEVEL + "src" + PATHLEVEL;
	std::string includePath = xueyePath + "include" + PATHLEVEL + projectName + PATHLEVEL;

	std::string deleteFileCommand;
#ifdef _WIN32
	deleteFileCommand = "del";
#elif __linux__
	deleteFileCommand = "rm";
#endif

	if (allSame != "only")
	{
		//执行准备脚本（准备脚本中有依赖库的调用）
		system(("sh" + SPACE + MARK(beforeBatPath) + SPACE + bit + SPACE + (allSame == "librely" ? "lib" : dlllib) + SPACE + debugRelease + SPACE + "same").c_str());
	}
	
	//编译工程
#ifdef _WIN32
	system((MARK(pyPath) + SPACE + bit + SPACE + dlllib + SPACE + debugRelease).c_str());
#elif __linux__
	system("mkdir" + SPACE + buildPath);
	system("cd" + SPACE + buildPath);
	system("cmake" + SPACE + "-DBIT=" + bit + SPACE + "-DDLLLIB=" + dlllib + SPACE + "-DDEBUGRELEASE=" + debugRelease + SPACE + buildPath);
	system("make" + SPACE + "-j8");
#endif

#ifdef _WIN32
	//向公共部分提供文件，有的时候可能没有inl文件
	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(srcPath + "*.h") + SPACE + MARK(includePath)).c_str());
	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(srcPath + "*.inl") + SPACE + MARK(includePath)).c_str());
#elif __linux__
	system(("cp" + SPACE + "-f" + SPACE + MARK(srcPath + "*.h") + SPACE + MARK(includePath)).c_str());
	system(("cp" + SPACE + "-f" + SPACE + MARK(srcPath + "*.inl") + SPACE + MARK(includePath)).c_str());
#endif

	//删除宏文件，并且替换特定的宏文件
	if (dlllib == "lib")
	{
		system((deleteFileCommand + SPACE + MARK(includePath + projectName + "Macro.h")).c_str());
		system((deleteFileCommand + SPACE + MARK(includePath + projectName + "MacroDll.h")).c_str());
		rename((includePath + projectName + "MacroLib.h").c_str(), (includePath + projectName + "Macro.h").c_str());
	}
	else if (dlllib == "dll")
	{
		system((deleteFileCommand + SPACE + MARK(includePath + projectName + "Macro.h")).c_str());
		system((deleteFileCommand + SPACE + MARK(includePath + projectName + "MacroLib.h")).c_str());
		rename((includePath + projectName + "MacroDll.h").c_str(), (includePath + projectName + "Macro.h").c_str());
	}

#ifdef _WIN32
	//删除临时文件
	system(("del" + SPACE + MARK(projectPath + "@AutomationLog.txt")).c_str());
	system(("del" + SPACE + "@AutomationLog.txt").c_str());
	system(("del" + SPACE + MARK(projectPath + "scripts" + PATHLEVEL + "@AutomationLog.txt")).c_str());
	system(("del" + SPACE + MARK(projectPath + "scripts" + PATHLEVEL + "msbuild.log")).c_str());
	std::string d;
	if (debugRelease != "release")
	{
		d = "d";
	}
	system(("del" + SPACE + MARK(xueyePath + "lib" + PATHLEVEL + projectName + d + ".ilk")).c_str());
	system(("del" + SPACE + MARK(xueyePath + "lib" + PATHLEVEL + projectName + d + ".exp")).c_str());
#elif __linux__
	system(("rm" + SPACE + "-rf" + SPACE + MARK(projectPath + "scripts" + PATHLEVEL + "build")).c_str());
#endif

	//执行后期脚本
	system(("sh" + SPACE + MARK(afterBatPath)).c_str());
	return 0;
}