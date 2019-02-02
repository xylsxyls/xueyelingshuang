#include "CSystem.h"
#if (_MSC_VER >= 1800)
#include <thread>
#endif
#include <objbase.h>
#include <direct.h>
#include <io.h>
#include <fstream>
#include <conio.h>
#include <tlhelp32.h>
#include <tchar.h>
#pragma comment(lib, "shell32.lib")
#pragma warning(disable: 4200)

double CSystem::GetCPUSpeedGHz()
{
#ifdef _WIN64
	return 0;
#elif _WIN32
	//?先是存放计时次数，后存放固定时间间隔值
	unsigned long int       ticks;
	//?存放两固定时刻的CPU内置时钟值，值的含意为计数
	unsigned long int       stock0, stock1;
	//?存放内置时钟值之差，好固定时段的计数值 
	unsigned long int       cycles;
	//?存放频率，为了提高精度，采用了相邻的测的5个频率的平均值
	unsigned long int       freq[5] = { 0, 0, 0, 0, 0 };
	//?循环次数
	unsigned long int       nums = 0;
	//?存放频率之和
	unsigned long int       total = 0;
	LARGE_INTEGER       t0, t1;
	LARGE_INTEGER       countfreq;
	//?返回高精度的计数频率，即每秒多少次;
	if (!QueryPerformanceFrequency(&countfreq))
	{
		return 0.0f;
	}
	//?返回特定进程的优先级;
	DWORD priority_class = GetPriorityClass(GetCurrentProcess());
	//?返回特定线程的优先级;
	int   thread_priority = GetThreadPriority(GetCurrentThread());
	//?将当前进程设成实时进程;
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	//?设定线程优先级;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	do
	{
		nums++;
		freq[4] = freq[3];
		freq[3] = freq[2];
		freq[2] = freq[1];
		freq[1] = freq[0];
		//?返回高精度计数的值;
		QueryPerformanceCounter(&t0);
		t1.LowPart = t0.LowPart;
		t1.HighPart = t0.HighPart;
		//?这句中的50和后面相同语句中的1000是一个经验值，起的作用是控制时间间隔，可以
		//?调节这两个值来实现最佳时间间隔。
		while ((unsigned long int)t1.LowPart - (unsigned long int)t0.LowPart < 10)
		{
			QueryPerformanceCounter(&t1);
		}
		_asm
		{
			//?启动读取CPU的内置时钟，其返回值是个64位的整数，高32到EDX，低32到EAX里
			rdtsc
				//?高位部份在短暂时间内是不会有变化的，故无需读出对比
				mov stock0, EAX
		}
		//? 重置初始时刻
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
			//? 使数据收敛
			ticks++;
		}
		//? 求出频率，单位：MHz
		freq[0] = cycles / ticks;
		if (cycles%ticks > ticks / 2)
		{
			//? 使数据收敛
			freq[0]++;
		}
		total = (freq[0] + freq[1] + freq[2] + freq[3] + freq[4]);
	} while ((nums < 5) || (nums < 100) && ((abs(5 * (long)freq[0] - (long)total) < 5)
		|| (abs(5 * (long)freq[1] - (long)total) < 5) || (abs(5 * (long)freq[2] - (long)total) < 5)
		|| (abs(5 * (long)freq[3] - (long)total) < 5) || (abs(5 * (long)freq[4] - (long)total) < 5)
		));
	//?条件循环，以确保循环不少于5次，在大于5次后确保达到一定的精度后退出
	if (total / 5 != (total + 1) / 5)
	{
		//? 使数据收敛
		total++;
	}
	//? 恢复进程及线程的优先级别;
	SetPriorityClass(GetCurrentProcess(), priority_class);
	SetThreadPriority(GetCurrentThread(), thread_priority);
	return double(total) / 5.0 / 1000.0;
#endif
}

RECT CSystem::GetTaskbarRect()
{
	HWND h = ::FindWindow("Shell_TrayWnd","");
	RECT r;
	::GetWindowRect(h,&r);
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

int CSystem::GetVisibleHeight()
{
	return GetSystemMetrics(SM_CYFULLSCREEN);
}

void CSystem::Sleep(long long milliseconds)
{
#if (_MSC_VER >= 1800)
	std::chrono::milliseconds dura(milliseconds);
	std::this_thread::sleep_for(dura);
#endif
}

std::string CSystem::uuid(int flag)
{
	char buffer[64] = { 0 };
	GUID guid;
	if (CoCreateGuid(&guid)) return "";
	std::string strFormat = "%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X";
	if (flag == 0) strFormat = "%08X%04X%04x%02X%02X%02X%02X%02X%02X%02X%02X";
	_snprintf(buffer, sizeof(buffer),
		strFormat.c_str(),
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2],
		guid.Data4[3], guid.Data4[4], guid.Data4[5],
		guid.Data4[6], guid.Data4[7]);
	return buffer;
}

void CSystem::OpenFolder(const std::string& folder)
{
    CSystem::ForbidRedir();
    ShellExecute(NULL, "open", NULL, NULL, folder.c_str(), SW_SHOWNORMAL);
    CSystem::RecoveryRedir();
}

void CSystem::OpenFolderAndSelectFile(const std::string& file)
{
    CSystem::ForbidRedir();
    ShellExecute(NULL, "open", "Explorer.exe", ("/select, " + file).c_str(), NULL, SW_SHOWDEFAULT);
    CSystem::RecoveryRedir();
}

void CSystem::OpenFile(const std::string& file)
{
    CSystem::ForbidRedir();
    ShellExecute(NULL, "open", file.c_str(), NULL, NULL, SW_SHOWNORMAL);
    CSystem::RecoveryRedir();
}

void CSystem::OpenWebPage(const std::string& webPage)
{
    CSystem::ForbidRedir();
    ShellExecute(NULL, "open", webPage.c_str(), NULL, NULL, SW_SHOWNORMAL);
    CSystem::RecoveryRedir();
}

void CSystem::CopyFileOver(const std::string& dstFile, const std::string& srcFile, bool over)
{
    CSystem::ForbidRedir();
	::CopyFile(srcFile.c_str(), dstFile.c_str(), over == false);
    CSystem::RecoveryRedir();
}

//安全的取得真实系统信息  
VOID SafeGetNativeSystemInfo(__out LPSYSTEM_INFO lpSystemInfo)
{
    if (NULL == lpSystemInfo)
    {
        return;
    }
    typedef VOID(WINAPI *LPFN_GetNativeSystemInfo)(LPSYSTEM_INFO lpSystemInfo);
    LPFN_GetNativeSystemInfo fnGetNativeSystemInfo = (LPFN_GetNativeSystemInfo)GetProcAddress(GetModuleHandle("kernel32"), "GetNativeSystemInfo");
    if (NULL != fnGetNativeSystemInfo)
    {
        fnGetNativeSystemInfo(lpSystemInfo);
    }
    else
    {
        GetSystemInfo(lpSystemInfo);
    }
}

int CSystem::GetSystemBits()
{
    SYSTEM_INFO si;
    SafeGetNativeSystemInfo(&si);
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
        si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
    {
        return 64;
    }
    return 32;
}

void CSystem::OutputMap(const std::map<std::string, std::string>& stringMap, const std::string& path)
{
	std::ofstream file(path.c_str(), std::ios::app);
    for (auto itData = stringMap.begin(); itData != stringMap.end(); ++itData)
    {
		if (path == "")
		{
			printf("[%s] = %s\n", itData->first.c_str(), itData->second.c_str());
		}
		else
		{
			file << "[" << itData->first.c_str() << "] = " << itData->second.c_str() << "\n";
		}
    }
}

void CSystem::OutputVector(const std::vector<std::string>& stringVector, const std::string& path)
{
	std::ofstream file(path.c_str(), std::ios::app);
    for (auto itData = stringVector.begin(); itData != stringVector.end(); ++itData)
    {
		if (path == "")
		{
			printf("%s\n", itData->c_str());
		}
		else
		{
			file << itData->c_str() << "\n";
		}
    }
}

void CSystem::ClearScanf()
{
    char pathJar[1024] = {};
    scanf("%[^\n]", pathJar);
    scanf("%*[^\n]");
    scanf("%*c");
}

std::vector<std::string> CSystem::exeParam()
{
	std::vector<std::string> result;
	for (int32_t index = 0; index < __argc; ++index)
	{
		result.push_back(__argv[index]);
	}
	return result;
}

std::string CSystem::PasswordScanf()
{
	std::string password;
	char ch;
	while ((ch = ::getch()) != '\r')
	{
		if (ch != 8)//不是回撤就录入
		{
			password.push_back(ch);
			putchar('*');//并且输出*号
		}
		else
		{
			putchar('\b');//这里是删除一个，我们通过输出回撤符 /b，回撤一格，
			putchar(' ');//再显示空格符把刚才的*给盖住，
			putchar('\b');//然后再 回撤一格等待录入。
		}
	}
	return password;
}

uint32_t CSystem::SystemThreadId()
{
#ifdef _WIN32
	return ::GetCurrentThreadId();
#else
	return ((_Thrd_t*)(char*)&(std::this_thread::get_id()))->_Id;
#endif
}

int32_t CSystem::GetCPUCoreCount()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
}

bool CSystem::ShellCopy(const char* from, const char* dest)
{
	SHFILEOPSTRUCT fileOp = { 0 };
	fileOp.wFunc = FO_COPY;
	TCHAR newFrom[MAX_PATH];
	_tcscpy_s(newFrom, from);
	newFrom[_tcsclen(from) + 1] = 0;
	fileOp.pFrom = newFrom;
	TCHAR newTo[MAX_PATH];
	_tcscpy_s(newTo, dest);
	newTo[_tcsclen(dest) + 1] = 0;
	fileOp.pTo = newTo;
	fileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
	return SHFileOperation(&fileOp) == 0;
}

int32_t CSystem::GetSystemVersionNum()
{
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
}

int32_t CSystem::processPid(const std::string processName)
{
	if (processName.empty())
	{
		return GetCurrentProcessId();
	}
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hSnapshot)
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
}

std::string CSystem::processName(int32_t pid)
{
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
}

std::string CSystem::getComputerName()
{
	char computerName[256] = {};
	DWORD length = 256;
	GetComputerNameA(computerName, &length);
	return computerName;
}

std::string CSystem::GetCurrentExePath()
{
	char szFilePath[1024] = {};
	::GetModuleFileName(NULL, szFilePath, 1024);
	return CSystem::GetName(szFilePath, 4);
}

std::string CSystem::GetCurrentExeName()
{
	char szFilePath[1024] = {};
	::GetModuleFileName(NULL, szFilePath, 1024);
	return CSystem::GetName(szFilePath, 3);
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

bool CSystem::ifRedirFrobid = false;
PVOID CSystem::oldValue = nullptr;

void CSystem::ForbidRedir()
{
    if (ifRedirFrobid == false && GetSystemBits() == 64)
    {
        Wow64DisableWow64FsRedirection(&oldValue);
        ifRedirFrobid = true;
    }
}

void CSystem::RecoveryRedir()
{
    if (ifRedirFrobid == true && GetSystemBits() == 64)
    {
        Wow64RevertWow64FsRedirection(oldValue);
        ifRedirFrobid = false;
    }
}

std::string CSystem::GetSysUserName()
{
    DWORD size = 1024;
    TCHAR szName[1024] = {};
    ::GetUserName(szName, &size);
    return szName;
}

int CSystem::GetCPUCount()
{
	SYSTEM_INFO si;
	::GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
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

std::string CSystem::GetEnvironment(const char* name)
{
	return ::getenv(name);
}

bool CSystem::CreateDir(const std::string& dir)
{
	return _mkdir(dir.c_str()) == 0;
}

bool CSystem::DestroyDir(const std::string& dir)
{
	return _rmdir(dir.c_str()) == 0;
}

bool CSystem::DirOrFileAccess(const std::string& dir)
{
	return _access(dir.c_str(), 0) == 0;
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
	GetConsoleTitle(pszOldWindowTitle, bufsize);
	// Format a "unique" NewWindowTitle.
	wsprintf(pszNewWindowTitle, "%d/%d", GetTickCount(), GetCurrentProcessId());
	// Change current window title.
	SetConsoleTitle(pszNewWindowTitle);
	// Ensure window title has been updated.
	Sleep(40);
	// Look for NewWindowTitle.
	hwndFound = FindWindow(NULL, pszNewWindowTitle);
	// Restore original window title.
	SetConsoleTitle(pszOldWindowTitle);
	return(hwndFound);
}

//int main()
//{
//	CSystem::setClipboardData(CSystem::GetConsoleHwnd(), "12as34");
//	//CSystem system;
//	double x = CSystem::GetCPUSpeedGHz();
//	RECT rect = CSystem::GetWindowResolution();
//	int xx = rect.right - rect.left;
//	int y = rect.bottom - rect.top;
//	int sss = CSystem::GetVisibleHeight();
//	int screenwidth=GetSystemMetrics(SM_CXFULLSCREEN);
//	int screenheight=GetSystemMetrics(SM_CYFULLSCREEN);
//	//?以下两个函数获取的是真正屏幕的大小，即实际的大小
//	int screenwidth_real=GetSystemMetrics(SM_CXSCREEN);
//	int screenheight_real=GetSystemMetrics(SM_CYSCREEN);
//	CSystem::Sleep(1000);
//	return 0;
//}