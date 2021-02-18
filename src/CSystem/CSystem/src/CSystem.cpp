#include "CSystem.h"
#if (_MSC_VER >= 1800 || __unix__)
#include <thread>
#endif
#ifdef _WIN32
#include <objbase.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <shlobj.h>
#include <psapi.h>
#pragma comment(lib, "shell32.lib")
#pragma warning(disable: 4200)
#elif __unix__
#include <unistd.h>
#include <pwd.h>
#include <termios.h>
#include <sys/stat.h>
#include <algorithm>
#include <dirent.h>
#endif
#include <fstream>
#include <queue>
#include <iostream>
#include <iterator>

#ifdef _WIN32
RECT CSystem::GetTaskbarRect()
{
	HWND h = ::FindWindowA("Shell_TrayWnd", "");
	RECT r;
	::GetWindowRect(h, &r);
	return r;
}

RECT CSystem::GetWindowResolution()
{
	int screenwidth_real = ::GetSystemMetrics(SM_CXSCREEN);
	int screenheight_real = ::GetSystemMetrics(SM_CYSCREEN);
	RECT rectResult;
	rectResult.left = 0;
	rectResult.top = 0;
	rectResult.right = screenwidth_real;
	rectResult.bottom = screenheight_real;
	return rectResult;
}

POINT CSystem::taskbarRightBottomPoint()
{
	RECT rect = { 0, 0, 0, 0 };
	::GetWindowRect(::FindWindowA("Shell_TrayWnd", ""), &rect);
	RECT screenRect = { 0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN) };

	//分4种情况，任务栏可能出现在左侧上侧右侧底侧
	//右侧
	if (rect.left != screenRect.left)
	{
		POINT point = { rect.left, screenRect.bottom };
		return point;
	}
	//底侧
	else if (rect.top != screenRect.top)
	{
		POINT point = { screenRect.right, rect.top };
		return point;
	}
	//上侧左侧和其余情况一律从右下角弹出
	else
	{
		POINT point = { screenRect.right, screenRect.bottom };
		return point;
	}
}

POINT CSystem::screenCenterPoint()
{
	int32_t screenwidth_real = ::GetSystemMetrics(SM_CXSCREEN);
	int32_t screenheight_real = ::GetSystemMetrics(SM_CYSCREEN);
	POINT point = { screenwidth_real / 2, screenheight_real / 2 };
	return point;
}

void CSystem::setClipboardData(HWND hWnd, const std::string& str)
{
	//打开剪贴板
	if (::OpenClipboard(hWnd))
	{
		HANDLE hClip;
		char* pBuf;
		//清空剪贴板
		::EmptyClipboard();

		//写入数据
		hClip = ::GlobalAlloc(GMEM_MOVEABLE, str.size() + 1);
		pBuf = (char*)::GlobalLock(hClip);
		::strcpy(pBuf, str.c_str());
		//解锁
		::GlobalUnlock(hClip);
		//设置格式
		::SetClipboardData(CF_TEXT, hClip);

		//关闭剪贴板
		::CloseClipboard();
	}
}

std::string CSystem::GetClipboardData(HWND hWnd)
{
	char* pBuf = nullptr;
	//打开剪贴板
	if (::OpenClipboard(hWnd))
	{
		//判断格式是否是我们所需要
		if (::IsClipboardFormatAvailable(CF_TEXT))
		{
			HANDLE hClip;
			//读取数据  
			hClip = ::GetClipboardData(CF_TEXT);
			pBuf = (char*)::GlobalLock(hClip);
			::GlobalUnlock(hClip);
			::CloseClipboard();
		}
	}
	if (pBuf == nullptr)
	{
		return std::string();
	}
	return pBuf;
}

HWND CSystem::GetConsoleHwnd()
{
	const int32_t bufsize = 1024;
	//This is what is returned to the caller.
	HWND hwndFound;
	// Contains fabricated
	char pszNewWindowTitle[bufsize];
	// WindowTitle.
	// Contains original
	char pszOldWindowTitle[bufsize];
	// WindowTitle.
	// Fetch current window title.
	GetConsoleTitleA(pszOldWindowTitle, bufsize);
	// Format a "unique" NewWindowTitle.
	sprintf(pszNewWindowTitle, "%d/%d", GetTickCount(), GetCurrentProcessId());
	// Change current window title.
	SetConsoleTitleA(pszNewWindowTitle);
	// Ensure window title has been updated.
	Sleep(40);
	// Look for NewWindowTitle.
	hwndFound = FindWindowA(NULL, pszNewWindowTitle);
	// Restore original window title.
	SetConsoleTitleA(pszOldWindowTitle);
	return(hwndFound);
}

typedef struct WNDINFO
{
	HWND  hWnd;
	DWORD dwPid;
}WNDINFO;

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	WNDINFO* pInfo = (WNDINFO*)lParam;
	DWORD dwProcessId = 0;
	GetWindowThreadProcessId(hWnd, &dwProcessId);
	if (dwProcessId == pInfo->dwPid)
	{
		pInfo->hWnd = hWnd;
		return FALSE;
	}
	return TRUE;
}

HWND CSystem::GetHwndByProcessId(uint32_t dwProcessId)
{
	WNDINFO info = { 0 };
	info.hWnd = NULL;
	info.dwPid = dwProcessId;
	EnumWindows(EnumWindowsProc, (LPARAM)&info);
	return info.hWnd;
}

std::string CSystem::GetRegOcxPath(const std::string& classid)
{
	HKEY hKey;
	RegOpenKeyEx(HKEY_CLASSES_ROOT, ("CLSID\\{" + classid + "}\\InprocServer32").c_str(), 0, KEY_READ, &hKey);
	DWORD dwType = REG_SZ;
	LPBYTE lpData = new BYTE[1024];
	memset(lpData, 0, 1024);
	DWORD cbData = 1024;
	RegQueryValueEx(hKey, _T(""), NULL, &dwType, lpData, &cbData);
	std::string temp;
	temp = (char*)lpData;
	std::string result = temp.substr(temp.find_last_of('\\') + 1);
	delete[] lpData;
	return result;
}

void* CSystem::ForbidRedir()
{
	if (GetSystemBits() != 64)
	{
		return nullptr;
	}
	void* oldValue = nullptr;
	Wow64DisableWow64FsRedirection(&oldValue);
	return oldValue;
}

void CSystem::RecoveryRedir(void* oldValue)
{
	if (GetSystemBits() != 64)
	{
		return;
	}
	Wow64RevertWow64FsRedirection(oldValue);
}

int CSystem::GetVisibleHeight()
{
	return GetSystemMetrics(SM_CYFULLSCREEN);
}

void CSystem::OpenFolder(const std::string& folder)
{
	ShellExecuteA(NULL, "open", NULL, NULL, folder.c_str(), SW_SHOWNORMAL);
}

void CSystem::OpenFolderAndSelectFile(const std::string& file)
{
	ShellExecuteA(NULL, "open", "Explorer.exe", ("/select, " + file).c_str(), NULL, SW_SHOWDEFAULT);
}

void CSystem::OpenFile(const std::string& file)
{
	ShellExecuteA(NULL, "open", file.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void CSystem::OpenWebPage(const std::string& webPage)
{
	ShellExecuteA(NULL, "open", webPage.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

bool CSystem::isMouseLeftDown()
{
	return ((::GetAsyncKeyState(MOUSE_MOVED) & 0x8000) ? 1 : 0);
}

bool CSystem::isMouseRightDown()
{
	return ((::GetAsyncKeyState(MOUSE_EVENT) & 0x8000) ? 1 : 0);
}

bool CSystem::isMouseMidDown()
{
	return ((::GetAsyncKeyState(MOUSE_WHEELED) & 0x8000) ? 1 : 0);
}

bool CSystem::ShellCopy(const char* from, const char* dest)
{
	SHFILEOPSTRUCTA fileOp = { 0 };
	fileOp.wFunc = FO_COPY;
	char newFrom[MAX_PATH];
	_tcscpy_s(newFrom, from);
	newFrom[_tcsclen(from) + 1] = 0;
	fileOp.pFrom = newFrom;
	char newTo[MAX_PATH];
	_tcscpy_s(newTo, dest);
	newTo[_tcsclen(dest) + 1] = 0;
	fileOp.pTo = newTo;
	fileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
	return SHFileOperationA(&fileOp) == 0;
}

uint32_t CSystem::processFirstPid(const std::wstring& processNameW)
{
	if (processNameW.empty())
	{
		return 0;
	}
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	PROCESSENTRY32W pe = { sizeof(pe) };
	for (BOOL ret = Process32FirstW(hSnapshot, &pe); ret; ret = ::Process32NextW(hSnapshot, &pe))
	{
		if (std::wstring(pe.szExeFile) == processNameW)
		{
			::CloseHandle(hSnapshot);
			return pe.th32ProcessID;
		}
	}
	::CloseHandle(hSnapshot);
	return 0;
}

std::vector<uint32_t> CSystem::processPid(const std::wstring& processNameW)
{
	std::vector<uint32_t> result;
	if (processNameW.empty())
	{
		return result;
	}
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		return result;
	}
	PROCESSENTRY32W pe = { sizeof(pe) };
	for (BOOL ret = Process32FirstW(hSnapshot, &pe); ret; ret = ::Process32NextW(hSnapshot, &pe))
	{
		if (std::wstring(pe.szExeFile) == processNameW)
		{
			result.push_back(pe.th32ProcessID);
		}
	}
	::CloseHandle(hSnapshot);
	return result;
}

std::wstring CSystem::processNameW(uint32_t pid)
{
	std::wstring result;
	PROCESSENTRY32W pe32 = { 0 };
	HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot != INVALID_HANDLE_VALUE)
	{
		pe32.dwSize = sizeof(PROCESSENTRY32W);
		if (::Process32FirstW(snapshot, &pe32))
		{
			do{
				if (pe32.th32ProcessID == pid)
				{
					result = pe32.szExeFile;
					break;
				}
			} while (::Process32NextW(snapshot, &pe32));
		}
		::CloseHandle(snapshot);
	}
	return result;
}
#endif

#ifdef __unix__
static inline uint64_t get_cycle_count()
{
	unsigned int lo,hi;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return ((uint64_t)hi << 32) | lo;
}
#endif

double CSystem::GetCPUSpeedGHz()
{
#ifdef _WIN64
	//VC++默认只支持32位的内联汇编，若要支持64位，需要安装Intel C++ Compiler XE
	return 0;
#elif _WIN32
	//先是存放计时次数，后存放固定时间间隔值
	unsigned long int       ticks;
	//存放两固定时刻的CPU内置时钟值，值的含意为计数
	unsigned long int       stock0, stock1;
	//存放内置时钟值之差，好固定时段的计数值 
	unsigned long int       cycles;
	//存放频率，为了提高精度，采用了相邻的测的5个频率的平均值
	unsigned long int       freq[5] = { 0, 0, 0, 0, 0 };
	//循环次数
	unsigned long int       nums = 0;
	//存放频率之和
	unsigned long int       total = 0;
	LARGE_INTEGER       t0, t1;
	LARGE_INTEGER       countfreq;
	//返回高精度的计数频率，即每秒多少次;
	if (!QueryPerformanceFrequency(&countfreq))
	{
		return 0.0f;
	}
	//返回特定进程的优先级;
	DWORD priority_class = GetPriorityClass(GetCurrentProcess());
	//返回特定线程的优先级;
	int   thread_priority = GetThreadPriority(GetCurrentThread());
	//将当前进程设成实时进程;
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	//设定线程优先级;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	do
	{
		nums++;
		freq[4] = freq[3];
		freq[3] = freq[2];
		freq[2] = freq[1];
		freq[1] = freq[0];
		//返回高精度计数的值;
		QueryPerformanceCounter(&t0);
		t1.LowPart = t0.LowPart;
		t1.HighPart = t0.HighPart;
		//这句中的50和后面相同语句中的1000是一个经验值，起的作用是控制时间间隔，可以
		//调节这两个值来实现最佳时间间隔。
		while ((unsigned long int)t1.LowPart - (unsigned long int)t0.LowPart < 10)
		{
			QueryPerformanceCounter(&t1);
		}
		_asm
		{
			//启动读取CPU的内置时钟，其返回值是个64位的整数，高32到EDX，低32到EAX里
			rdtsc
				//高位部份在短暂时间内是不会有变化的，故无需读出对比
				mov stock0, EAX
		}
		// 重置初始时刻
		t0.LowPart = t1.LowPart;
		t0.HighPart = t1.HighPart;
		while ((unsigned long int)t1.LowPart - (unsigned long int)t0.LowPart < 1000)
		{
			QueryPerformanceCounter(&t1);
		}
		_asm
		{
			rdtsc
				mov  stock1, EAX
		}
		cycles = stock1 - stock0;
		ticks = (unsigned long int) t1.LowPart - (unsigned long int) t0.LowPart;
		ticks = ticks * 1000000;
		ticks = ticks / countfreq.LowPart;
		if (ticks % countfreq.LowPart > countfreq.LowPart / 2)
		{
			// 使数据收敛
			ticks++;
		}
		// 求出频率，单位：MHz
		freq[0] = cycles / ticks;
		if (cycles%ticks > ticks / 2)
		{
			// 使数据收敛
			freq[0]++;
		}
		total = (freq[0] + freq[1] + freq[2] + freq[3] + freq[4]);
	} while ((nums < 5) || (nums < 100) && ((abs(5 * (long)freq[0] - (long)total) < 5)
		|| (abs(5 * (long)freq[1] - (long)total) < 5) || (abs(5 * (long)freq[2] - (long)total) < 5)
		|| (abs(5 * (long)freq[3] - (long)total) < 5) || (abs(5 * (long)freq[4] - (long)total) < 5)
		));
	//条件循环，以确保循环不少于5次，在大于5次后确保达到一定的精度后退出
	if (total / 5 != (total + 1) / 5)
	{
		// 使数据收敛
		total++;
	}
	// 恢复进程及线程的优先级别;
	SetPriorityClass(GetCurrentProcess(), priority_class);
	SetThreadPriority(GetCurrentThread(), thread_priority);
	return double(total) / 5.0 / 1000.0;
#elif __unix__
	int delayms = 700;
	uint64_t oldTime = get_cycle_count();
	usleep(delayms * 1000);
	uint64_t newTime = get_cycle_count();
	return ((newTime - oldTime) / (delayms * 1000)) / 1000.0;
#endif
}

void CSystem::Sleep(long long milliseconds)
{
#if (_MSC_VER >= 1800 || __unix__)
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
}

std::string CSystem::uuid(int flag)
{
#ifdef _WIN32
	char buffer[64] = { 0 };
	GUID guid;
	if (CoCreateGuid(&guid))
	{
		return "";
	}
	std::string strFormat = "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x";
	if (flag == 0)
	{
		strFormat = "%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x";
	}
	_snprintf(buffer, sizeof(buffer),
		strFormat.c_str(),
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2],
		guid.Data4[3], guid.Data4[4], guid.Data4[5],
		guid.Data4[6], guid.Data4[7]);
	return buffer;
#elif __unix__
	std::string result = CSystem::readFile("/proc/sys/kernel/random/uuid");
	if(result.empty())
	{
		return "";
	}
	result.pop_back();
	if (flag == 0)
	{
		for(auto it = result.begin();it != result.end();)
		{
			if(*it == '-')
			{
				it = result.erase(it);
				continue;
			}
			++it;
		}
	}
	return result;
#endif
}

void CSystem::CopyFileOver(const std::string& dstFile, const std::string& srcFile, bool over)
{
#ifdef _WIN32
	::CopyFileA(srcFile.c_str(), dstFile.c_str(), over == false);
#elif __unix__
	if(CSystem::DirOrFileExist(dstFile) && !over)
	{
		return;
	}
	system(("cp -f " + srcFile + " " + dstFile).c_str());
#endif	
}

#ifdef _WIN32
//安全的取得真实系统信息  
VOID SafeGetNativeSystemInfo(__out LPSYSTEM_INFO lpSystemInfo)
{
    if (NULL == lpSystemInfo)
    {
        return;
    }
    typedef VOID(WINAPI *LPFN_GetNativeSystemInfo)(LPSYSTEM_INFO lpSystemInfo);
    LPFN_GetNativeSystemInfo fnGetNativeSystemInfo = (LPFN_GetNativeSystemInfo)GetProcAddress(GetModuleHandleA("kernel32"), "GetNativeSystemInfo");
    if (NULL != fnGetNativeSystemInfo)
    {
        fnGetNativeSystemInfo(lpSystemInfo);
    }
    else
    {
        GetSystemInfo(lpSystemInfo);
    }
}
#endif

int CSystem::GetSystemBits()
{
#ifdef _WIN32
    SYSTEM_INFO si;
    SafeGetNativeSystemInfo(&si);
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
        si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
    {
        return 64;
    }
    return 32;
#elif __unix__
	std::string result;
	if (SystemCommand("uname -a", result) == -1 || result.empty())
	{
		return 0;
	}
	if (result.find("x86_64") != -1)
	{
		return 64;
	}
	return 32;
#endif
}

void CSystem::OutputMap(const std::map<std::string, std::string>& stringMap, const std::string& path)
{
	std::ofstream file(path.c_str(), std::ios_base::binary | std::ios_base::app);
    for (auto itData = stringMap.begin(); itData != stringMap.end(); ++itData)
    {
		if (path.empty())
		{
			printf("[%s] = %s\n", itData->first.c_str(), itData->second.c_str());
		}
		else
		{
			file << "[" << itData->first.c_str() << "] = " << itData->second.c_str() << "\n";
		}
    }
	file.close();
}

void CSystem::OutputVector(const std::vector<std::string>& stringVector, const std::string& path)
{
	std::ofstream file(path.c_str(), std::ios_base::binary | std::ios_base::app);
    for (auto itData = stringVector.begin(); itData != stringVector.end(); ++itData)
    {
		if (path.empty())
		{
			printf("%s\n", itData->c_str());
		}
		else
		{
			file << itData->c_str() << "\n";
		}
    }
	file.close();
}

void CSystem::ClearScanf()
{
    char pathJar[1024] = {};
    scanf("%[^\n]", pathJar);
    scanf("%*[^\n]");
    scanf("%*c");
}

std::vector<std::string> CSystem::exeParam(int argc, char** argv)
{
	std::vector<std::string> result;
#ifdef _WIN32
	for (int32_t index = 0; index < __argc; ++index)
	{
		result.push_back(__argv[index]);
	}
#elif __unix__
for (int32_t index = 0; index < argc; ++index)
	{
		result.push_back(argv[index]);
	}
#endif
	return result;
}

#ifdef __unix__
int _getch()
{
    struct termios oldt, newt;
    int ch;
    tcgetattr( STDIN_FILENO, &oldt );
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
    return ch;
}
#endif

std::string CSystem::PasswordScanf()
{
#ifdef _WIN32
	char returnCode = '\r';
	char backspace = 8;
#elif __unix__
	char returnCode = '\n';
	char backspace = 127;
#endif
	std::string password;
	char ch;
	while ((ch = ::_getch()) != returnCode)
	{
		//不是Backspace就录入
		if (ch != backspace)
		{
			password.push_back(ch);
			//并且输出*号
			putchar('*');
		}
		else
		{
			//这里是删除一个，我们通过输出回撤符 /b，回撤一格，
			putchar('\b');
			//再显示空格符把刚才的*给盖住，
			putchar(' ');
			//然后再 回撤一格等待录入。
			putchar('\b');
			if (!password.empty())
			{
				password.pop_back();
			}
		}
	}
	printf("\n");
	return password;
}

int32_t CSystem::SystemCommand(const std::string& command, std::string& result, bool isShowCmd)
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
#elif __unix__
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
#elif __unix__
	return pclose(pin);
#endif
}

uint32_t CSystem::SystemThreadId()
{
#ifdef _WIN32
	return ::GetCurrentThreadId();
	//return ((_Thrd_t*)(char*)&(std::this_thread::get_id()))->_Id;
#elif __unix__
	std::thread::id threadId = std::this_thread::get_id();
	return (uint32_t)(*(__gthread_t*)(char*)(&threadId));
#endif
}

int32_t CSystem::GetCPUCoreCount()
{
#ifdef _WIN32
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
#elif __unix__
	std::string result;
	if (SystemCommand("grep 'processor' /proc/cpuinfo | sort -u | wc -l", result) == -1 || result.empty())
	{
		return 0;
	}
	result.pop_back();
	return atoi(result.c_str());
#endif
}

#ifdef __unix__
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
#endif

int32_t CSystem::GetSystemVersionNum()
{
#ifdef _WIN32
	DWORD dwVersion = 0;
	HMODULE hinstDLL = LoadLibraryExW(L"kernel32.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
	if (hinstDLL != NULL)
	{
		HRSRC hResInfo = FindResource(hinstDLL, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
		if (hResInfo != NULL)
		{
			HGLOBAL hResData = LoadResource(hinstDLL, hResInfo);
			if (hResData != NULL)
			{
				static const WCHAR wszVerInfo[] = L"VS_VERSION_INFO";
				struct VS_VERSIONINFO {
					WORD wLength;
					WORD wValueLength;
					WORD wType;
					WCHAR szKey[ARRAYSIZE(wszVerInfo)];
					VS_FIXEDFILEINFO Value;
					WORD Children[];
				} *lpVI = (struct VS_VERSIONINFO *)LockResource(hResData);
				if ((lpVI != NULL) && (lstrcmpiW(lpVI->szKey, wszVerInfo) == 0) && (lpVI->wValueLength > 0))
				{
					dwVersion = lpVI->Value.dwFileVersionMS;
				}
			}
		}
		FreeLibrary(hinstDLL);
	}
	return dwVersion;
#elif __unix__
	std::string result;
	if (SystemCommand("cat /etc/issue", result) == -1 || result.empty())
	{
		return 0;
	}
	result.pop_back();

	std::vector<std::string> vec;
	Split(vec, result, " ");
	if(vec.size() < 2)
	{
		return 0;
	}
	std::vector<std::string> vecOs;
	Split(vecOs, vec[1], ".");
	if(vecOs.size() < 2)
	{
		return 0;
	}
	return atoi((vecOs[0] + vecOs[1]).c_str());
#endif
}

uint32_t CSystem::currentProcessPid()
{
#ifdef _WIN32
	return GetCurrentProcessId();
#elif __unix__
	return getpid();
#endif
}

uint32_t CSystem::processFirstPid(const std::string& processName)
{
#ifdef _WIN32
	if (processName.empty())
	{
		return 0;
	}
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	PROCESSENTRY32 pe = { sizeof(pe) };
	for (BOOL ret = Process32First(hSnapshot, &pe); ret; ret = ::Process32Next(hSnapshot, &pe))
	{
		if (std::string(pe.szExeFile) == processName)
		{
			::CloseHandle(hSnapshot);
			return pe.th32ProcessID;
		}
	}
	::CloseHandle(hSnapshot);
	return 0;
#elif __unix__
	std::string result;
	if (SystemCommand("pidof " + processName, result) == -1 || result.empty())
	{
		return 0;
	}
	result.pop_back();
	std::vector<std::string> vecPid;
	Split(vecPid, result, " ");
	if(vecPid.empty())
	{
		return 0;
	}
	return atoi(vecPid[0].c_str());
#endif
}

std::vector<uint32_t> CSystem::processPid(const std::string& processName)
{
#ifdef _WIN32
	std::vector<uint32_t> result;
	if (processName.empty())
	{
		return result;
	}
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		return result;
	}
	PROCESSENTRY32 pe = { sizeof(pe) };
	for (BOOL ret = Process32First(hSnapshot, &pe); ret; ret = ::Process32Next(hSnapshot, &pe))
	{
		if (std::string(pe.szExeFile) == processName)
		{
			result.push_back(pe.th32ProcessID);
		}
	}
	::CloseHandle(hSnapshot);
	return result;
#elif __unix__
	std::vector<uint32_t> vecResult;
	std::string result;
	if (SystemCommand("pidof " + processName, result) == -1 || result.empty())
	{
		return vecResult;
	}
	result.pop_back();
	std::vector<std::string> vecPid;
	Split(vecPid, result, " ");
	if(vecPid.empty())
	{
		return vecResult;
	}
	int32_t index = -1;
	while (index++ != vecPid.size() - 1)
	{
		vecResult.push_back(atoi(vecPid[index].c_str()));
	}
	return vecResult;
#endif
}

std::string CSystem::processName(uint32_t pid)
{
#ifdef _WIN32
	std::string result;
	PROCESSENTRY32 pe32 = { 0 };
	HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot != INVALID_HANDLE_VALUE)
	{
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if (::Process32First(snapshot, &pe32))
		{
			do{
				if (pe32.th32ProcessID == pid)
				{
					result = pe32.szExeFile;
					break;
				}
			} while (::Process32Next(snapshot, &pe32));
		}
		::CloseHandle(snapshot);
	}
	return result;
#elif __unix__
	std::string result;
	if (SystemCommand("cat /proc/" + std::to_string(pid) + "/cmdline", result) == -1 || result.empty())
	{
		return "";
	}
	return CSystem::GetName(result, 3);
#endif
}

uint64_t CSystem::currentMemory()
{
#ifdef _MSC_VER
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(::GetCurrentProcess(), &pmc, sizeof(pmc));
	return pmc.WorkingSetSize;
#elif __unix__
	std::string stat_info = CSystem::readFile("/proc/self/status");
	int32_t first = stat_info.find("\nVmRSS:", 0);
	std::string memory = stat_info.substr(first + 7, stat_info.find_first_of('\n', first + 1) - first - 9);
	return strtoull(memory.c_str(), nullptr, 10) * 1024;
#endif
}

uint32_t CSystem::GetTickCount()
{
#ifdef _MSC_VER
	return ::GetTickCount();
#elif __unix__
	struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

std::string CSystem::getComputerName()
{
	char computerName[256] = {};
#ifdef _WIN32
	DWORD length = 256;
	GetComputerNameA(computerName, &length);
#elif __unix__
	gethostname(computerName, 256);
#endif
	return computerName;
}

std::string CSystem::GetCurrentExePath()
{
	char szFilePath[1024] = {};
#ifdef _WIN32
	::GetModuleFileNameA(NULL, szFilePath, 1024);
#elif __unix__
	::readlink("/proc/self/exe", szFilePath, 1024);
#endif
	return CSystem::GetName(szFilePath, 4);
}

std::string CSystem::GetCurrentExeName()
{
	char szFilePath[1024] = {};
#ifdef _WIN32
	::GetModuleFileNameA(NULL, szFilePath, 1024);
#elif __unix__
	::readlink("/proc/self/exe", szFilePath, 1024);
#endif
	return CSystem::GetName(szFilePath, 3);
}

std::string CSystem::GetSystemTempPath()
{
#ifdef _WIN32
	char szPath[MAX_PATH] = {};
	if (::SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, szPath) < 0)
	{
		return "";
	}
	return std::string(szPath) + "\\";
#elif __unix__
	return "/tmp/";
#endif
}

std::string CSystem::GetName(const std::string& path, int32_t flag)
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

bool CSystem::deleteFile(const char* path)
{
	return ::remove(path) == 0;
}

std::string CSystem::inputString(const std::string& tip)
{
	printf("%s\n", tip.c_str());
	std::string result;
	char ch = 0;
	while ((ch = std::cin.get()) != '\n')
	{
		result += ch;
	}
	return result;
}

void CSystem::killProcess(int32_t pid)
{
#ifdef _WIN32
	char command[256] = {};
	::_snprintf(command, 256, "taskkill /f /pid %d", pid);
	::WinExec(command, SW_HIDE);
#elif __unix__
	system(("kill -9 " + std::to_string(pid)).c_str());
#endif
}

bool CSystem::rename(const std::string& oldPath, const std::string& newPath)
{
	return ::rename(oldPath.c_str(), newPath.c_str()) == 0;
}

std::string CSystem::GetSysUserName()
{
#ifdef _WIN32
    DWORD size = 1024;
    char szName[1024] = {};
    ::GetUserNameA(szName, &size);
    return szName;
#elif __unix__
	uid_t userid;
    struct passwd* pwd;
    userid = getuid();
    pwd = getpwuid(userid);
    return pwd->pw_name;
#endif
}

std::string CSystem::GetEnvironment(const char* name)
{
	return ::getenv(name);
}

bool CSystem::CreateDir(const std::string& dir)
{
#ifdef _WIN32
	return _mkdir(dir.c_str()) == 0;
#elif __unix__
	return mkdir(dir.c_str() ,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#endif
}

bool CSystem::DestroyDir(const std::string& dir)
{
#ifdef _WIN32
	return _rmdir(dir.c_str()) == 0;
#elif __unix__
	return rmdir(dir.c_str()) == 0;
#endif
}

bool CSystem::DirOrFileExist(const std::string& dir)
{
#ifdef _WIN32
	return _access(dir.c_str(), 0) == 0;
#elif __unix__
	return access(dir.c_str(), 0) == 0;
#endif
}

std::string CSystem::timetToStr(time_t timet, bool isLocal)
{
	char buf[20];
	buf[19] = 0;
	if (isLocal)
	{
		//转为本地时间
		tm* local = localtime(&timet);
		sprintf(buf,
			"%04d-%02d-%d %02d:%02d:%02d",
			local->tm_year + 1900,
			local->tm_mon + 1,
			local->tm_mday,
			local->tm_hour,
			local->tm_min,
			local->tm_sec);
	}
	else
	{
		//转为格林威治时间
		strftime(buf, 64, "%Y-%m-%d %H:%M:%S", gmtime(&timet));
	}
	return buf;
}

std::string CSystem::commonFile(const std::string& name)
{
#ifdef _WIN32
	std::string path = CSystem::GetEnvironment("XUEYELINGSHUANG") + "common\\";
#elif __unix__
	std::string path = CSystem::GetEnvironment("XUEYELINGSHUANG") + "common/";
#endif
	double version = 0;
	std::string result;
	std::vector<std::string> vecPath = CSystem::findFilePath(path, 3);
	int32_t index = -1;
	while (index++ != vecPath.size() - 1)
	{
		const std::string& filePath = vecPath[index];
		std::string fileName = CSystem::GetName(filePath, 1);
		std::string fileSuffix = CSystem::GetName(filePath, 2);
#ifdef _WIN32
		if(fileSuffix != "exe")
		{
			continue;
		}
		fileName.pop_back();
		fileName.pop_back();
		fileName.pop_back();
		fileName.pop_back();
#elif __unix__
		if (std::count(fileName.begin(), fileName.end(), '.') != 1)
		{
			continue;
		}
#endif
		if (fileName.find(name) == 0)
		{
			std::string strVersion = fileName.substr(name.size(), fileName.size() - name.size());
			double curVersion = atof(strVersion.c_str());
			if (curVersion > version)
			{
				version = curVersion;
				result = filePath;
			}
		}
	}
	return result;
}

std::string CSystem::readFile(const std::string& path)
{
	std::ifstream file(path.c_str(), std::ios_base::in | std::ios_base::binary);
	if (!file.is_open())
	{
		return "";
	}
	std::string result((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	return result;
}

void CSystem::saveFile(const std::string& content, const std::string& path)
{
	std::ofstream file(path.c_str(), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	file << content;
	file.close();
}

std::vector<std::string> CSystem::findFilePath(const std::string& strPath,
	int32_t flag,
	const std::string& fileStr,
	const std::function<bool (const std::string&)>& EveryFilePath,
	std::vector<std::string>* unVisitPath)
{
	std::string dir = strPath;
	if (dir.empty())
	{
		dir = CSystem::GetCurrentExePath();
	}
#ifdef _WIN32
	char level = '\\';
#elif __unix__
	char level = '/';
#endif
	if (dir.back() != level)
	{
		dir.push_back(level);
	}
#ifdef _WIN32
	dir.push_back('*');
#endif

	if (unVisitPath != nullptr)
	{
		unVisitPath->clear();
	}

	std::vector<std::string> result;
#ifdef _WIN32
	_finddata_t fileDir;
#elif __unix__
	struct dirent *pDirent = nullptr;
#endif

	//用队列实现递归
	std::queue<std::string> queue_dir;
	queue_dir.push(std::string(dir));

	while (!queue_dir.empty())
	{
		std::string curDir = queue_dir.front();
		queue_dir.pop();
#ifdef _WIN32
		auto lfDir = _findfirst(curDir.c_str(), &fileDir);
#elif __unix__
		DIR* pDir = opendir(curDir.c_str());
#endif

		//如果是-1表示该文件夹不可访问
#ifdef _WIN32
		if (lfDir == -1)
#elif __unix__
		if (pDir == nullptr)
#endif
		{
			if (unVisitPath != nullptr)
			{
				curDir.pop_back();
				unVisitPath->emplace_back(curDir);
			}
			continue;
		}
#ifdef _WIN32
		while (_findnext(lfDir, &fileDir) == 0)
#elif __unix__
		while ((pDirent = readdir(pDir)) != 0)
#endif
		{
#ifdef _WIN32
			std::string strName = fileDir.name;
#elif __unix__
			std::string strName = pDirent->d_name;
#endif
			//是目录，加入队列
#ifdef _WIN32
			if ((fileDir.attrib >= 16 && fileDir.attrib <= 23) || (fileDir.attrib >= 48 && fileDir.attrib <= 55))
#elif __unix__
			if (pDirent->d_type == DT_DIR)
#endif
			{
				//去掉当前目录和上一级目录
				if (strName == "." || strName == "..")
				{
					continue;
				}
				//减去最后一个*号
				std::string tmpstr = curDir;
#ifdef _WIN32
				tmpstr.pop_back();
#endif
				tmpstr.append(strName);
				//把当前目录放到队列中以便下一次遍历
				tmpstr.push_back(level);
#ifdef _WIN32
				tmpstr.push_back('*');
#endif
				queue_dir.emplace(tmpstr);
				continue;
			}
			bool isExit = false;
			std::string tmpfilename = curDir;
#ifdef _WIN32
			tmpfilename.pop_back();
#endif
			switch (flag)
			{
				//1表示找文件全名，带后缀名
			case 1:
			{
				if (strName == fileStr)
				{
					tmpfilename.append(strName);
					if (EveryFilePath != nullptr)
					{
						isExit = EveryFilePath(tmpfilename);
					}
					result.emplace_back(tmpfilename);
				}
			}
			break;
			//2表示找文件后缀名
			case 2:
			{
				std::string lowerFileStr;
				std::transform(fileStr.begin(), fileStr.end(), std::back_inserter(lowerFileStr), ::tolower);
				std::string nameSuffix = CSystem::GetName(strName, 2);
				std::string lowerNameSuffix;
				std::transform(nameSuffix.begin(), nameSuffix.end(), std::back_inserter(lowerNameSuffix), ::tolower);
				if (lowerFileStr == lowerNameSuffix)
				{
					tmpfilename.append(strName);
					if (EveryFilePath != nullptr)
					{
						isExit = EveryFilePath(tmpfilename);
					}
					result.emplace_back(tmpfilename);
				}
			}
			break;
			//3表示查找所有文件，不做过滤全部添加进来
			case 3:
			{
				tmpfilename.append(strName);
				if (EveryFilePath != nullptr)
				{
					isExit = EveryFilePath(tmpfilename);
				}
				result.emplace_back(tmpfilename);
			}
			break;
			default:
				break;
			}
			if (isExit)
			{
				break;
			}
		}
#ifdef _WIN32
		_findclose(lfDir);
#elif __unix__
		closedir(pDir);
#endif
	}
	return result;
}

//int main()
//{
//	auto sss2 = CSystem::commonFile("FileReplace");
//	CSystem::setClipboardData(CSystem::GetConsoleHwnd(), "12as34");
//	//CSystem system;
//	double x = CSystem::GetCPUSpeedGHz();
//	RECT rect = CSystem::GetWindowResolution();
//	int xx = rect.right - rect.left;
//	int y = rect.bottom - rect.top;
//	int sss = CSystem::GetVisibleHeight();
//	int screenwidth=GetSystemMetrics(SM_CXFULLSCREEN);
//	int screenheight=GetSystemMetrics(SM_CYFULLSCREEN);
//	//以下两个函数获取的是真正屏幕的大小，即实际的大小
//	int screenwidth_real=GetSystemMetrics(SM_CXSCREEN);
//	int screenheight_real=GetSystemMetrics(SM_CYSCREEN);
//	CSystem::Sleep(1000);
//	return 0;
//}