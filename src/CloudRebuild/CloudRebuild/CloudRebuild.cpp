#include "CloudRebuild.h"
#include <string>
#ifdef __linux__
#include <unistd.h>
#endif

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
	
	//xueyelingshuang·��
	std::string xueyePath = getenv("XUEYELINGSHUANG");
	//����·��
	std::string projectPath = xueyePath + "src" + PATHLEVEL + projectName + PATHLEVEL;

	//׼���ű�·��
	std::string beforeBatPath = projectPath + "scripts" + PATHLEVEL + "before_" + projectName + ".sh";
	//���ڽű�·��
	std::string afterBatPath = projectPath + "scripts" + PATHLEVEL + "after_" + projectName + ".sh";

#ifdef _WIN32
	//32λ�Զ�������python�ű�·��
	std::string pyPath = projectPath + "scripts" + PATHLEVEL + "rebuild_" + projectName + ".py";
#elif __linux__
	//linux��CMakeLists�ļ�·��
	std::string cmakelistsPath = projectPath + "scripts" + PATHLEVEL;
	//linux��buildĿ¼
	std::string buildPath = cmakelistsPath + "build" + PATHLEVEL;
	//libĿ¼
	std::string libPath = xueyePath + "lib" + PATHLEVEL;
#endif

	std::string srcPath = projectPath + projectName + PATHLEVEL + "src" + PATHLEVEL;
	std::string includePath = xueyePath + "include" + PATHLEVEL + projectName + PATHLEVEL;

	std::string d;
	if (debugRelease != "release")
	{
		d = "d";
	}

	std::string deleteFileCommand;
#ifdef _WIN32
	deleteFileCommand = "del";
#elif __linux__
	deleteFileCommand = "rm";
#endif

	if (allSame != "only")
	{
		//ִ��׼���ű���׼���ű�����������ĵ��ã�
		system(("sh" + SPACE + MARK(beforeBatPath) + SPACE + bit + SPACE + (allSame == "librely" ? "lib" : dlllib) + SPACE + debugRelease + SPACE + "same").c_str());
	}
	
	//���빤��
#ifdef _WIN32
	system((MARK(pyPath) + SPACE + bit + SPACE + dlllib + SPACE + debugRelease).c_str());
#elif __linux__
	system(("rm" + SPACE + "-f" + SPACE + MARK(libPath + "lib" + projectName + d + (dlllib == "dll" ? ".so" : ".a"))).c_str());
	system(("mkdir" + SPACE + "-p" + SPACE + MARK(buildPath)).c_str());
	char oldWorkPath[1024] = {};
	getcwd(oldWorkPath, 1024);
	chdir(buildPath.c_str());
	system(("cmake" + SPACE + "-DBIT=" + bit + SPACE + "-DDLLLIB=" + dlllib + SPACE + "-DDEBUGRELEASE=" + debugRelease + SPACE + MARK(cmakelistsPath)).c_str());
	system(("make" + SPACE + "-j8").c_str());
	chdir(oldWorkPath);
#endif

#ifdef _WIN32
	//�򹫹������ṩ�ļ����е�ʱ�����û��inl�ļ�
	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(srcPath + "*.h") + SPACE + MARK(includePath)).c_str());
	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(srcPath + "*.inl") + SPACE + MARK(includePath)).c_str());
#elif __linux__
	if(projectName != "DllRelyTest")
	{
		system(("mkdir" + SPACE + "-p" + SPACE + MARK(includePath)).c_str());
	}
	system(("cp" + SPACE + "-f" + SPACE + MARK(srcPath) + "*.h" + SPACE + MARK(includePath)).c_str());
	system(("cp" + SPACE + "-f" + SPACE + MARK(srcPath) + "*.inl" + SPACE + MARK(includePath)).c_str());
#endif

	//ɾ�����ļ��������滻�ض��ĺ��ļ�
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
	//ɾ����ʱ�ļ�
	system(("del" + SPACE + MARK(projectPath + "@AutomationLog.txt")).c_str());
	system(("del" + SPACE + "@AutomationLog.txt").c_str());
	system(("del" + SPACE + MARK(projectPath + "scripts" + PATHLEVEL + "@AutomationLog.txt")).c_str());
	system(("del" + SPACE + MARK(projectPath + "scripts" + PATHLEVEL + "msbuild.log")).c_str());
	
	system(("del" + SPACE + MARK(xueyePath + "lib" + PATHLEVEL + projectName + d + ".ilk")).c_str());
	system(("del" + SPACE + MARK(xueyePath + "lib" + PATHLEVEL + projectName + d + ".exp")).c_str());
#elif __linux__
	system(("rm" + SPACE + "-rf" + SPACE + MARK(buildPath)).c_str());
#endif

	//ִ�к��ڽű�
	system(("sh" + SPACE + MARK(afterBatPath)).c_str());
	return 0;
}