#include "CloudRebuild.h"
#include <string>
#ifdef __linux__
#include <unistd.h>
#include <vector>
#include <fstream>
#include <dirent.h>
#endif

#define SPACE std::string(" ")
#define MARK(src) ("\"" + src + "\"")
#ifdef _WIN32
#define PATHLEVEL std::string("\\")
#elif __linux__
#define PATHLEVEL std::string("/")
#endif

#ifdef __linux__

int32_t SystemCommand(const std::string& command, std::string& result, bool isShowCmd)
{
	if (command.size() == 0)
	{
		//command is empty
		return -1;
	}
	if (!isShowCmd)
	{
#ifdef _WIN32
		result.clear();
		SECURITY_ATTRIBUTES sa;
		HANDLE hRead, hWrite;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;
		if (!CreatePipe(&hRead, &hWrite, &sa, 0))
		{
			return -1;
		}

		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		si.cb = sizeof(STARTUPINFO);
		GetStartupInfoA(&si);
		si.hStdError = hWrite;
		si.hStdOutput = hWrite;
		si.wShowWindow = SW_HIDE;
		si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		//关键步骤，CreateProcess函数参数意义请查阅MSDN   
		if (!CreateProcessA(NULL, (char*)command.c_str(), NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
		{
			return -1;
		}
		CloseHandle(hWrite);
		char buffer[4096] = { 0 };
		DWORD bytesRead;
		while (true)
		{
			memset(buffer, 0, strlen(buffer));
			if (ReadFile(hRead, buffer, 4095, &bytesRead, NULL) == NULL)
			{
				break;
			}
			//buffer中就是执行的结果，可以保存到文本，也可以直接输出   
			result += buffer;
			Sleep(10);
		}
		return 0;
#endif
	}
	char buffer[4096] = {};
	std::string fresult;
#ifdef _WIN32
	FILE *pin = _popen(command.c_str(), "r");
#elif __linux__
	FILE *pin = popen(command.c_str(), "r");
#endif
	if (!pin)
	{
		//popen failed
		return -1;
	}
	result.clear();
	while (!feof(pin))
	{
		if (fgets(buffer, sizeof(buffer), pin) != nullptr)
		{
			fresult += buffer;
		}
	}
	result = fresult;
	//-1:pclose failed; else shell ret
#ifdef _WIN32
	return _pclose(pin);
#elif __linux__
	return pclose(pin);
#endif
}

int32_t GetCPUCoreCount()
{
#ifdef _WIN32
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
#elif __linux__
	std::string result;
	if (SystemCommand("grep 'processor' /proc/cpuinfo | sort -u | wc -l", result, true) == -1 || result.empty())
	{
		return 0;
	}
	result.pop_back();
	return atoi(result.c_str());
#endif
}

static void Split(std::vector<std::string>& result, const std::string& splitString, const std::string& separate_character)
{
	result.clear();
	//?分割字符串的长度,这样就可以支持如“,,”多字符串的分隔符
	size_t separate_characterLen = separate_character.length();
	size_t lastPosition = 0;
	int32_t index = -1;
	while (-1 != (index = (int32_t)splitString.find(separate_character, lastPosition)))
	{
		result.push_back(splitString.substr(lastPosition, index - lastPosition));
		lastPosition = index + separate_characterLen;
	}
	//?截取最后一个分隔符后的内容
	//?if (!lastString.empty()) //如果最后一个分隔符后还有内容就入队
	result.push_back(splitString.substr(lastPosition));
}

static void LoadTxtWithSplit(std::vector<std::vector<std::string>>& vectxt, const std::string& strPath, const std::string& strSplit)
{
	vectxt.clear();
	std::ifstream myfile(strPath);
	std::string strLine;
	std::vector<std::string> vecLine;
	while (getline(myfile, strLine))
	{
		Split(vecLine, strLine, strSplit);
		vectxt.push_back(vecLine);
	}
	myfile.close();
	myfile.clear();
}

//文件夹地址，文件列表
void FindDirFile(const char* dirPath,std::vector<std::string>& vec)
{
    struct dirent* pDirent = nullptr;
    DIR* pDir = opendir(dirPath);
    while ((pDirent = readdir(pDir)) != NULL)
	{
        vec.push_back(std::string(pDirent->d_name));
    }
    closedir(pDir);
}
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
	std::string cmakelistsPath = projectPath + "scripts" + PATHLEVEL;
	//linux下build目录
	std::string buildPath = cmakelistsPath + "build" + PATHLEVEL;
	//lib目录
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
		//执行准备脚本（准备脚本中有依赖库的调用）
		system(("bash" + SPACE + MARK(beforeBatPath) + SPACE + bit + SPACE + (allSame == "librely" ? "lib" : dlllib) + SPACE + debugRelease + SPACE + "same").c_str());
	}
	
	//编译工程
#ifdef _WIN32
	system((MARK(pyPath) + SPACE + bit + SPACE + dlllib + SPACE + debugRelease).c_str());
#elif __linux__
	std::vector<std::string> vecRely;
	std::vector<std::vector<std::string>> vectxt;
	LoadTxtWithSplit(vectxt, beforeBatPath, " ");
	int32_t index = -1;
	while(index++ != vectxt.size() - 1)
	{
		std::vector<std::string>& vecLine = vectxt[index];
		if(vecLine.empty() || vecLine[0] != "\"$CLOUD_REBUILD\"")
		{
			continue;
		}
		if(vecLine.size() == 1)
		{
			printf("error line shell = %s\n", beforeBatPath.c_str());
			sleep(50);
			continue;
		}
		if(vecLine[1] == "DllRelyTest")
		{
			continue;
		}
		vecRely.push_back(vecLine[1]);
	}
	std::string librariesRely;
	index = -1;
	while (index++ != vecRely.size() - 1)
	{
		std::string& rely = vecRely[index];
		std::string relyDll = "lib" + rely + d + ".so";
		std::string relyLib = "lib" + rely + d + ".a";
		if (::access((libPath + relyDll).c_str(), 0) == 0)
		{
			librariesRely = librariesRely + relyDll;
		}
		else if (::access((libPath + relyLib).c_str(), 0) == 0)
		{
			librariesRely = librariesRely + relyLib;
		}
		else
		{
			printf("%s\n", ("need " + relyDll + " or " + relyLib).c_str());
			sleep(50);
			continue;
		}
		if(index != vecRely.size() - 1)
		{
			librariesRely.append(";");
		}
	}

	system(("rm" + SPACE + "-f" + SPACE + MARK(libPath + "lib" + projectName + d + ".so")).c_str());
	system(("rm" + SPACE + "-f" + SPACE + MARK(libPath + "lib" + projectName + d + ".a")).c_str());
	system(("mkdir" + SPACE + "-p" + SPACE + MARK(buildPath)).c_str());
	char oldWorkPath[1024] = {};
	getcwd(oldWorkPath, 1024);
	chdir(buildPath.c_str());
	system(("cmake" + SPACE + "-DBIT=" + bit + SPACE + "-DDLLLIB=" + dlllib + SPACE + "-DDEBUGRELEASE=" + debugRelease + SPACE + "-DLIBRARIESRELY=" + MARK(librariesRely) + SPACE + MARK(cmakelistsPath)).c_str());
	if (projectName == "QtControls" || projectName == "DialogManager")
	{
		system(("make" + SPACE + "-j" + std::to_string(GetCPUCoreCount())).c_str());
	}
	else
	{
		system(("make" + SPACE + "-j").c_str());
	}
	chdir(oldWorkPath);
#endif

#ifdef _WIN32
	//向公共部分提供文件，有的时候可能没有inl文件
	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(srcPath + "*.h") + SPACE + MARK(includePath)).c_str());
	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(srcPath + "*.inl") + SPACE + MARK(includePath)).c_str());
#elif __linux__
	if (projectName != "DllRelyTest")
	{
		system(("mkdir" + SPACE + "-p" + SPACE + MARK(includePath)).c_str());
	}
	//linux下无法拷贝自定义目录下的文件
	system(("cp" + SPACE + "-f" + SPACE + MARK(srcPath) + "*.h" + SPACE + MARK(includePath)).c_str());
	std::vector<std::string> vecFile;
	FindDirFile(srcPath.c_str(), vecFile);
	bool hasInl = false;
	index = -1;
	while (index++ != vecFile.size() - 1)
	{
		std::string& fileName = vecFile[index];
		if (fileName.size() <= 4)
		{
			continue;
		}
		if (fileName.substr(fileName.size() - 4, 4) == ".inl")
		{
			hasInl = true;
			break;
		}
	}
	if (hasInl)
	{
		system(("cp" + SPACE + "-f" + SPACE + MARK(srcPath) + "*.inl" + SPACE + MARK(includePath)).c_str());
	}
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
	
	system(("del" + SPACE + MARK(xueyePath + "lib" + PATHLEVEL + projectName + d + ".ilk")).c_str());
	system(("del" + SPACE + MARK(xueyePath + "lib" + PATHLEVEL + projectName + d + ".exp")).c_str());
#elif __linux__
	system(("rm" + SPACE + "-rf" + SPACE + MARK(buildPath)).c_str());
#endif

	//执行后期脚本
	system(("bash" + SPACE + MARK(afterBatPath)).c_str());
	return 0;
}