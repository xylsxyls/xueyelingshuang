#include "CDump.h"
#include <stdint.h>
#ifdef _MSC_VER
#include "mdump.h"
#elif __unix__
#include <execinfo.h>
#include <fstream>
#include <signal.h>
#include <limits.h>
#include <regex>
#include <iomanip>
#include <unistd.h>
#endif

static bool g_init = false;

static std::string GetName(const std::string& path, int32_t flag)
{
	int32_t left = (int32_t)path.find_last_of("/\\");
	std::string name = path.substr(left + 1, path.length() - left - 1);
	int32_t point = (int32_t)name.find_last_of(".");
	switch (flag)
	{
	case 1:
	{
		return name;
	}
	case 2:
	{
		return name.substr(point + 1, point == -1 ? 0 : name.length() - point - 1);
	}
	case 3:
	{
		return name.substr(0, point == -1 ? name.length() : point);
	}
	case 4:
	{
		return path.substr(0, left + 1);
	}
	default:
		return "";
	}
}

static std::string GetCurrentExePath()
{
	char szFilePath[1024] = {};
#ifdef _WIN32
	::GetModuleFileNameA(NULL, szFilePath, 1024);
#elif __unix__
	int ignore __attribute__((unused)) = ::readlink("/proc/self/exe", szFilePath, 1024);
#endif
	return GetName(szFilePath, 4);
}

static std::string GetCurrentExeName()
{
	char szFilePath[1024] = {};
#ifdef _WIN32
	::GetModuleFileNameA(NULL, szFilePath, 1024);
	return GetName(szFilePath, 3);
#elif __unix__
	int ignore __attribute__((unused)) = ::readlink("/proc/self/exe", szFilePath, 1024);
	return GetName(szFilePath, 1);
#endif
}

#ifdef __unix__

static char s_dumpFileDir[512] = {};
static char s_currentExeName[128] = {};
static char s_currentExePath[512] = {};

static std::vector<std::string> Split(const std::string& splitString, const std::string& separate_character)
{
	std::vector<std::string> strs;
	size_t separate_characterLen = separate_character.length();
	size_t lastPosition = 0;
	int32_t index = -1;
	while (-1 != (index = (int32_t)splitString.find(separate_character, lastPosition)))
	{
		strs.push_back(splitString.substr(lastPosition, index - lastPosition));
		lastPosition = index + separate_characterLen;	 
	}
	strs.push_back(splitString.substr(lastPosition));
	return strs;
}

static size_t Replace(std::string& str, const std::string& oldstr, const std::string& newstr)
{
	size_t count = 0;
	size_t pos = 0;
	while (true)
	{
		pos = str.find(oldstr, pos);
		if (pos == std::string::npos)
		{
			break;
		}
		str.replace(pos, oldstr.length(), newstr);
		pos += newstr.length();
		++count;
	}
	return count;
}

static std::string GetCurrentExeMap()
{
	std::ostringstream oss;
	oss << "/proc/" << getpid() << "/maps";
	std::string filePath = oss.str();

	std::ifstream file(filePath);
	if (!file)
	{
		return std::string();
	}

	std::ostringstream result;
	result << file.rdbuf();

	return result.str();
}

static std::string GetRealPath(const std::string& symlinkPath)
{
	char realPath[PATH_MAX] = {};
	if (realpath(symlinkPath.c_str(), realPath))
	{
		return std::string(realPath);
	}
	else
	{
		// realpath failed, return an empty string
		return std::string();
	}
}

static std::string HexSubtraction(const std::string& hex1, const std::string& hex2)
{
	// 去除字符串前面的空格和0
	auto trimLeft = [](const std::string& str)
	{
		size_t startpos = str.find_first_not_of(" 0");
		return (startpos == std::string::npos) ? "" : str.substr(startpos);
	};

	std::string trimmedHex1 = trimLeft(hex1);
	std::string trimmedHex2 = trimLeft(hex2);

	if (trimmedHex1.empty())
	{
		trimmedHex1 = "0";
	}
	if (trimmedHex2.empty())
	{
		trimmedHex2 = "0";
	}

	unsigned long long num1 = std::stoull(trimmedHex1, 0, 16);
	unsigned long long num2 = std::stoull(trimmedHex2, 0, 16);

	if (num1 < num2)
	{
		return std::string();
	}

	unsigned long long result = num1 - num2;

	std::stringstream ss;
	ss << std::hex << result;

	return ss.str();
}

static std::map<std::string, std::string> ParseProcMaps(const std::string& procMaps)
{
	std::istringstream iss(procMaps);
	std::string line;
	std::regex re("([0-9a-fA-F]+)-[0-9a-fA-F]+\\s.*\\s.*\\s.*\\s.*\\s(/.*)");
	std::map<std::string, std::string> libToAddr;

	while (std::getline(iss, line))
	{
		std::smatch match;
		if (std::regex_search(line, match, re))
		{
			std::string addr = match[1];
			std::string lib = match[2];
			if (HexSubtraction(addr, sizeof(&g_init) == 8 ? "100000000" : "10000").empty())
			{
				addr = "0";
			}
			if (libToAddr.find(lib) == libToAddr.end())
			{
				libToAddr[lib] = addr;
			}
		}
	}
	return libToAddr;
}

static std::vector<std::string> ParseBacktrace(const std::string& backtrace)
{
	std::regex re("(.*?)(?:\\((.*?)\\))?\\s*\\[(0x)?([0-9a-fA-F]+)\\]");
	std::smatch match;
	std::vector<std::string> result;

	if (std::regex_search(backtrace, match, re))
	{
		result.push_back(match[1]); // 路径
		result.push_back(match[2]); // 函数名
		result.push_back(match[4]); // 地址值，去掉0x
	}

	return result;
}

static std::string GetAddr2lineOutput(const std::string& command)
{
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
	if (!pipe)
	{
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
	{
		result += buffer.data();
	}
	return result;
}

static void DumpFun(int signalNum)
{
	std::string dumpFileDir = s_dumpFileDir;
	std::map<std::string, std::string> exeMaps = ParseProcMaps(GetCurrentExeMap());
	
	char szBaseName[128] = {};
	strncpy(szBaseName, (dumpFileDir.empty() ? (std::string(s_currentExePath) + "core-file_" + s_currentExeName).c_str() : (dumpFileDir + "core-file_" + s_currentExeName).c_str()), 128 - 1);
	size_t uBaseNameLen = strlen(szBaseName);

	time_t tNow = time(nullptr);
	strftime(szBaseName + uBaseNameLen, 128 - uBaseNameLen, "_%Y%m%d_%H%M%S.log", localtime(&tNow));

	std::string dumpFileName = szBaseName;

	std::ofstream dumpFile(dumpFileName, std::ios::app | std::ios::binary);

	for (auto it = exeMaps.begin(); it != exeMaps.end(); ++it)
	{
		dumpFile << "reference addr : " << it->first << " " << it->second << std::endl;
	}

	dumpFile << "signalNum = " << signalNum << std::endl;

	void* buffer[128] = {};
	int32_t nptrs = backtrace(buffer, 128);
	dumpFile << "backtrace() returned " << nptrs << " addresses" << std::endl;

	//backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO);

	char** strings = backtrace_symbols(buffer, nptrs);
	if (strings == nullptr)
	{
		dumpFile << "backtrace_symbols nullptr" << std::endl;
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}

	int32_t index = -1;
	while (index++ != nptrs - 1)
	{
		std::string stack = strings[index];
		std::vector<std::string> vecStack = ParseBacktrace(stack);
		if (vecStack.size() != 3)
		{
			dumpFile << "ParseBacktrace failed, stack = " << stack << std::endl;
			continue;
		}
		std::string realPath = GetRealPath(vecStack[0]);
		if (GetName(realPath, 1) == std::string(s_currentExeName))
		{
			//realPath = s_currentExeName;
			vecStack[0] = s_currentExeName;
		}
		bool isFindPath = true;
		if (realPath.empty() && !vecStack[0].empty())
		{
			realPath = vecStack[0];
			isFindPath = false;
		}
		std::string funOffsetAddr;
		if (isFindPath)
		{
			std::vector<std::string> vecFunOffsetAddr = Split(vecStack[1], "+");
			if (vecFunOffsetAddr.size() == 2)
			{
				funOffsetAddr = vecFunOffsetAddr[1];
			}
		}

		std::string offsetAddr = "0";
		std::string function;
		auto it = exeMaps.find(realPath);
		if (it == exeMaps.end())
		{
			//dumpFile << "find realPath failed, realPath: " << GetRealPath(vecStack[0]) << " stackPath: " << vecStack[0] << "[1]: " << vecStack[1] << std::endl;
			isFindPath = false;
			function = vecStack[1];
			size_t pos = function.find('+');
			if (pos != std::string::npos)
			{
    			function.insert(pos + 1, "0x");
			}
		}
		else
		{
			offsetAddr = HexSubtraction(vecStack[2], it->second);
		}
		
		std::string fileName;
		std::string line;

		if (isFindPath)
		{
			std::string addr2lineStr = GetAddr2lineOutput("addr2line -C -f -e " + realPath + " " + offsetAddr);
			std::vector<std::string> vecAddr2line = Split(addr2lineStr, "\n");
			if (vecAddr2line.size() >= 1)
			{
				function = vecAddr2line[0];
				if (!function.empty() && function.back() != ')')
				{
					function.push_back('(');
					function.push_back(')');
				}
			}
			if (vecAddr2line.size() >= 2)
			{
				std::vector<std::string> vecFileLine = Split(vecAddr2line[1], ":");
				if (vecFileLine.size() >= 1)
				{
					fileName = vecFileLine[0];
					Replace(fileName, "?", "");
				}
				if (vecFileLine.size() >= 2)
				{
					line = vecFileLine[1];
					Replace(line, "?", "");
				}
			}
		}
		
		dumpFile << "#" << index << " 0x" << std::setw(sizeof(&g_init) == 8 ? 16 : 8) << std::setfill('0') << vecStack[2];
		if (!function.empty())
		{
			dumpFile << " function: " << function;
		}
		if (!funOffsetAddr.empty() && isFindPath)
		{
			dumpFile << "+" << funOffsetAddr;
		}
		if (!fileName.empty())
		{
			dumpFile << " fileName: " << fileName;
		}
		if (!line.empty())
		{
			dumpFile << " line: " << line;
		}
		dumpFile << " from ";
		if (vecStack[0] != realPath && GetName(realPath, 1) != std::string(s_currentExeName))
		{
			dumpFile << vecStack[0] << " => ";
		}
		dumpFile << (GetName(realPath, 1) == std::string(s_currentExeName) ? s_currentExeName : realPath) << std::endl;
	}
	
	free(strings);

	signal(signalNum, SIG_DFL);
}
#endif

bool CDump::declareDumpFile(const std::string& dumpFileDir)
{
	if (g_init)
	{
		return false;
	}
#ifdef _MSC_VER
	WIN32DUMP::CMiniDumper::Enable(GetCurrentExeName().c_str(), false, (dumpFileDir.empty() ? GetCurrentExePath().c_str() : dumpFileDir.c_str()));
#elif __unix__
	//在进入signal回调函数时，有些系统会自动清空容器全局变量
	::memcpy(s_dumpFileDir, dumpFileDir.c_str(), dumpFileDir.size());
	std::string currentExeName = GetCurrentExeName();
	::memcpy(s_currentExeName, currentExeName.c_str(), currentExeName.size());
	std::string currentExePath = GetCurrentExePath();
	::memcpy(s_currentExePath, currentExePath.c_str(), currentExePath.size());

	signal(SIGSEGV, DumpFun);
	signal(SIGABRT, DumpFun);
#endif
	g_init = true;
	return true;
}
