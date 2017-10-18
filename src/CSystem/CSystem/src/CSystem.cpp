#include "CSystem.h"
#include <thread>
#include <objbase.h>

double CSystem::GetCPUSpeedGHz(){
	//?先是存放计时次数，后存放固定时间间隔值
	unsigned long int       ticks;          
	//?存放两固定时刻的CPU内置时钟值，值的含意为计数
	unsigned long int       stock0, stock1; 
	//?存放内置时钟值之差，好固定时段的计数值 
	unsigned long int       cycles;         
	//?存放频率，为了提高精度，采用了相邻的测的5个频率的平均值
	unsigned long int       freq[5] = {0,0,0,0,0};
	//?循环次数
	unsigned long int       nums = 0;   
	//?存放频率之和
	unsigned long int       total = 0;      
	LARGE_INTEGER       t0,t1;
	LARGE_INTEGER       countfreq;
	//?返回高精度的计数频率，即每秒多少次;
	if (!QueryPerformanceFrequency( &countfreq ) )
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
		while ( (unsigned long int)t1.LowPart - (unsigned long int)t0.LowPart<10)
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
		while ((unsigned long int)t1.LowPart-(unsigned long int)t0.LowPart<1000 )
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
		if ( ticks % countfreq.LowPart > countfreq.LowPart/2 )
		{
			//? 使数据收敛
			ticks++;           
		}
		//? 求出频率，单位：MHz
		freq[0] = cycles / ticks;    
		if ( cycles%ticks > ticks/2 )
		{
			//? 使数据收敛
			freq[0]++;             
		}
		total = (freq[0] + freq[1] + freq[2] + freq[3] + freq[4]);
	} while ((nums < 5 ) || (nums < 100) && ( (abs(5 * (long)freq[0] - (long)total) < 5)
		|| (abs(5 * (long)freq[1]-(long)total) < 5) || (abs(5 * (long)freq[2] - (long)total) < 5)
		|| (abs(5 * (long)freq[3]-(long)total) < 5) || (abs(5 * (long)freq[4] - (long)total) < 5)
		));
	//?条件循环，以确保循环不少于5次，在大于5次后确保达到一定的精度后退出
	if ( total/5  !=  ( total + 1 )/5 )
	{
		//? 使数据收敛
		total ++;               
	}
	//? 恢复进程及线程的优先级别;
	SetPriorityClass(GetCurrentProcess(), priority_class);
	SetThreadPriority(GetCurrentThread(), thread_priority);
	return double(total) / 5.0 / 1000.0;
}

RECT CSystem::GetTaskbarRect(){
	HWND h = ::FindWindow("Shell_TrayWnd","");
	RECT r;
	::GetWindowRect(h,&r);
	return r;
}

RECT CSystem::GetWindowResolution(){
	int screenwidth_real=GetSystemMetrics(SM_CXSCREEN);
	int screenheight_real=GetSystemMetrics(SM_CYSCREEN);
	RECT rectResult;
	rectResult.left = 0;
	rectResult.top = 0;
	rectResult.right = screenwidth_real;
	rectResult.bottom = screenheight_real;
	return rectResult;
}

int CSystem::GetVisibleHeight(){
	return GetSystemMetrics(SM_CYFULLSCREEN);
}

void CSystem::Sleep(long long milliseconds){
	std::chrono::milliseconds dura(milliseconds);
	std::this_thread::sleep_for(dura);
}

string CSystem::uuid(int flag){
	char buffer[64] = { 0 };
	GUID guid;
	if (CoCreateGuid(&guid)) return "";
	string strFormat = "%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X";
	if (flag == 0) strFormat = "%08X%04X%04x%02X%02X%02X%02X%02X%02X%02X%02X";
	_snprintf(buffer, sizeof(buffer),
		strFormat.c_str(),
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2],
		guid.Data4[3], guid.Data4[4], guid.Data4[5],
		guid.Data4[6], guid.Data4[7]);
	return buffer;
}

void CSystem::OpenFolder(const string& folder)
{
    CSystem::ForbidRedir();
    ShellExecute(NULL, "open", NULL, NULL, folder.c_str(), SW_SHOWNORMAL);
    CSystem::RecoveryRedir();
}

void CSystem::OpenFolderAndSelectFile(const string& file)
{
    CSystem::ForbidRedir();
    ShellExecute(NULL, "open", "Explorer.exe", ("/select, " + file).c_str(), NULL, SW_SHOWDEFAULT);
    CSystem::RecoveryRedir();
}

void CSystem::OpenFile(const string& file)
{
    CSystem::ForbidRedir();
    ShellExecute(NULL, "open", file.c_str(), NULL, NULL, SW_SHOWNORMAL);
    CSystem::RecoveryRedir();
}

void CSystem::OpenWebPage(const string& webPage)
{
    CSystem::ForbidRedir();
    ShellExecute(NULL, "open", webPage.c_str(), NULL, NULL, SW_SHOWNORMAL);
    CSystem::RecoveryRedir();
}

void CSystem::CopyFileOver(const string& dstFile, const string& srcFile, bool over)
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

string CSystem::GetSysUserName()
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

/*
int main(){
	//CSystem system;
	double x = CSystem::GetCPUSpeedGHz();
	RECT rect = CSystem::GetWindowResolution();
	int xx = rect.right - rect.left;
	int y = rect.bottom - rect.top;
	int sss = CSystem::GetVisibleHeight();
	int screenwidth=GetSystemMetrics(SM_CXFULLSCREEN);
	int screenheight=GetSystemMetrics(SM_CYFULLSCREEN);
	//?以下两个函数获取的是真正屏幕的大小，即实际的大小
	int screenwidth_real=GetSystemMetrics(SM_CXSCREEN);
	int screenheight_real=GetSystemMetrics(SM_CYSCREEN);
	CSystem::Sleep(1000);
	return 0;
}
*/