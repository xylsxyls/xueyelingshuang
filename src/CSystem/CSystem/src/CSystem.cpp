#ifndef _WIN32
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#endif
#include "CSystem.h"
#include <cstring>
#include <system_error>
#if CSYSTEM_HAS_NATIVE_API
#include <mutex>
#include <algorithm>
#include <limits.h>
#ifdef _WIN32
#include <shellapi.h>
#include <sddl.h>
#include <fcntl.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#endif
#if (_MSC_VER >= 1800 || defined(__unix__) || defined(__APPLE__))
#include <thread>
#endif
#ifdef _MSC_VER
#include <objbase.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <shlobj.h>
#include <psapi.h>
#include <dshow.h>
#include <comdef.h>
#include <SetupAPI.h>
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma warning(disable: 4200)
#elif defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <utime.h>
#include <pwd.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <limits.h>
#include <algorithm>
#include <codecvt>
#include <dirent.h>
#include <dlfcn.h>
#include <locale>
#include <string.h>
#endif
#include <fstream>
#include <memory>
#include <utility>
#include <queue>
#include <iostream>
#include <iterator>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#ifdef _WIN32
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#elif defined(__APPLE__)
#include <Security/SecRandom.h>
#elif defined(__linux__) && !defined(__EMSCRIPTEN__)
#include <sys/syscall.h>
#include <linux/random.h>
#endif
#ifdef __APPLE__
#include <codecvt>
#include <locale>
#include <TargetConditionals.h>
#if !TARGET_OS_IPHONE
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <libproc.h>
#endif
#include <mach-o/dyld.h>
#include <uuid/uuid.h>
#endif
#if defined(__linux__)
#include <sys/syscall.h>
#endif
#if defined(__unix__) || defined(__APPLE__)
#include <signal.h>
#endif

#ifdef _MSC_VER
RECT CSystem::GetTaskbarRect()
{
    HWND h = ::FindWindowA("Shell_TrayWnd", "");
    RECT r = { 0, 0, 0, 0 };
    if (h != nullptr)
    {
        ::GetWindowRect(h, &r);
    }
    return r;
}
#endif

#ifdef _MSC_VER
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
#endif

#ifdef _MSC_VER
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
#endif

#ifdef _MSC_VER
POINT CSystem::screenCenterPoint()
{
    int32_t screenwidth_real = ::GetSystemMetrics(SM_CXSCREEN);
    int32_t screenheight_real = ::GetSystemMetrics(SM_CYSCREEN);
    POINT point = { screenwidth_real / 2, screenheight_real / 2 };
    return point;
}
#endif

#ifdef _MSC_VER
void CSystem::setClipboardData(HWND hWnd, const std::string& str)
{
    //打开剪贴板
    if (!::OpenClipboard(hWnd))
    {
        return;
    }

    //清空剪贴板
    if (!::EmptyClipboard())
    {
        ::CloseClipboard();
        return;
    }

    //写入数据
    HGLOBAL hClip = ::GlobalAlloc(GMEM_MOVEABLE, str.size() + 1);
    if (hClip == nullptr)
    {
        ::CloseClipboard();
        return;
    }
    char* pBuf = static_cast<char*>(::GlobalLock(hClip));
    if (pBuf == nullptr)
    {
        ::GlobalFree(hClip);
        ::CloseClipboard();
        return;
    }
    ::memcpy(pBuf, str.c_str(), str.size() + 1);
    //解锁
    ::GlobalUnlock(hClip);
    //设置格式，成功后剪贴板拥有hClip，失败时需要自己释放。
    if (::SetClipboardData(CF_TEXT, hClip) == nullptr)
    {
        ::GlobalFree(hClip);
    }

    //关闭剪贴板
    ::CloseClipboard();
}
#endif

#ifdef _MSC_VER
std::string CSystem::GetClipboardData(HWND hWnd)
{
    std::string result;
    //打开剪贴板
    if (::OpenClipboard(hWnd))
    {
        //判断格式是否是我们所需要
        if (::IsClipboardFormatAvailable(CF_TEXT))
        {
            //读取数据
            HANDLE hClip = ::GetClipboardData(CF_TEXT);
            if (hClip != nullptr)
            {
                char* pBuf = static_cast<char*>(::GlobalLock(hClip));
                if (pBuf != nullptr)
                {
                    result = pBuf;
                    ::GlobalUnlock(hClip);
                }
            }
        }
        ::CloseClipboard();
    }
    return result;
}
#endif

#ifdef _MSC_VER
HWND CSystem::GetConsoleHwnd()
{
    return ::GetConsoleWindow();
}
#endif

#ifdef _MSC_VER
HWND CSystem::GetHwndByProcessId(uint32_t dwProcessId)
{
    std::pair<uint32_t, HWND> query(dwProcessId, nullptr);
    ::EnumWindows(findWindowCallback, reinterpret_cast<LPARAM>(&query));
    return query.second;
}
#endif

#ifdef _MSC_VER
HWND CSystem::findVisibleWindowByProcessId(uint32_t processId)
{
    if (processId == 0)
    {
        return nullptr;
    }
    std::pair<uint32_t, HWND> query(processId, nullptr);
    EnumWindows(findVisibleWindowCallback, reinterpret_cast<LPARAM>(&query));
    return query.second;
}
#endif

#ifdef _MSC_VER
std::string CSystem::GetRegOcxPath(const std::string& classid)
{
    HKEY hKey = nullptr;
    LONG openRet = ::RegOpenKeyExA(HKEY_CLASSES_ROOT, ("CLSID\\{" + classid + "}\\InprocServer32").c_str(), 0, KEY_READ, &hKey);
    if (openRet != ERROR_SUCCESS)
    {
        return "";
    }
    DWORD dwType = REG_SZ;
    char data[1024] = {};
    DWORD cbData = sizeof(data);
    LONG queryRet = ::RegQueryValueExA(hKey, "", nullptr, &dwType, reinterpret_cast<LPBYTE>(data), &cbData);
    ::RegCloseKey(hKey);
    if (queryRet != ERROR_SUCCESS || (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
    {
        return "";
    }
    data[sizeof(data) - 1] = '\0';
    std::string temp = data;
    size_t position = temp.find_last_of("\\/");
    if (position == std::string::npos)
    {
        return "";
    }
    std::string result = temp.substr(0, position + 1);
    return result;
}
#endif

#ifdef _MSC_VER
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
#endif

#ifdef _MSC_VER
void CSystem::RecoveryRedir(void* oldValue)
{
    if (GetSystemBits() != 64)
    {
        return;
    }
    Wow64RevertWow64FsRedirection(oldValue);
}
#endif

#ifdef _MSC_VER
int CSystem::GetVisibleHeight()
{
    return GetSystemMetrics(SM_CYFULLSCREEN);
}
#endif

#ifdef _MSC_VER
void CSystem::OpenWebPage(const std::string& webPage)
{
    ShellExecuteA(NULL, "open", webPage.c_str(), NULL, NULL, SW_SHOWNORMAL);
}
#endif

#ifdef _MSC_VER
bool CSystem::isMouseLeftDown()
{
    return ((::GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0);
}
#endif

#ifdef _MSC_VER
bool CSystem::isMouseRightDown()
{
    return ((::GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? 1 : 0);
}
#endif

#ifdef _MSC_VER
bool CSystem::isMouseMidDown()
{
    return ((::GetAsyncKeyState(VK_MBUTTON) & 0x8000) ? 1 : 0);
}
#endif

#ifdef _MSC_VER
bool CSystem::ShellCopy(const char* from, const char* dest)
{
    if (from == nullptr || dest == nullptr)
    {
        return false;
    }
    size_t fromLen = ::strlen(from);
    size_t destLen = ::strlen(dest);
    if (fromLen + 2 > MAX_PATH || destLen + 2 > MAX_PATH)
    {
        return false;
    }

    SHFILEOPSTRUCTA fileOp = { 0 };
    fileOp.wFunc = FO_COPY;
    char newFrom[MAX_PATH] = {};
    ::memcpy(newFrom, from, fromLen);
    newFrom[fromLen + 1] = 0;
    fileOp.pFrom = newFrom;
    char newTo[MAX_PATH] = {};
    ::memcpy(newTo, dest, destLen);
    newTo[destLen + 1] = 0;
    fileOp.pTo = newTo;
    fileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
    return SHFileOperationA(&fileOp) == 0;
}
#endif

#ifdef _MSC_VER
#if !defined(__APPLE__) || !TARGET_OS_IPHONE
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
#endif // CSYSTEM_PLATFORM_CAPABILITY
#endif

#ifdef _MSC_VER
#if !defined(__APPLE__) || !TARGET_OS_IPHONE
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
#endif // CSYSTEM_PLATFORM_CAPABILITY
#endif

#ifdef _MSC_VER
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

#ifdef _MSC_VER
std::vector<std::string> CSystem::processMountDll(uint32_t pid)
{
    std::vector<std::string> result;
    HANDLE hShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (hShot == INVALID_HANDLE_VALUE)
    {
        return result;
    }

    MODULEENTRY32 te = { sizeof(te) };
    BOOL bRet = ::Module32First(hShot, &te);
    while (bRet)
    {
        result.push_back(te.szModule);
        bRet = ::Module32Next(hShot, &te);
    }
    ::CloseHandle(hShot);
    return result;
}
#endif

#ifdef _MSC_VER
std::vector<std::string> CSystem::allCameraName()
{
    std::vector<std::string> result;
    HRESULT hr = ::CoInitialize(nullptr);
    if (SUCCEEDED(hr)) {
        IEnumMoniker* pEnum = nullptr;
        hr = enumerateDevices(CLSID_VideoInputDeviceCategory, &pEnum);
        if (SUCCEEDED(hr)) {
            displayDeviceInformation(pEnum, result);
            pEnum->Release();
        }
        //麦克风...........
        //        hr = enumerateDevices(CLSID_AudioInputDeviceCategory, &pEnum);
        //        if (SUCCEEDED(hr)){
        //            displayDeviceInformation(pEnum,list);
        //            pEnum->Release();
        //        }
        ::CoUninitialize();
    }
    return result;
}
#endif

#ifdef _MSC_VER
std::map<std::string, std::vector<std::string>> CSystem::allDeviceInfo()
{
    std::map<std::string, std::vector<std::string>> allDeviceInfo;
    DWORD dwFlag = (DIGCF_ALLCLASSES | DIGCF_PRESENT);
    HDEVINFO hDevInfo = ::SetupDiGetClassDevs(NULL, NULL, NULL, dwFlag);
    if (INVALID_HANDLE_VALUE == hDevInfo)
    {
        return allDeviceInfo;
    }

    SP_DEVINFO_DATA sDevInfoData;
    sDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    //Device Identification Strings
    TCHAR szDIS[MAX_PATH] = {};
    DWORD nSize = 0;

    for (int i = 0; ::SetupDiEnumDeviceInfo(hDevInfo, i, &sDevInfoData); i++)
    {
        nSize = 0;
        if (!::SetupDiGetDeviceInstanceId(hDevInfo, &sDevInfoData, szDIS, sizeof(szDIS), &nSize))
        {
            break;
        }

        std::vector<std::string> vecDeviceInfo;
        split(vecDeviceInfo, szDIS, "\\");
        if (vecDeviceInfo.empty())
        {
            continue;
        }
        allDeviceInfo[vecDeviceInfo[0]].push_back(szDIS);
    }
    ::SetupDiDestroyDeviceInfoList(hDevInfo);
    return allDeviceInfo;
}
#endif

#if CSYSTEM_HAS_DESKTOP_API
double CSystem::GetCPUSpeedGHz()
{
#ifdef _WIN32
    HKEY key = nullptr;
    if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
    {
        return 0;
    }
    DWORD mhz = 0, bytes = sizeof(mhz), type = 0;
    const LONG status = ::RegQueryValueExW(key, L"~MHz", nullptr, &type, reinterpret_cast<BYTE*>(&mhz), &bytes);
    ::RegCloseKey(key);
    return status == ERROR_SUCCESS && type == REG_DWORD && bytes == sizeof(mhz) ? mhz / 1000.0 : 0.0;
#elif defined(__APPLE__)
    uint64_t hz = 0;
    size_t bytes = sizeof(hz);
    return ::sysctlbyname("hw.cpufrequency", &hz, &bytes, nullptr, 0) == 0 && bytes == sizeof(hz) ? hz / 1e9 : 0.0;
#else
    std::ifstream source("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    uint64_t khz = 0;
    return source >> khz ? khz / 1e6 : 0.0;
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

void CSystem::Sleep(long long milliseconds)
{
#if (_MSC_VER >= 1800 || defined(__unix__) || defined(__APPLE__))
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
}

#ifdef _WIN32
BOOL CALLBACK CSystem::initializeTimerResolution(PINIT_ONCE once, PVOID parameter, PVOID* context)
{
    (void)once;
    (void)parameter;
    (void)context;
    try
    {
        wchar_t directory[MAX_PATH] = {};
        const UINT length = ::GetSystemDirectoryW(directory, MAX_PATH);
        if (length == 0 || length >= MAX_PATH)
        {
            return TRUE;
        }
        const std::wstring path = std::wstring(directory, length) + L"\\winmm.dll";
        HMODULE module = ::LoadLibraryW(path.c_str());
        if (module == nullptr)
        {
            return TRUE;
        }
        typedef UINT(WINAPI* PeriodFunction)(UINT);
        PeriodFunction beginPeriod = reinterpret_cast<PeriodFunction>(::GetProcAddress(module, "timeBeginPeriod"));
        PeriodFunction endPeriod = reinterpret_cast<PeriodFunction>(::GetProcAddress(module, "timeEndPeriod"));
        const bool active = beginPeriod != nullptr && endPeriod != nullptr && beginPeriod(1) == 0;
        // InitOnce串行创建，shared_ptr在模块卸载时成对归还计时精度并释放系统模块。
        /** 归还本模块持有的系统计时精度租约
        @param [in] value 由shared_ptr独占管理的系统模块句柄
        */
        static std::shared_ptr<void> s_resolution(module, [endPeriod, active](void* value)
        {
            if (active)
            {
                endPeriod(1);
            }
            ::FreeLibrary(static_cast<HMODULE>(value));
        });
        return TRUE;
    }
    catch (...)
    {
        // shared_ptr构造失败也会调用删除器，不能让异常穿过Windows回调边界。
        return FALSE;
    }
}

void CSystem::sleepWithTimerResolution(uint32_t milliseconds)
{
    if (milliseconds != 0)
    {
        static INIT_ONCE s_legacyTimerOnce = INIT_ONCE_STATIC_INIT;
        ::InitOnceExecuteOnce(&s_legacyTimerOnce, initializeTimerResolution, nullptr, nullptr);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}
#endif

void CSystem::sleepPrecise(uint32_t milliseconds)
{
#ifdef _WIN32
    typedef HANDLE(WINAPI* CreateTimer)(LPSECURITY_ATTRIBUTES, LPCWSTR, DWORD, DWORD);
    CreateTimer createTimer = reinterpret_cast<CreateTimer>(
        ::GetProcAddress(::GetModuleHandleW(L"kernel32.dll"), "CreateWaitableTimerExW"));
    if (milliseconds > 0 && createTimer != nullptr)
    {
        HANDLE timer = createTimer(nullptr, nullptr, 0x00000002, TIMER_MODIFY_STATE | SYNCHRONIZE);
        if (timer != nullptr)
        {
            LARGE_INTEGER due = {};
            due.QuadPart = -static_cast<LONGLONG>(milliseconds) * 10000;
            const bool elapsed = ::SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE) != FALSE &&
                ::WaitForSingleObject(timer, INFINITE) == WAIT_OBJECT_0;
            ::CloseHandle(timer);
            if (elapsed)
            {
                return;
            }
        }
    }
    sleepWithTimerResolution(milliseconds);
#else
    Sleep(milliseconds);
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
#elif defined(__ANDROID__)
    uint8_t value[16] = {};
    if (!secureRandomBytes(value, sizeof(value)))
    {
        return "";
    }
    value[6] = static_cast<uint8_t>((value[6] & 0x0f) | 0x40);
    value[8] = static_cast<uint8_t>((value[8] & 0x3f) | 0x80);
    const char kHex[] = "0123456789abcdef";
    std::string result;
    result.reserve(flag == 0 ? 32 : 36);
    for (size_t index = 0; index < sizeof(value); ++index)
    {
        if (flag != 0 && (index == 4 || index == 6 || index == 8 || index == 10))
        {
            result.push_back('-');
        }
        result.push_back(kHex[value[index] >> 4]);
        result.push_back(kHex[value[index] & 0x0f]);
    }
    return result;
#elif defined(__APPLE__)
    uuid_t value;
    uuid_generate_random(value);
    char buffer[37] = {};
    uuid_unparse_lower(value, buffer);
    std::string result(buffer);
    if (flag == 0)
    {
        result.erase(std::remove(result.begin(), result.end(), '-'), result.end());
    }
    return result;
#elif defined(__unix__)
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
    if (srcFile.empty() || dstFile.empty() || srcFile.find('\0') != std::string::npos ||
        dstFile.find('\0') != std::string::npos) { return; }
#ifdef _WIN32
    ::CopyFileA(srcFile.c_str(), dstFile.c_str(), over == false);
#elif defined(__unix__) || defined(__APPLE__)
    std::unique_ptr<FILE, int(*)(FILE*)> source(std::fopen(srcFile.c_str(), "rb"), std::fclose);
    struct stat from = {}, to = {};
    if (!source || ::fstat(::fileno(source.get()), &from) != 0 || S_ISDIR(from.st_mode)) { return; }
    const int fd = ::open(dstFile.c_str(), O_WRONLY | O_CREAT | O_CLOEXEC | (over ? 0 : O_EXCL), 0666);
    if (fd < 0) { return; }
    std::unique_ptr<FILE, int(*)(FILE*)> destination(::fdopen(fd, "wb"), std::fclose);
    if (!destination) { ::close(fd); return; }
    if (::fstat(fd, &to) != 0 ||
        (from.st_dev == to.st_dev && from.st_ino == to.st_ino) ||
        (S_ISREG(to.st_mode) && ::ftruncate(fd, 0) != 0))
    {
        return;
    }
    char block[16384];
    size_t count;
    while ((count = std::fread(block, 1, sizeof(block), source.get())) != 0)
    {
        if (std::fwrite(block, 1, count, destination.get()) != count) { return; }
    }
#endif
}

int CSystem::GetSystemBits()
{
#ifdef _WIN32
    SYSTEM_INFO si;
    getNativeSystemInfo(&si);
    const WORD arm64Architecture = 12; // 兼容未定义 PROCESSOR_ARCHITECTURE_ARM64 的旧 Windows SDK。
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
        si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 ||
        si.wProcessorArchitecture == arm64Architecture)
    {
        return 64;
    }
    return 32;
#elif defined(__unix__) || defined(__APPLE__)
    return sizeof(void*) * 8;
#endif
}

void CSystem::OutputMap(const std::map<std::string, std::string>& stringMap, const std::string& path)
{
    if (path.find('\0') != std::string::npos) { return; }
    std::ofstream file;
    if (!path.empty()) { file.open(path.c_str(), std::ios_base::binary | std::ios_base::app); }
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
    if (path.find('\0') != std::string::npos) { return; }
    std::ofstream file;
    if (!path.empty()) { file.open(path.c_str(), std::ios_base::binary | std::ios_base::app); }
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

#if CSYSTEM_HAS_DESKTOP_API
void CSystem::ClearScanf()
{
    int ch;
    do { ch = std::getchar(); } while (ch != '\n' && ch != EOF);
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

std::vector<std::string> CSystem::exeParam(int argc, char** argv)
{
    std::vector<std::string> result;
#ifdef _WIN32
    for (int32_t index = 0; index < __argc; ++index)
    {
        result.push_back(__argv[index]);
    }
#elif defined(__unix__) || defined(__APPLE__)
for (int32_t index = 0; index < argc; ++index)
    {
        result.push_back(argv[index]);
    }
#endif
    return result;
}

#if CSYSTEM_HAS_DESKTOP_API
std::string CSystem::PasswordScanf()
{
#ifdef _WIN32
    char returnCode = '\r';
    char backspace = 8;
#elif defined(__unix__) || defined(__APPLE__)
    char returnCode = '\n';
    char backspace = 127;
#endif
    std::string password;
    int ch;
    while ((ch = CSystem::getConsoleCharacter()) != EOF && ch != returnCode)
    {
        //不是Backspace就录入
        if (ch != backspace)
        {
            password.push_back(static_cast<char>(ch));
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
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if !defined(__APPLE__) || !TARGET_OS_IPHONE
int32_t CSystem::SystemCommand(const std::string& command, std::string& result, bool isShowCmd)
{
    if (command.empty() || command.find('\0') != std::string::npos)
    {
        return -1;
    }
    // 命令可能引用 result，清空输出前先保留输入。
    const std::string text(command);
    result.clear();
#ifdef _WIN32
    if (!isShowCmd)
    {
        std::vector<char> arguments(text.begin(), text.end());
        arguments.push_back('\0');
        SECURITY_ATTRIBUTES security = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
        HANDLE readHandle = nullptr, writeHandle = nullptr;
        if (!::CreatePipe(&readHandle, &writeHandle, &security, 0)) { return -1; }
        typedef std::unique_ptr<void, decltype(&::CloseHandle)> Handle;
        Handle reader(readHandle, ::CloseHandle), writer(writeHandle, ::CloseHandle);
        if (!::SetHandleInformation(readHandle, HANDLE_FLAG_INHERIT, 0)) { return -1; }
        // 只允许本次子进程继承自己的标准流，其他并发调用的管道不能延迟本次 EOF。
        HANDLE input = nullptr;
        const HANDLE standard = ::GetStdHandle(STD_INPUT_HANDLE);
        if (standard == nullptr || standard == INVALID_HANDLE_VALUE ||
            !::DuplicateHandle(::GetCurrentProcess(), standard, ::GetCurrentProcess(), &input, 0, TRUE, DUPLICATE_SAME_ACCESS))
        {
            input = ::CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security, OPEN_EXISTING, 0, nullptr);
            if (input == INVALID_HANDLE_VALUE) { return -1; }
        }
        Handle childInput(input, ::CloseHandle);
        SIZE_T attributeBytes = 0;
        ::InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeBytes);
        if (attributeBytes == 0) { return -1; }
        std::vector<unsigned char> storage(attributeBytes);
        STARTUPINFOEXA startup = {};
        startup.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
        if (!::InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &attributeBytes)) { return -1; }
        HANDLE streams[] = {input, writeHandle};
        startup.StartupInfo.cb = sizeof(startup);
        startup.StartupInfo.hStdInput = input;
        startup.StartupInfo.hStdOutput = startup.StartupInfo.hStdError = writeHandle;
        startup.StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        startup.StartupInfo.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION child = {};
        const bool started = ::UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
            streams, sizeof(streams), nullptr, nullptr) && ::CreateProcessA(nullptr, arguments.data(), nullptr, nullptr,
            TRUE, EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr, &startup.StartupInfo, &child);
        ::DeleteProcThreadAttributeList(startup.lpAttributeList);
        if (!started) { return -1; }
        Handle process(child.hProcess, ::CloseHandle), thread(child.hThread, ::CloseHandle);
        writer.reset();
        char buffer[4096];
        DWORD received = 0;
        for (;;)
        {
            if (!::ReadFile(reader.get(), buffer, sizeof(buffer), &received, nullptr))
            {
                if (::GetLastError() != ERROR_BROKEN_PIPE) { return -1; }
                break;
            }
            if (received == 0) { break; }
            result.append(buffer, received);
        }
        DWORD code = 0;
        if (::WaitForSingleObject(process.get(), INFINITE) != WAIT_OBJECT_0 || !::GetExitCodeProcess(process.get(), &code)) { return -1; }
        return static_cast<int32_t>(code);
    }
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(_popen(text.c_str(), "rb"), _pclose);
#else
    (void)isShowCmd;
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(::popen(text.c_str(), "r"), ::pclose);
#endif
    if (!pipe) { return -1; }
    char bytes[4096];
    size_t count;
    while ((count = std::fread(bytes, 1, sizeof(bytes), pipe.get())) != 0) { result.append(bytes, count); }
    if (std::ferror(pipe.get())) { return -1; }
    const int status = pipe.get_deleter()(pipe.release());
#ifdef _WIN32
    return status;
#else
    if (status == -1) { return -1; }
    if (WIFEXITED(status)) { return WEXITSTATUS(status); }
    return WIFSIGNALED(status) ? 128 + WTERMSIG(status) : -1;
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

uint32_t CSystem::SystemThreadId()
{
#ifdef _WIN32
    return ::GetCurrentThreadId();
    //return ((_Thrd_t*)(char*)&(std::this_thread::get_id()))->_Id;
#elif defined(__APPLE__)
    uint64_t id = 0;
    pthread_threadid_np(nullptr, &id);
    return static_cast<uint32_t>(id);
#elif defined(__linux__)
    return static_cast<uint32_t>(syscall(SYS_gettid));
#else
    return static_cast<uint32_t>(std::hash<std::thread::id>()(std::this_thread::get_id()));
#endif
}

int32_t CSystem::GetCPUCoreCount()
{
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
#elif defined(__unix__) || defined(__APPLE__)
    return static_cast<int32_t>(std::thread::hardware_concurrency());
#endif
}

#if defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__))
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
                const uint8_t* bytes = static_cast<const uint8_t*>(LockResource(hResData));
                const DWORD resourceSize = SizeofResource(hinstDLL, hResInfo);
                const size_t keyOffset = 3 * sizeof(WORD);
                const size_t valueOffset = (keyOffset + sizeof(wszVerInfo) + 3) & ~static_cast<size_t>(3);
                // 版本资源的值按DWORD对齐，使用SDK类型读取，不在实现中另定义结构体。
                if (bytes != nullptr && resourceSize >= valueOffset + sizeof(VS_FIXEDFILEINFO))
                {
                    WORD valueLength = 0;
                    WCHAR key[ARRAYSIZE(wszVerInfo)] = {};
                    std::memcpy(&valueLength, bytes + sizeof(WORD), sizeof(valueLength));
                    std::memcpy(key, bytes + keyOffset, sizeof(key));
                    key[ARRAYSIZE(key) - 1] = L'\0';
                    if (lstrcmpiW(key, wszVerInfo) == 0 && valueLength >= sizeof(VS_FIXEDFILEINFO))
                    {
                        VS_FIXEDFILEINFO value = {};
                        std::memcpy(&value, bytes + valueOffset, sizeof(value));
                        dwVersion = value.dwFileVersionMS;
                    }
                }
            }
        }
        FreeLibrary(hinstDLL);
    }
    return dwVersion;
#elif defined(__unix__) || defined(__APPLE__)
    std::string result;
    if (SystemCommand("cat /etc/issue", result) == -1 || result.empty())
    {
        return 0;
    }
    result.pop_back();

    std::vector<std::string> vec;
    split(vec, result, " ");
    if(vec.size() < 2)
    {
        return 0;
    }
    std::vector<std::string> vecOs;
    split(vecOs, vec[1], ".");
    if(vecOs.size() < 2)
    {
        return 0;
    }
    return atoi((vecOs[0] + vecOs[1]).c_str());
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

uint32_t CSystem::currentProcessPid()
{
#ifdef _WIN32
    return GetCurrentProcessId();
#elif defined(__unix__) || defined(__APPLE__)
    return getpid();
#endif
}

bool CSystem::processIdentity(uint32_t processId, uint64_t& identity)
{
    identity = 0;
    if (processId == 0)
    {
        return false;
    }
#ifdef CSYSTEM_SHARED_IDENTITY
    if (mobileSelfIdentity() == 0)
    {
        return false;
    }
    if (processId == static_cast<uint32_t>(CSystem::currentProcessPid()))
    {
        identity = mobileSelfIdentity();
        return true;
    }
    if (processId > static_cast<uint32_t>(INT32_MAX) || CSystem::processDefinitelyExited(processId))
    {
        return false;
    }
    try
    {
        return readProcessIdentity(processId, identity);
    }
    catch (...)
    {
        identity = 0;
        return false;
    }
#elif defined(_WIN32)
    HANDLE process = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (process == nullptr)
    {
        process = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    }
    if (process == nullptr)
    {
        return false;
    }
    FILETIME created = {}, exited = {}, kernel = {}, user = {};
    const bool success = ::GetProcessTimes(process, &created, &exited, &kernel, &user) != FALSE;
    ::CloseHandle(process);
    if (success)
    {
        identity = (static_cast<uint64_t>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
    }
    return success && identity != 0;
#elif defined(__linux__) && !defined(__EMSCRIPTEN__)
    try
    {
        std::ifstream input((std::string("/proc/") + std::to_string(processId) + "/stat").c_str());
        const std::string line((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        if (input.bad() || line.empty())
        {
            return false;
        }
        // comm允许空格及括号，最后一个右括号之后才是固定字段3。
        size_t cursor = line.rfind(')');
        if (cursor == std::string::npos)
        {
            return false;
        }
        ++cursor;
        for (int32_t field = 3; field <= 22; ++field)
        {
            cursor = line.find_first_not_of(' ', cursor);
            if (cursor == std::string::npos)
            {
                return false;
            }
            const size_t end = line.find(' ', cursor);
            if (field == 22)
            {
                // 内核记录中starttime后仍有字段；截短数字或读取失败不能成为新进程身份。
                if (end == std::string::npos || line[cursor] < '0' || line[cursor] > '9')
                {
                    return false;
                }
                char* tail = nullptr;
                errno = 0;
                const uint64_t parsed = std::strtoull(line.c_str() + cursor, &tail, 10);
                if (errno != 0 || parsed == 0 || tail == line.c_str() + cursor || *tail != ' ')
                {
                    return false;
                }
                identity = parsed;
                return true;
            }
            if (end == std::string::npos)
            {
                return false;
            }
            cursor = end + 1;
        }
    }
    catch (...)
    {
        identity = 0;
    }
    return false;
#elif defined(__APPLE__) && !TARGET_OS_IPHONE
    struct proc_bsdinfo info = {};
    if (::proc_pidinfo(static_cast<int32_t>(processId), PROC_PIDTBSDINFO, 0, &info, sizeof(info)) != sizeof(info))
    {
        return false;
    }
    identity = static_cast<uint64_t>(info.pbi_start_tvsec) * 1000000 + info.pbi_start_tvusec;
    return identity != 0;
#else
    return false;
#endif
}

bool CSystem::processDefinitelyExited(uint32_t processId)
{
    if (processId == 0 || processId > static_cast<uint32_t>(INT32_MAX))
    {
        return false;
    }
#ifdef _WIN32
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, processId);
    if (process == nullptr)
    {
        return GetLastError() == ERROR_INVALID_PARAMETER;
    }
    const DWORD state = WaitForSingleObject(process, 0);
    CloseHandle(process);
    return state == WAIT_OBJECT_0;
#elif defined(__unix__) || defined(__APPLE__)
#if defined(__linux__)
    try
    {
        std::ifstream input((std::string("/proc/") + std::to_string(processId) + "/stat").c_str());
        const std::string line((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        if (!input.bad() && linuxProcessStatExited(line))
        {
            return true;
        }
    }
    catch (...) { /* 查询失败不据此判定进程死亡。 */ }
#elif defined(__APPLE__) && !TARGET_OS_IPHONE
    // Darwin对尚未被父进程回收的僵尸执行kill(pid, 0)仍返回成功。
    // 仅完整且明确的SZOMB记录可判定退出；权限失败、截短记录和暂停状态不回收。
    int query[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, static_cast<int>(processId) };
    struct kinfo_proc info = {};
    size_t bytes = sizeof(info);
    if (::sysctl(query, 4, &info, &bytes, nullptr, 0) == 0 && bytes == sizeof(info) && info.kp_proc.p_stat == SZOMB)
    {
        return true;
    }
#endif
    if (::kill(static_cast<pid_t>(processId), 0) == 0)
    {
        return false;
    }
    return errno == ESRCH;
#else
    return false;
#endif
}

#if !defined(__APPLE__) || !TARGET_OS_IPHONE
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
#else
    const std::vector<uint32_t> matches = processPid(processName);
    return matches.empty() ? 0 : matches.front();
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if !defined(__APPLE__) || !TARGET_OS_IPHONE
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
#elif defined(__APPLE__)
    std::vector<uint32_t> matches;
#if !TARGET_OS_IPHONE
    if (processName.empty() || processName.find('/') != std::string::npos || processName.find('\0') != std::string::npos)
    {
        return matches;
    }
    const int count = proc_listallpids(nullptr, 0);
    if (count <= 0 || count > 65408)
    {
        return matches;
    }
    std::vector<int> pids(static_cast<size_t>(count) + 128);
    const int copied = proc_listallpids(pids.data(), static_cast<int>(pids.size() * sizeof(int)));
    if (copied <= 0)
    {
        return matches;
    }
    for (size_t index = 0; index < pids.size() && index < static_cast<size_t>(copied); ++index)
    {
        if (pids[index] <= 0)
        {
            continue;
        }
        char path[PROC_PIDPATHINFO_MAXSIZE] = {};
        if (proc_pidpath(pids[index], path, sizeof(path)) <= 0)
        {
            continue;
        }
        const char* separator = std::strrchr(path, '/');
        if (processName == (separator == nullptr ? path : separator + 1))
        {
            matches.push_back(static_cast<uint32_t>(pids[index]));
        }
    }
#else
    (void)processName; // iOS不支持独立网关进程，本入口不枚举其他应用。
#endif
    return matches;
#elif defined(__linux__)
    std::vector<uint32_t> matches;
    if (processName.empty() || processName.find('/') != std::string::npos || processName.find('\0') != std::string::npos)
    {
        return matches;
    }
    std::unique_ptr<DIR, int(*)(DIR*)> processes(::opendir("/proc"), ::closedir);
    if (!processes) { return matches; }
    while (dirent* entry = ::readdir(processes.get()))
    {
        if (entry->d_name[0] < '1' || entry->d_name[0] > '9') { continue; }
        char* end = nullptr;
        const unsigned long pid = std::strtoul(entry->d_name, &end, 10);
        if (*end == '\0' && pid <= INT32_MAX && CSystem::processName(static_cast<uint32_t>(pid)) == processName)
        {
            matches.push_back(static_cast<uint32_t>(pid));
        }
    }
    std::sort(matches.begin(), matches.end(), std::greater<uint32_t>());
    return matches;
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if !defined(__APPLE__) || !TARGET_OS_IPHONE
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
#elif defined(__APPLE__) && !TARGET_OS_IPHONE
    if (pid == 0 || pid > INT32_MAX)
    {
        return "";
    }
    char name[PROC_PIDPATHINFO_MAXSIZE] = {};
    const int count = ::proc_pidpath(static_cast<int>(pid), name, sizeof(name));
    if (count > 0 && static_cast<size_t>(count) < sizeof(name))
    {
        return CSystem::GetName(std::string(name, count), 1);
    }
    const int shortCount = ::proc_name(static_cast<int>(pid), name, sizeof(name));
    return shortCount > 0 && static_cast<size_t>(shortCount) < sizeof(name) ?
        std::string(name, shortCount) : std::string();
#elif defined(__linux__)
    if (pid == 0 || pid > INT32_MAX) { return ""; }
    // argv[0] 单独以 NUL 结尾。后继参数可含斜杠，不能当作可执行文件名。
    std::ifstream command(("/proc/" + std::to_string(pid) + "/cmdline").c_str(), std::ios::binary);
    std::string first;
    if (!std::getline(command, first, '\0')) { return ""; }
    const size_t separator = first.rfind('/');
    return separator == std::string::npos ? first : first.substr(separator + 1);
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32) || defined(__linux__)
uint64_t CSystem::currentMemory()
{
#ifdef _MSC_VER
    PROCESS_MEMORY_COUNTERS pmc = {};
    return GetProcessMemoryInfo(::GetCurrentProcess(), &pmc, sizeof(pmc)) ? pmc.WorkingSetSize : 0;
#elif defined(__unix__) || defined(__APPLE__)
    std::string stat_info = CSystem::readFile("/proc/self/status");
    const std::string key = "VmRSS:";
    size_t first = stat_info.find(key);
    if (first == std::string::npos)
    {
        return 0;
    }
    size_t valueStart = first + key.size();
    size_t valueEnd = stat_info.find_first_of('\n', valueStart);
    std::string memory = stat_info.substr(valueStart, valueEnd == std::string::npos ? std::string::npos : valueEnd - valueStart);
    return strtoull(memory.c_str(), nullptr, 10) * 1024;
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY
#endif // CSYSTEM_NATIVE_API

uint32_t CSystem::GetTickCount()
{
#ifdef _MSC_VER
    return ::GetTickCount();
#elif defined(__unix__) || defined(__APPLE__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint32_t>(static_cast<uint64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000);
#endif
}

std::chrono::high_resolution_clock::time_point CSystem::GetHighTickCount()
{
    return std::chrono::high_resolution_clock::now();
}

int32_t CSystem::GetHighTickCountMilliRunTime(const std::chrono::high_resolution_clock::time_point& beginTime)
{
    return (int32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - beginTime).count();
}

int32_t CSystem::GetHighTickCountMicroRunTime(const std::chrono::high_resolution_clock::time_point& beginTime)
{
    return (int32_t)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - beginTime).count();
}

int64_t CSystem::GetHighTickCountNanoRunTime(const std::chrono::high_resolution_clock::time_point& beginTime)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - beginTime).count();
}

int32_t CSystem::GetMilliRunTime(const std::chrono::high_resolution_clock::time_point& endTime, const std::chrono::high_resolution_clock::time_point& beginTime)
{
    return (int32_t)std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime).count();
}

int32_t CSystem::GetMicroRunTime(const std::chrono::high_resolution_clock::time_point& endTime, const std::chrono::high_resolution_clock::time_point& beginTime)
{
    return (int32_t)std::chrono::duration_cast<std::chrono::microseconds>(endTime - beginTime).count();
}

int64_t CSystem::GetNanoRunTime(const std::chrono::high_resolution_clock::time_point& endTime, const std::chrono::high_resolution_clock::time_point& beginTime)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - beginTime).count();
}

#if CSYSTEM_HAS_NATIVE_API
std::string CSystem::getComputerName()
{
    char computerName[256] = {};
#ifdef _WIN32
    DWORD length = 256;
    GetComputerNameA(computerName, &length);
#elif defined(__unix__) || defined(__APPLE__)
    gethostname(computerName, 256);
#endif
    return computerName;
}

std::string CSystem::GetCurrentDllPath()
{
    char szFilePath[1024] = {};
#ifdef _MSC_VER
    ::GetModuleFileNameA(getCurrentModule(), szFilePath, 1024);
#elif defined(__unix__) || defined(__APPLE__)
    getModuleCurrentPath(szFilePath);
#endif
    return CSystem::GetName(szFilePath, 4);
}

std::string CSystem::currentExecutablePathUtf8()
{
#ifdef _WIN32
    std::vector<wchar_t> path(32768, 0);
    const DWORD size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    return size == 0 || size >= path.size() ? std::string() : widePathToUtf8(std::wstring(path.data(), size));
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    if (size == 0 || size > 65536)
    {
        return std::string();
    }
    std::vector<char> path(size, 0);
    if (_NSGetExecutablePath(path.data(), &size) != 0)
    {
        return std::string();
    }
    return canonicalPathUtf8(path.data());
#elif defined(__unix__)
    std::vector<char> path(65536, 0);
    const ssize_t size = readlink("/proc/self/exe", path.data(), path.size() - 1);
    return size <= 0 || static_cast<size_t>(size) >= path.size() - 1 ? std::string() : std::string(path.data(), static_cast<size_t>(size));
#else
    return std::string();
#endif
}

std::string CSystem::GetCurrentExePath()
{
    char szFilePath[1024] = {};
#ifdef _WIN32
    ::GetModuleFileNameA(NULL, szFilePath, 1024);
#elif defined(__APPLE__)
    uint32_t size = sizeof(szFilePath);
    if (_NSGetExecutablePath(szFilePath, &size) != 0)
    {
        return "";
    }
#elif defined(__unix__) || defined(__APPLE__)
    ssize_t len = ::readlink("/proc/self/exe", szFilePath, sizeof(szFilePath) - 1);
    if (len < 0)
    {
        return "";
    }
    szFilePath[len] = '\0';
#endif
    return CSystem::GetName(szFilePath, 4);
}

std::string CSystem::GetCurrentExeName()
{
    // 默认进程名只从完整路径提取；截短路径可能指向另一个文件名。
    char szFilePath[1024] = {};
#ifdef _WIN32
    const uint32_t length = ::GetModuleFileNameA(NULL, szFilePath, sizeof(szFilePath));
    if (length == 0 || length >= sizeof(szFilePath))
    {
        return "";
    }
#elif defined(__APPLE__)
    uint32_t size = sizeof(szFilePath);
    if (_NSGetExecutablePath(szFilePath, &size) != 0)
    {
        return "";
    }
#elif defined(__unix__) || defined(__APPLE__)
    ssize_t len = ::readlink("/proc/self/exe", szFilePath, sizeof(szFilePath) - 1);
    if (len < 0 || static_cast<size_t>(len) >= sizeof(szFilePath) - 1)
    {
        return "";
    }
    szFilePath[len] = '\0';
#endif
    return CSystem::GetName(szFilePath, 3);
}

std::string CSystem::GetCurrentExeFullName()
{
    // readlink返回缓冲区上限时无法确认完整性，不能登记被截短的名字。
    char szFilePath[1024] = {};
#ifdef _WIN32
    const uint32_t length = ::GetModuleFileNameA(NULL, szFilePath, sizeof(szFilePath));
    if (length == 0 || length >= sizeof(szFilePath))
    {
        return "";
    }
#elif defined(__APPLE__)
    uint32_t size = sizeof(szFilePath);
    if (_NSGetExecutablePath(szFilePath, &size) != 0)
    {
        return "";
    }
#elif defined(__unix__) || defined(__APPLE__)
    ssize_t len = ::readlink("/proc/self/exe", szFilePath, sizeof(szFilePath) - 1);
    if (len < 0 || static_cast<size_t>(len) >= sizeof(szFilePath) - 1)
    {
        return "";
    }
    szFilePath[len] = '\0';
#endif
    return CSystem::GetName(szFilePath, 1);
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
#elif defined(__unix__) || defined(__APPLE__)
    const char* directory = std::getenv("TMPDIR");
    return directory && *directory ? joinPath(directory, "") + (directory[std::strlen(directory) - 1] == '/' ? "" : "/") : "/tmp/";
#endif
}
#endif // CSYSTEM_NATIVE_API

std::string CSystem::joinPath(const std::string& directory, const std::string& name)
{
    if (directory.empty())
    {
        return name;
    }
    if (name.empty())
    {
        return directory;
    }
    const char last = directory[directory.size() - 1];
    const bool separator = last != '/' && last != '\\';
    std::string result;
    result.reserve(directory.size() + name.size() + (separator ? 1 : 0));
    result.append(directory);
    if (separator)
    {
        result.push_back('/');
    }
    result.append(name);
    return result;
}

#if CSYSTEM_HAS_NATIVE_API
std::string CSystem::canonicalPath(const std::string& path, bool requireExisting)
{
    if (path.empty() || path.find('\0') != std::string::npos)
    {
        return "";
    }
#ifdef _WIN32
    std::string result;
    if (requireExisting)
    {
        HANDLE handle = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (handle == INVALID_HANDLE_VALUE)
        {
            return "";
        }
        std::unique_ptr<void, decltype(&::CloseHandle)> handleGuard(handle, &::CloseHandle);
        const DWORD required = GetFinalPathNameByHandleA(handle, nullptr, 0, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
        if (required > 0)
        {
            std::vector<char> buffer(static_cast<size_t>(required) + 1, '\0');
            const DWORD written = GetFinalPathNameByHandleA(handle, &buffer[0], static_cast<DWORD>(buffer.size()), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
            if (written > 0 && written < buffer.size())
            {
                result.assign(&buffer[0], written);
            }
        }
        if (result.compare(0, 8, "\\\\?\\UNC\\") == 0)
        {
            result = "\\\\" + result.substr(8);
        }
        else if (result.compare(0, 4, "\\\\?\\") == 0)
        {
            result.erase(0, 4);
        }
    }
    else
    {
        const DWORD required = GetFullPathNameA(path.c_str(), 0, nullptr, nullptr);
        if (required == 0)
        {
            return "";
        }
        std::vector<char> buffer(static_cast<size_t>(required) + 1, '\0');
        const DWORD written = GetFullPathNameA(path.c_str(), static_cast<DWORD>(buffer.size()), &buffer[0], nullptr);
        if (written == 0 || written >= buffer.size())
        {
            return "";
        }
        result.assign(&buffer[0], written);
    }
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] = result[i] == '/' ? '\\' : static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
    }
    while (result.size() > 3 && result.back() == '\\')
    {
        result.pop_back();
    }
    return result;
#elif defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
    char buffer[PATH_MAX] = {};
    if (requireExisting)
    {
        return realpath(path.c_str(), buffer) == nullptr ? "" : std::string(buffer);
    }
    if (path[0] != '/' && getcwd(buffer, sizeof(buffer)) == nullptr)
    {
        return "";
    }
    const std::string absolute = path[0] == '/' ? path : std::string(buffer) + "/" + path;
    std::vector<std::string> parts;
    size_t begin = 0;
    while (begin < absolute.size())
    {
        const size_t slash = absolute.find('/', begin);
        const std::string part = absolute.substr(begin, slash == std::string::npos ? std::string::npos : slash - begin);
        if (part == "..")
        {
            if (!parts.empty())
            {
                parts.pop_back();
            }
        }
        else if (!part.empty() && part != ".")
        {
            parts.push_back(part);
        }
        if (slash == std::string::npos)
        {
            break;
        }
        begin = slash + 1;
    }
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        result += "/" + parts[i];
    }
    return result.empty() ? "/" : result;
#else
    return "";
#endif
}

bool CSystem::isPathWithin(const std::string& path, const std::string& directory, bool requireExisting)
{
    const std::string normalized = canonicalPath(path, requireExisting);
    std::string root = canonicalPath(directory, true);
    if (root.empty())
    {
        root = canonicalPath(directory, false);
    }
    if (normalized.empty() || root.empty())
    {
        return false;
    }
#ifdef _WIN32
    const char separator = '\\';
#else
    const char separator = '/';
#endif
    if (root.back() != separator)
    {
        root += separator;
    }
    return normalized.size() > root.size() && normalized.compare(0, root.size(), root) == 0;
}

bool CSystem::migrateDirectory(const std::string& source, const std::string& destination, std::string& errorText)
{
    errorText.clear();
    const std::string from = canonicalPath(source, false), to = canonicalPath(destination, false);
    if (from.empty() || to.empty() || from == to || isPathWithin(from, to, false) || isPathWithin(to, from, false))
    {
        errorText = "Invalid or overlapping migration roots";
        return false;
    }
    if (!DirOrFileExist(source))
    {
        return true;
    }
    if (!plainDirectory(source) ||
        (DirOrFileExist(destination) && !plainDirectory(destination)))
    {
        errorText = "Migration root is not a plain directory";
        return false;
    }
    return migrateTree(source, destination, errorText);
}

bool CSystem::createDirectories(const std::string& path)
{
    const std::string target = canonicalPath(path, false);
    if (target.empty())
    {
        return false;
    }
    if (DirOrFileExist(target))
    {
        return plainDirectory(target);
    }
#ifdef _WIN32
    const size_t slash = target.rfind('\\');
#else
    const size_t slash = target.rfind('/');
#endif
    if (slash != std::string::npos && slash != 0 &&
        (slash + 1 == target.size() || !createDirectories(target.substr(0, slash + 1))))
    {
        return false;
    }
    // 另一创建者可能已完成 mkdir，只在最后检查到普通目录时接受 EEXIST。
    return (CreateDir(target) || errno == EEXIST) && plainDirectory(target);
}
#endif // CSYSTEM_NATIVE_API

std::string CSystem::ensureFileExtension(const std::string& path, const std::string& extension)
{
    if (path.empty() || extension.empty())
    {
        return path;
    }
    bool matches = path.size() >= extension.size();
    const size_t offset = matches ? path.size() - extension.size() : 0;
    for (size_t index = 0; matches && index < extension.size(); ++index)
    {
        const char left = path[offset + index];
        const char right = extension[index];
        const char foldedLeft = left >= 'A' && left <= 'Z' ? static_cast<char>(left + ('a' - 'A')) : left;
        const char foldedRight = right >= 'A' && right <= 'Z' ? static_cast<char>(right + ('a' - 'A')) : right;
        matches = foldedLeft == foldedRight;
    }
    return (matches ? path.substr(0, offset) : path) + extension;
}

std::string CSystem::GetName(const std::string& path, int32_t flag)
{
    if (flag < 1 || flag > 4)
    {
        return std::string();
    }
    size_t start = path.size();
    size_t dot = std::string::npos;
    while (start > 0)
    {
        const char ch = path[start - 1];
        if (ch == '/' || ch == '\\')
        {
            break;
        }
        --start;
        if (ch == '.' && dot == std::string::npos)
        {
            dot = start;
        }
    }
    if (flag == 1)
    {
        return std::string(path.data() + start, path.size() - start);
    }
    if (flag == 4)
    {
        return std::string(path.data(), start);
    }
    if (flag == 2)
    {
        return dot == std::string::npos ? std::string() : std::string(path.data() + dot + 1, path.size() - dot - 1);
    }
    return std::string(path.data() + start, (dot == std::string::npos ? path.size() : dot) - start);
}

#if CSYSTEM_HAS_NATIVE_API
bool CSystem::deleteFile(const char* path)
{
    if (path == nullptr || path[0] == '\0')
    {
        return false;
    }
    return ::remove(path) == 0;
}

FILE* CSystem::openBinaryOutputFile(const std::wstring& path, std::string* errorText)
{
    setSystemError(errorText, "");
    if (path.empty() || path.find(L'\0') != std::wstring::npos)
    {
        setSystemError(errorText, "invalid file path");
        return nullptr;
    }

#ifdef _WIN32
    FILE* file = nullptr;
    if (_wfopen_s(&file, path.c_str(), L"wb") != 0 || file == nullptr)
    {
        setSystemError(errorText, "open binary output file failed");
        return nullptr;
    }
    return file;
#elif defined(__unix__) || defined(__APPLE__)
    std::string utf8Path = widePathToUtf8(path);
    if (utf8Path.empty())
    {
        setSystemError(errorText, "convert file path to utf8 failed");
        return nullptr;
    }

    FILE* file = fopen(utf8Path.c_str(), "wb");
    if (file == nullptr)
    {
        setSystemError(errorText, "open binary output file failed");
        return nullptr;
    }
    return file;
#else
    setSystemError(errorText, "unsupported platform");
    return nullptr;
#endif
}

bool CSystem::readBinaryFile(const std::wstring& path, size_t maximumBytes, std::vector<uint8_t>& data)
{
    if (path.empty() || path.find(L'\0') != std::wstring::npos)
    {
        return false;
    }
#ifdef _WIN32
    FILE* opened = _wfopen(path.c_str(), L"rb");
#elif defined(__unix__) || defined(__APPLE__)
    const std::string utf8Path = widePathToUtf8(path);
    FILE* opened = utf8Path.empty() ? nullptr : fopen(utf8Path.c_str(), "rb");
#else
    FILE* opened = nullptr;
#endif
    std::unique_ptr<FILE, int (*)(FILE*)> file(opened, &fclose);
    if (!file)
    {
        return false;
    }
    std::vector<uint8_t> result;
    uint8_t buffer[16384];
    for (;;)
    {
        const size_t read = fread(buffer, 1, sizeof(buffer), file.get());
        if (read > maximumBytes - result.size())
        {
            return false;
        }
        result.insert(result.end(), buffer, buffer + read);
        if (read < sizeof(buffer))
        {
            if (ferror(file.get()) || !feof(file.get()))
            {
                return false;
            }
            break;
        }
    }
    if (fclose(file.release()) != 0)
    {
        return false;
    }
    data.swap(result);
    return true;
}

FILE* CSystem::openPrivateBinaryOutputFile(const std::wstring& path, std::string* errorText)
{
    setSystemError(errorText, "");
    if (path.empty() || path.find(L'\0') != std::wstring::npos)
    {
        setSystemError(errorText, "invalid private file path");
        return nullptr;
    }
#ifdef _WIN32
    HANDLE tokenValue = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tokenValue))
    {
        setSystemError(errorText, "cannot query private file owner");
        return nullptr;
    }
    std::unique_ptr<void, decltype(&CloseHandle)> token(tokenValue, &CloseHandle);
    DWORD size = 0;
    GetTokenInformation(token.get(), TokenUser, nullptr, 0, &size);
    if (size == 0 || size > 65536)
    {
        setSystemError(errorText, "invalid private file owner");
        return nullptr;
    }
    std::vector<uint8_t> owner(size);
    if (!GetTokenInformation(token.get(), TokenUser, owner.data(), size, &size))
    {
        setSystemError(errorText, "cannot read private file owner");
        return nullptr;
    }
    LPWSTR sidValue = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(owner.data())->User.Sid, &sidValue))
    {
        setSystemError(errorText, "cannot encode private file owner");
        return nullptr;
    }
    std::unique_ptr<wchar_t, decltype(&LocalFree)> sid(sidValue, &LocalFree);
    const std::wstring permissions = L"D:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;FA;;;" + std::wstring(sid.get()) + L")";
    PSECURITY_DESCRIPTOR descriptorValue = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(permissions.c_str(), SDDL_REVISION_1, &descriptorValue, nullptr))
    {
        setSystemError(errorText, "cannot protect private file");
        return nullptr;
    }
    std::unique_ptr<void, decltype(&LocalFree)> descriptor(descriptorValue, &LocalFree);
    SECURITY_ATTRIBUTES attributes = {};
    attributes.nLength = sizeof(attributes);
    attributes.lpSecurityDescriptor = descriptor.get();
    HANDLE opened = CreateFileW(path.c_str(), GENERIC_WRITE, 0, &attributes, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (opened == INVALID_HANDLE_VALUE)
    {
        setSystemError(errorText, "cannot exclusively create private file");
        return nullptr;
    }
    std::unique_ptr<void, decltype(&CloseHandle)> handle(opened, &CloseHandle);
    const int fileDescriptor = _open_osfhandle(reinterpret_cast<intptr_t>(handle.get()), _O_BINARY | _O_WRONLY | _O_NOINHERIT);
    if (fileDescriptor < 0)
    {
        handle.reset();
        DeleteFileW(path.c_str());
        setSystemError(errorText, "cannot attach private file handle");
        return nullptr;
    }
    handle.release();
    FILE* file = _fdopen(fileDescriptor, "wb");
    if (file == nullptr)
    {
        _close(fileDescriptor);
        DeleteFileW(path.c_str());
    }
    return file;
#elif defined(__unix__) || defined(__APPLE__)
    const std::string utf8 = widePathToUtf8(path);
    if (utf8.empty())
    {
        return nullptr;
    }
    int flags = O_WRONLY | O_CREAT | O_EXCL;
#ifdef O_NOFOLLOW
    flags |= O_NOFOLLOW;
#endif
#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif
    const int fileDescriptor = ::open(utf8.c_str(), flags, 0600);
    if (fileDescriptor < 0)
    {
        setSystemError(errorText, "cannot exclusively create private file");
        return nullptr;
    }
    FILE* file = ::fdopen(fileDescriptor, "wb");
    if (file == nullptr)
    {
        ::close(fileDescriptor);
        ::unlink(utf8.c_str());
    }
    return file;
#else
    setSystemError(errorText, "unsupported private file platform");
    return nullptr;
#endif
}

bool CSystem::flushBinaryOutputFile(FILE* file)
{
    if (file == nullptr || fflush(file) != 0)
    {
        return false;
    }
#ifdef _WIN32
    return _commit(_fileno(file)) == 0;
#elif defined(__unix__) || defined(__APPLE__)
    return ::fsync(::fileno(file)) == 0;
#else
    return false;
#endif
}

bool CSystem::installFileIfAbsent(const std::wstring& sourcePath, const std::wstring& destinationPath)
{
    if (sourcePath.empty() || destinationPath.empty() || sourcePath.find(L'\0') != std::wstring::npos ||
        destinationPath.find(L'\0') != std::wstring::npos)
    {
        return false;
    }
#ifdef _WIN32
    return MoveFileExW(sourcePath.c_str(), destinationPath.c_str(), MOVEFILE_WRITE_THROUGH) != 0;
#elif defined(__unix__) || defined(__APPLE__)
    const std::string source = widePathToUtf8(sourcePath);
    const std::string destination = widePathToUtf8(destinationPath);
    if (source.empty() || destination.empty() || ::link(source.c_str(), destination.c_str()) != 0)
    {
        return false;
    }
    // 目标安装成功后，临时别名删除失败不撤回已提交目标；宿主可清理自己的别名。
    ::unlink(source.c_str());
    return true;
#else
    return false;
#endif
}

bool CSystem::plainFileUtf8(const std::string& path)
{
    if (path.empty() || path.find('\0') != std::string::npos)
    {
        return false;
    }
#ifdef _WIN32
    if (path.size() > INT_MAX)
    {
        return false;
    }
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), static_cast<int>(path.size()), nullptr, 0);
    if (size <= 0)
    {
        return false;
    }
    std::vector<wchar_t> wide(static_cast<size_t>(size) + 1, 0);
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), static_cast<int>(path.size()), wide.data(), size) != size)
    {
        return false;
    }
    const DWORD attributes = GetFileAttributesW(wide.data());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) == 0;
#else
    return plainFile(path);
#endif
}

bool CSystem::writeBinaryOutputFile(FILE* file, const void* data, size_t size)
{
    return file != nullptr && data != nullptr && fwrite(data, 1, size, file) == size;
}

bool CSystem::fileStampUtf8(const std::string& path, uint64_t* bytes, uint64_t* stamp)
{
    if (bytes == nullptr || stamp == nullptr)
    {
        return false;
    }
    *bytes = 0;
    *stamp = 0;
    if (!plainFileUtf8(path))
    {
        return false;
    }
#ifdef _WIN32
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), static_cast<int>(path.size()), nullptr, 0);
    std::vector<wchar_t> wide(static_cast<size_t>(length) + 1, 0);
    if (length <= 0 || MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), static_cast<int>(path.size()), wide.data(), length) != length)
    {
        return false;
    }
    WIN32_FILE_ATTRIBUTE_DATA info = {};
    if (!GetFileAttributesExW(wide.data(), GetFileExInfoStandard, &info) ||
        (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0)
    {
        return false;
    }
    *bytes = (static_cast<uint64_t>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    *stamp = (static_cast<uint64_t>(info.ftLastWriteTime.dwHighDateTime) << 32) | info.ftLastWriteTime.dwLowDateTime;
    return true;
#elif defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
    struct stat info;
    if (lstat(path.c_str(), &info) != 0 || !S_ISREG(info.st_mode) || info.st_size < 0)
    {
        return false;
    }
    *bytes = static_cast<uint64_t>(info.st_size);
#ifdef __APPLE__
    *stamp = static_cast<uint64_t>(info.st_mtimespec.tv_sec) * UINT64_C(1000000000) + info.st_mtimespec.tv_nsec;
#else
    *stamp = static_cast<uint64_t>(info.st_mtim.tv_sec) * UINT64_C(1000000000) + info.st_mtim.tv_nsec;
#endif
    return true;
#else
    return false;
#endif
}

bool CSystem::closeBinaryOutputFile(FILE* file)
{
    return file != nullptr && fclose(file) == 0;
}

bool CSystem::deleteFile(const std::wstring& path)
{
    if (path.empty() || path.find(L'\0') != std::wstring::npos)
    {
        return false;
    }

#ifdef _WIN32
    return ::DeleteFileW(path.c_str()) ? true : false;
#elif defined(__unix__) || defined(__APPLE__)
    std::string utf8Path = widePathToUtf8(path);
    if (utf8Path.empty())
    {
        return false;
    }
    return ::remove(utf8Path.c_str()) == 0;
#else
    return false;
#endif
}

bool CSystem::fileExists(const std::wstring& path)
{
    if (path.empty() || path.find(L'\0') != std::wstring::npos)
    {
        return false;
    }

#ifdef _WIN32
    DWORD attributes = ::GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
#elif defined(__unix__) || defined(__APPLE__)
    std::string utf8Path = widePathToUtf8(path);
    if (utf8Path.empty())
    {
        return false;
    }

    struct stat fileStat;
    if (::stat(utf8Path.c_str(), &fileStat) != 0)
    {
        return false;
    }
    return S_ISREG(fileStat.st_mode) ? true : false;
#else
    return false;
#endif
}

#if CSYSTEM_HAS_DESKTOP_API
std::string CSystem::inputString(const std::string& tip)
{
    printf("%s\n", tip.c_str());
    std::string result;
    char ch = 0;
    while (std::cin.get(ch))
    {
        if (ch == '\n')
        {
            break;
        }
        result += ch;
    }
    return result;
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

void CSystem::killProcess(int32_t pid)
{
    if (pid <= 0)
    {
        return;
    }
#ifdef _WIN32
    char command[256] = {};
    ::_snprintf(command, 256, "taskkill /f /pid %d", pid);
    ::WinExec(command, SW_HIDE);
#elif defined(__unix__) || defined(__APPLE__)
    ::kill(static_cast<pid_t>(pid), SIGKILL);
#endif
}

bool CSystem::rename(const std::string& oldPath, const std::string& newPath)
{
    return !oldPath.empty() && !newPath.empty() && oldPath.find('\0') == std::string::npos &&
        newPath.find('\0') == std::string::npos && ::rename(oldPath.c_str(), newPath.c_str()) == 0;
}

bool CSystem::replaceFile(const std::wstring& sourcePath, const std::wstring& destinationPath)
{
    if (sourcePath.empty() || destinationPath.empty() || sourcePath.find(L'\0') != std::wstring::npos ||
        destinationPath.find(L'\0') != std::wstring::npos)
    {
        return false;
    }
#ifdef _WIN32
    return MoveFileExW(sourcePath.c_str(), destinationPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#elif defined(__unix__) || defined(__APPLE__)
    const std::string source = widePathToUtf8(sourcePath);
    const std::string destination = widePathToUtf8(destinationPath);
    return !source.empty() && !destination.empty() && ::rename(source.c_str(), destination.c_str()) == 0;
#else
    return false;
#endif
}

std::string CSystem::GetSysUserName()
{
#ifdef _WIN32
    DWORD size = 1024;
    char szName[1024] = {};
    ::GetUserNameA(szName, &size);
    return szName;
#elif defined(__unix__) || defined(__APPLE__)
    uid_t userid = 0;
    struct passwd* pwd = nullptr;
    userid = getuid();
    pwd = getpwuid(userid);
    std::string name;
    if (pwd != nullptr)
    {
        return pwd->pw_name;
    }
#if !defined(__APPLE__) || !TARGET_OS_IPHONE
    SystemCommand("whoami", name);
    if (!name.empty() && name.back() == '\n')
    {
        name.pop_back();
    }
#endif
    return name;
#endif
}

std::string CSystem::GetEnvironment(const char* name)
{
    if (name == nullptr)
    {
        return "";
    }
    char* value = ::getenv(name);
    if (value == nullptr)
    {
        return "";
    }
    return value;
}

bool CSystem::CreateDir(const std::string& dir)
{
    if (dir.empty() || dir.find('\0') != std::string::npos) { return false; }
#ifdef _WIN32
    return _mkdir(dir.c_str()) == 0;
#elif defined(__unix__) || defined(__APPLE__)
    return mkdir(dir.c_str() ,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#endif
}

bool CSystem::DestroyDir(const std::string& dir)
{
    if (dir.empty() || dir.find('\0') != std::string::npos) { return false; }
#ifdef _WIN32
    return _rmdir(dir.c_str()) == 0;
#elif defined(__unix__) || defined(__APPLE__)
    return rmdir(dir.c_str()) == 0;
#endif
}

bool CSystem::DirOrFileExist(const std::string& dir)
{
    if (dir.empty() || dir.find('\0') != std::string::npos) { return false; }
#ifdef _WIN32
    return _access(dir.c_str(), 0) == 0;
#elif defined(__unix__) || defined(__APPLE__)
    return access(dir.c_str(), 0) == 0;
#endif
}

#if CSYSTEM_HAS_DESKTOP_API && !defined(_WIN32)
void CSystem::openDesktopPath(const std::string& program, const char* option, const std::string& path)
{
    // 在 fork 前完成 PATH 查找及字符串分配，子进程仅执行异步信号安全的系统调用。
    std::string executable = program;
    if (program.find('/') == std::string::npos)
    {
        const char* search = std::getenv("PATH");
        if (search == nullptr) { return; }
        std::string paths(search);
        size_t start = 0;
        for (;;)
        {
            const size_t end = paths.find(':', start);
            const std::string dir = paths.substr(start, end - start);
            executable = joinPath(dir.empty() ? "." : dir, program);
            if (::access(executable.c_str(), X_OK) == 0) { break; }
            if (end == std::string::npos) { return; }
            start = end + 1;
        }
    }
    const char* binary = executable.c_str();
    char* argument = const_cast<char*>(path.c_str());
    char* args[] = {const_cast<char*>(binary), option == nullptr ? argument : const_cast<char*>(option),
        option == nullptr ? nullptr : argument, nullptr};
    const pid_t child = ::fork();
    if (child == 0)
    {
        if (::fork() == 0) { ::execv(binary, args); }
        ::_exit(0);
    }
    if (child > 0)
    {
        int status = 0;
        while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {}
    }
}
#endif

#if CSYSTEM_HAS_DESKTOP_API
void CSystem::OpenFolder(const std::string& folder)
{
    if (folder.empty() || folder.find('\0') != std::string::npos) { return; }
#ifdef _WIN32
    ShellExecuteA(NULL, "open", folder.c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    openDesktopPath("/usr/bin/open", nullptr, folder);
#else
    openDesktopPath("xdg-open", nullptr, folder);
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if CSYSTEM_HAS_DESKTOP_API
void CSystem::OpenFolderAndSelectFile(const std::string& file)
{
    if (file.empty() || file.find('\0') != std::string::npos) { return; }
#ifdef _WIN32
    ShellExecuteA(NULL, "open", "Explorer.exe", ("/select,\"" + file + "\"").c_str(), NULL, SW_SHOWDEFAULT);
#elif defined(__APPLE__)
    openDesktopPath("/usr/bin/open", "-R", file);
#else
    openDesktopPath("nautilus", "--select", file);
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if CSYSTEM_HAS_DESKTOP_API
void CSystem::OpenFile(const std::string& file)
{
    if (file.empty() || file.find('\0') != std::string::npos) { return; }
#ifdef _WIN32
    ShellExecuteA(NULL, "open", file.c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    openDesktopPath("/usr/bin/open", nullptr, file);
#else
    // 兼容现有“程序名 路径”入口；已有文件（包括带空格的路径）总是交给默认程序。
    const size_t space = file.find(' ');
    if (space != std::string::npos && space > 0 && space + 1 < file.size() && ::access(file.c_str(), F_OK) != 0)
    {
        openDesktopPath(file.substr(0, space), nullptr, file.substr(space + 1));
    }
    else { openDesktopPath("xdg-open", nullptr, file); }
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

void CSystem::childProcessExit(int32_t exitCode)
{
#ifdef _MSC_VER
    ::_exit(exitCode);
#elif defined(__unix__) || defined(__APPLE__)
    ::_exit(exitCode);
#else
    std::_Exit(exitCode);
#endif
}
#endif // CSYSTEM_NATIVE_API

std::string CSystem::timetToStr(time_t timet, bool isLocal)
{
    char buf[20] = {};
    tm timeInfo;
    memset(&timeInfo, 0, sizeof(timeInfo));
    if (isLocal)
    {
        // 转为本地时间
#ifdef _WIN32
        if (localtime_s(&timeInfo, &timet) != 0)
#elif defined(__unix__) || defined(__APPLE__)
        if (localtime_r(&timet, &timeInfo) == nullptr)
#endif
        {
            return "";
        }
    }
    else
    {
        // 转为UTC时间
#ifdef _WIN32
        if (gmtime_s(&timeInfo, &timet) != 0)
#elif defined(__unix__) || defined(__APPLE__)
        if (gmtime_r(&timet, &timeInfo) == nullptr)
#endif
        {
            return "";
        }
    }
    if (strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeInfo) == 0)
    {
        return "";
    }
    return buf;
}

#if CSYSTEM_HAS_NATIVE_API
std::string CSystem::commonFile(const std::string& name)
{
#ifdef _WIN32
    std::string path = CSystem::GetEnvironment("XUEYELINGSHUANG") + "common\\";
#elif defined(__unix__) || defined(__APPLE__)
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
#elif defined(__unix__) || defined(__APPLE__)
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
    if (path.find('\0') != std::string::npos)
    {
        return "";
    }
    std::unique_ptr<FILE, int (*)(FILE*)> file(fopen(path.c_str(), "rb"), &fclose);
    if (!file)
    {
        return "";
    }
    // procfs 文件与管道不一定有可查询的长度。
    char buffer[16384];
    std::string result;
    size_t count = 0;
    while ((count = fread(buffer, 1, sizeof(buffer), file.get())) != 0)
    {
        result.append(buffer, count);
    }
    if (ferror(file.get()))
    {
        return "";
    }
    return result;
}

void CSystem::saveFile(const std::string& content, const std::string& path)
{
    if (path.find('\0') != std::string::npos) { return; }
    FILE* file = fopen(path.c_str(), "wb");
    if (file == nullptr)
    {
        return;
    }
    fwrite(content.c_str(), 1, content.size(), file);
    fflush(file);
    fclose(file);
}

std::vector<std::string> CSystem::findFilePath(const std::string& strPath,
    int32_t flag,
    const std::string& fileStr,
    const std::function<bool (const std::string&)>& EveryFilePath,
    std::vector<std::string>* unVisitPath)
{
    std::vector<std::string> result;
    if (unVisitPath != nullptr)
    {
        unVisitPath->clear();
    }

    if (flag < 1 || flag > 3 || strPath.find('\0') != std::string::npos) { return result; }
    std::string dir = strPath;
    if (dir.empty())
    {
        dir = CSystem::GetCurrentExePath();
    }
    if (dir.empty())
    {
        return result;
    }
#ifdef _WIN32
    char level = '\\';
#elif defined(__unix__) || defined(__APPLE__)
    char level = '/';
#endif
    if (dir.back() != level)
    {
        dir.push_back(level);
    }
#ifdef _WIN32
    dir.push_back('*');
#endif

#ifdef _WIN32
    _finddata_t fileDir;
#elif defined(__unix__) || defined(__APPLE__)
    struct dirent *pDirent = nullptr;
#endif

    std::vector<std::string> vecLowerFileStr;
    if (flag == 2)
    {
        std::string lowerFileStr;
        std::transform(fileStr.begin(), fileStr.end(), std::back_inserter(lowerFileStr), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        split(vecLowerFileStr, lowerFileStr, '.');
    }

    //用队列实现递归
    std::queue<std::string> queue_dir;
    queue_dir.push(std::string(dir));

    while (!queue_dir.empty())
    {
        std::string curDir = queue_dir.front();
        queue_dir.pop();
#ifdef _WIN32
        auto lfDir = _findfirst(curDir.c_str(), &fileDir);
#elif defined(__unix__) || defined(__APPLE__)
        DIR* pDir = opendir(curDir.c_str());
#endif

        //如果是-1表示该文件夹不可访问
#ifdef _WIN32
        if (lfDir == -1)
#elif defined(__unix__) || defined(__APPLE__)
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
        auto closeSearch = [](intptr_t* handle) { ::_findclose(*handle); };
        std::unique_ptr<intptr_t, decltype(closeSearch)> search(&lfDir, closeSearch);
#else
        std::unique_ptr<DIR, int(*)(DIR*)> search(pDir, ::closedir);
#endif
#ifdef _WIN32
        do
#elif defined(__unix__) || defined(__APPLE__)
        while ((pDirent = readdir(pDir)) != 0)
#endif
        {
#ifdef _WIN32
            std::string strName = fileDir.name;
#elif defined(__unix__) || defined(__APPLE__)
            std::string strName = pDirent->d_name;
#endif
            //是目录，加入队列
#ifdef _WIN32
            if ((fileDir.attrib & _A_SUBDIR) != 0)
#elif defined(__unix__) || defined(__APPLE__)
            struct stat entryInfo = {};
            if (pDirent->d_type == DT_DIR || (pDirent->d_type == DT_UNKNOWN &&
                ::fstatat(::dirfd(pDir), strName.c_str(), &entryInfo, AT_SYMLINK_NOFOLLOW) == 0 && S_ISDIR(entryInfo.st_mode)))
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
#ifdef _WIN32
                const DWORD attributes = ::GetFileAttributesA(tmpstr.c_str());
                if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) { continue; }
#endif
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
                std::string nameSuffix = CSystem::GetName(strName, 2);
                std::string lowerNameSuffix;
                std::transform(nameSuffix.begin(), nameSuffix.end(), std::back_inserter(lowerNameSuffix), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                if ((vecLowerFileStr.empty() && lowerNameSuffix.empty()) ||
                    (!vecLowerFileStr.empty() && std::find(vecLowerFileStr.begin(), vecLowerFileStr.end(), lowerNameSuffix) != vecLowerFileStr.end()))
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
            {
                break;
            }
            }
            if (isExit)
            {
                return result;
            }
#ifdef _WIN32
        } while (_findnext(lfDir, &fileDir) == 0);
#elif defined(__unix__) || defined(__APPLE__)
        }
#endif
    }
    return result;
}
#endif // CSYSTEM_NATIVE_API

bool CSystem::commandArguments(int32_t argc, char** argv, std::vector<std::string>& arguments)
{
#ifdef _WIN32
    if ((argc != 0 || argv != nullptr) && (argc < 1 || argc > 4096 || argv == nullptr))
#else
    if (argc < 1 || argc > 4096 || argv == nullptr)
#endif
    {
        return false;
    }
    std::vector<std::string> values;
#ifdef _WIN32
    int count = 0;
    std::unique_ptr<wchar_t*, HLOCAL (WINAPI *)(HLOCAL)> wide(
        CommandLineToArgvW(GetCommandLineW(), &count), &LocalFree);
    if (!wide || count < 1 || count > 4096)
    {
        return false;
    }
    for (int32_t index = 1; index < count; ++index)
    {
        const std::wstring text(wide.get()[index]);
        const std::string converted = widePathToUtf8(text);
        if (!text.empty() && converted.empty())
        {
            return false;
        }
        values.push_back(converted);
    }
#else
    for (int32_t index = 1; index < argc; ++index)
    {
        if (argv[index] == nullptr)
        {
            return false;
        }
        values.push_back(argv[index]);
    }
#endif
    arguments.swap(values);
    return true;
}

#if CSYSTEM_HAS_NATIVE_API
std::string CSystem::canonicalPathUtf8(const std::string& path, bool requireExisting)
{
    if (path.empty() || path.size() > 4096 || path.find('\0') != std::string::npos)
    {
        return "";
    }
#ifdef _WIN32
    const std::wstring input = utf8PathToWide(path);
    if (input.empty())
    {
        return "";
    }
    DWORD needed = GetFullPathNameW(input.c_str(), 0, nullptr, nullptr);
    if (needed == 0 || needed > 32768)
    {
        return "";
    }
    std::vector<wchar_t> wide(needed + 1);
    DWORD count = GetFullPathNameW(input.c_str(), static_cast<DWORD>(wide.size()), wide.data(), nullptr);
    if (count == 0 || count >= wide.size())
    {
        return "";
    }
    if (requireExisting)
    {
        HANDLE file = CreateFileW(wide.data(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                  OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (file == INVALID_HANDLE_VALUE)
        {
            return "";
        }
        needed = GetFinalPathNameByHandleW(file, nullptr, 0, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
        if (needed == 0 || needed > 32768)
        {
            CloseHandle(file);
            return "";
        }
        try
        {
            wide.resize(needed + 1);
        }
        catch (...)
        {
            CloseHandle(file);
            throw;
        }
        count = GetFinalPathNameByHandleW(file, wide.data(), static_cast<DWORD>(wide.size()), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
        CloseHandle(file);
        if (count == 0 || count >= wide.size())
        {
            return "";
        }
    }
    std::wstring value(wide.data(), count);
    if (value.compare(0, 8, L"\\\\?\\UNC\\") == 0)
    {
        value = L"\\\\" + value.substr(8);
    }
    else if (value.compare(0, 4, L"\\\\?\\") == 0)
    {
        value.erase(0, 4);
    }
    std::string result = widePathToUtf8(value);
    for (char& byte : result)
    {
        if (byte == '\\')
        {
            byte = '/';
        }
        else if (byte >= 'A' && byte <= 'Z')
        {
            byte = static_cast<char>(byte - 'A' + 'a');
        }
    }
    while (result.size() > 3 && result.back() == '/')
    {
        result.pop_back();
    }
    return result;
#else
    return canonicalPath(path, requireExisting);
#endif
}

bool CSystem::createDirectoriesUtf8(const std::string& path)
{
#ifdef _WIN32
    const std::string absolute = canonicalPathUtf8(path, false);
    if (absolute.empty())
    {
        return false;
    }
    const std::wstring wide = utf8PathToWide(absolute);
    const DWORD attributes = GetFileAttributesW(wide.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES)
    {
        return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0;
    }
    const std::string parent = GetName(absolute, 4);
    if (parent.empty() || parent == absolute || !createDirectoriesUtf8(parent))
    {
        return false;
    }
    if (!CreateDirectoryW(wide.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
    {
        return false;
    }
    const DWORD created = GetFileAttributesW(wide.c_str());
    return created != INVALID_FILE_ATTRIBUTES && (created & FILE_ATTRIBUTE_DIRECTORY) != 0 && (created & FILE_ATTRIBUTE_REPARSE_POINT) == 0;
#else
    return createDirectories(path);
#endif
}

intptr_t CSystem::openFileHandle(const char* path, uint32_t mode, uint32_t* error)
{
    if (error)
    {
        *error = 0;
    }
    if (!path || !*path || std::strlen(path) > 4096 || mode < 1 || mode > 3)
    {
#ifdef _WIN32
        systemFileFailure(error, ERROR_INVALID_PARAMETER);
#else
        systemFileFailure(error, EINVAL);
#endif
        return -1;
    }
    intptr_t handle = -1;
#ifdef _WIN32
    const std::wstring wide = utf8PathToWide(path);
    if (wide.empty())
    {
        systemFileFailure(error, ERROR_NO_UNICODE_TRANSLATION);
        return -1;
    }
    const HANDLE native = CreateFileW(wide.c_str(), mode == 1 ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
        mode == 1 ? FILE_SHARE_READ : 0, nullptr, mode == 2 ? CREATE_NEW : OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (native == INVALID_HANDLE_VALUE)
    {
        systemFileFailure(error, GetLastError());
        return -1;
    }
    handle = reinterpret_cast<intptr_t>(native);
#else
    handle = ::open(path, O_CLOEXEC | O_NOFOLLOW | (mode == 1 ? O_RDONLY : O_RDWR) | (mode == 2 ? O_CREAT | O_EXCL : 0), 0600);
    if (handle < 0)
    {
        systemFileFailure(error, errno);
        return -1;
    }
#endif
    if (!validateSystemFileHandle(handle, error))
    {
        closeFileHandle(&handle, nullptr);
        return -1;
    }
    return handle;
}

intptr_t CSystem::createObservedOutputFile(const char* path, uint32_t* error)
{
#ifdef _WIN32
    if (error)
    {
        *error = 0;
    }
    if (!path || !*path || std::strlen(path) > 4096)
    {
        systemFileFailure(error, ERROR_INVALID_PARAMETER);
        return -1;
    }
    const std::wstring wide = utf8PathToWide(path);
    if (wide.empty())
    {
        systemFileFailure(error, ERROR_NO_UNICODE_TRANSLATION);
        return -1;
    }
    const HANDLE native = CreateFileW(wide.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (native == INVALID_HANDLE_VALUE)
    {
        systemFileFailure(error, GetLastError());
        return -1;
    }
    intptr_t handle = reinterpret_cast<intptr_t>(native);
    if (!validateSystemFileHandle(handle, error))
    {
        closeFileHandle(&handle);
        return -1;
    }
    return handle;
#else
    return openFileHandle(path, 2, error);
#endif
}

intptr_t CSystem::duplicateFileHandle(intptr_t native, uint32_t* error)
{
    if (error)
    {
        *error = 0;
    }
    if (!isSystemFileHandle(native))
    {
#ifdef _WIN32
        systemFileFailure(error, ERROR_INVALID_PARAMETER);
#else
        systemFileFailure(error, EINVAL);
#endif
        return -1;
    }
    intptr_t handle = -1;
#ifdef _WIN32
    HANDLE value = INVALID_HANDLE_VALUE;
    if (!DuplicateHandle(GetCurrentProcess(), reinterpret_cast<HANDLE>(native), GetCurrentProcess(), &value, 0, FALSE, DUPLICATE_SAME_ACCESS))
    { systemFileFailure(error, GetLastError()); return -1; }
    handle = reinterpret_cast<intptr_t>(value);
#else
    handle = fcntl(static_cast<int>(native), F_DUPFD_CLOEXEC, 0);
    if (handle < 0)
    {
        systemFileFailure(error, errno);
        return -1;
    }
#endif
    if (!validateSystemFileHandle(handle, error))
    {
        closeFileHandle(&handle, nullptr);
        return -1;
    }
    return handle;
}

bool CSystem::readFileHandle(intptr_t handle, uint64_t offset, uint8_t* output, uint32_t capacity, uint32_t* received, uint32_t* error)
{
    if (received)
    {
        *received = 0;
    }
    if (error)
    {
        *error = 0;
    }
    if (!isSystemFileHandle(handle))
    {
#ifdef _WIN32
        return systemFileFailure(error, ERROR_INVALID_HANDLE);
#else
        return systemFileFailure(error, EBADF);
#endif
    }
    if (received == nullptr || (capacity != 0 && output == nullptr) || capacity > 16777216 ||
        offset > static_cast<uint64_t>(INT64_MAX) - capacity)
    {
#ifdef _WIN32
        return systemFileFailure(error, ERROR_INVALID_PARAMETER);
#else
        return systemFileFailure(error, EINVAL);
#endif
    }
    uint32_t used = 0;
#ifdef _WIN32
    if (reinterpret_cast<HANDLE>(handle) == INVALID_HANDLE_VALUE)
    {
        return systemFileFailure(error, ERROR_INVALID_HANDLE);
    }
    LARGE_INTEGER position;
    position.QuadPart = static_cast<LONGLONG>(offset);
    if (!SetFilePointerEx(reinterpret_cast<HANDLE>(handle), position, nullptr, FILE_BEGIN))
    {
        return systemFileFailure(error, GetLastError());
    }
    while (used < capacity)
    {
        DWORD count = 0;
        if (!ReadFile(reinterpret_cast<HANDLE>(handle), output + used, capacity - used, &count, nullptr))
        {
            return systemFileFailure(error, GetLastError());
        }
        if (count == 0)
        {
            break;
        }
        used += count;
    }
#else
    if (static_cast<int>(handle) < 0)
    {
        return systemFileFailure(error, EBADF);
    }
    uint32_t interrupted = 0;
    while (used < capacity)
    {
        const ssize_t count = pread(static_cast<int>(handle), output + used, capacity - used, static_cast<off_t>(offset + used));
        if (count < 0)
        {
            if (errno == EINTR && ++interrupted <= 16)
            {
                continue;
            }
            return systemFileFailure(error, errno);
        }
        if (count == 0)
        {
            break;
        }
        used += static_cast<uint32_t>(count);
    }
#endif
    *received = used;
    if (error)
    {
        *error = 0;
    }
    return true;
}

bool CSystem::writeFileHandle(intptr_t handle, uint64_t offset, const uint8_t* input, uint32_t size, uint32_t* error)
{
    if (error)
    {
        *error = 0;
    }
    if (!isSystemFileHandle(handle))
    {
#ifdef _WIN32
        return systemFileFailure(error, ERROR_INVALID_HANDLE);
#else
        return systemFileFailure(error, EBADF);
#endif
    }
    if ((size != 0 && input == nullptr) || size > 16777216 || offset > static_cast<uint64_t>(INT64_MAX) - size)
    {
#ifdef _WIN32
        return systemFileFailure(error, ERROR_INVALID_PARAMETER);
#else
        return systemFileFailure(error, EINVAL);
#endif
    }
    uint32_t used = 0;
#ifdef _WIN32
    if (reinterpret_cast<HANDLE>(handle) == INVALID_HANDLE_VALUE)
    {
        return systemFileFailure(error, ERROR_INVALID_HANDLE);
    }
    LARGE_INTEGER position;
    position.QuadPart = static_cast<LONGLONG>(offset);
    if (!SetFilePointerEx(reinterpret_cast<HANDLE>(handle), position, nullptr, FILE_BEGIN))
    {
        return systemFileFailure(error, GetLastError());
    }
    while (used < size)
    {
        DWORD count = 0;
        if (!WriteFile(reinterpret_cast<HANDLE>(handle), input + used, size - used, &count, nullptr))
        {
            return systemFileFailure(error, GetLastError());
        }
        if (count == 0)
        {
            return systemFileFailure(error, ERROR_WRITE_FAULT);
        }
        used += count;
    }
#else
    if (static_cast<int>(handle) < 0)
    {
        return systemFileFailure(error, EBADF);
    }
    uint32_t interrupted = 0;
    while (used < size)
    {
        const ssize_t count = pwrite(static_cast<int>(handle), input + used, size - used, static_cast<off_t>(offset + used));
        if (count < 0)
        {
            if (errno == EINTR && ++interrupted <= 16)
            {
                continue;
            }
            return systemFileFailure(error, errno);
        }
        if (count == 0)
        {
            return systemFileFailure(error, EIO);
        }
        used += static_cast<uint32_t>(count);
    }
#endif
    if (error)
    {
        *error = 0;
    }
    return true;
}

bool CSystem::flushFileHandle(intptr_t handle, uint32_t* error)
{
    if (error)
    {
        *error = 0;
    }
    if (!isSystemFileHandle(handle))
    {
#ifdef _WIN32
        return systemFileFailure(error, ERROR_INVALID_HANDLE);
#else
        return systemFileFailure(error, EBADF);
#endif
    }
#ifdef _WIN32
    if (reinterpret_cast<HANDLE>(handle) == INVALID_HANDLE_VALUE)
    {
        return systemFileFailure(error, ERROR_INVALID_HANDLE);
    }
    if (!FlushFileBuffers(reinterpret_cast<HANDLE>(handle)))
    {
        return systemFileFailure(error, GetLastError());
    }
#else
    if (static_cast<int>(handle) < 0)
    {
        return systemFileFailure(error, EBADF);
    }
    if (fsync(static_cast<int>(handle)) != 0)
    {
        return systemFileFailure(error, errno);
    }
#endif
    if (error)
    {
        *error = 0;
    }
    return true;
}

bool CSystem::fileHandleSize(intptr_t handle, uint64_t* bytes, uint32_t* error)
{
    if (bytes)
    {
        *bytes = 0;
    }
    if (error)
    {
        *error = 0;
    }
    if (!isSystemFileHandle(handle))
    {
#ifdef _WIN32
        return systemFileFailure(error, ERROR_INVALID_HANDLE);
#else
        return systemFileFailure(error, EBADF);
#endif
    }
    if (bytes != nullptr)
    {
        *bytes = 0;
    }
    if (bytes == nullptr)
    {
#ifdef _WIN32
        return systemFileFailure(error, ERROR_INVALID_PARAMETER);
#else
        return systemFileFailure(error, EINVAL);
#endif
    }
#ifdef _WIN32
    LARGE_INTEGER count;
    if (!GetFileSizeEx(reinterpret_cast<HANDLE>(handle), &count))
    {
        return systemFileFailure(error, GetLastError());
    }
    if (count.QuadPart < 0)
    {
        return systemFileFailure(error, ERROR_INVALID_DATA);
    }
    *bytes = static_cast<uint64_t>(count.QuadPart);
#else
    struct stat information = {};
    if (fstat(static_cast<int>(handle), &information) != 0)
    {
        return systemFileFailure(error, errno);
    }
    if (information.st_size < 0)
    {
        return systemFileFailure(error, EINVAL);
    }
    *bytes = static_cast<uint64_t>(information.st_size);
#endif
    if (error)
    {
        *error = 0;
    }
    return true;
}

bool CSystem::resizeFileHandle(intptr_t handle, uint64_t bytes, uint32_t* error)
{
    if (error)
    {
        *error = 0;
    }
    if (!isSystemFileHandle(handle))
    {
#ifdef _WIN32
        return systemFileFailure(error, ERROR_INVALID_HANDLE);
#else
        return systemFileFailure(error, EBADF);
#endif
    }
    if (bytes > INT64_MAX)
    {
#ifdef _WIN32
        return systemFileFailure(error, ERROR_INVALID_PARAMETER);
#else
        return systemFileFailure(error, EINVAL);
#endif
    }
#ifdef _WIN32
    LARGE_INTEGER position;
    position.QuadPart = static_cast<LONGLONG>(bytes);
    if (!SetFilePointerEx(reinterpret_cast<HANDLE>(handle), position, nullptr, FILE_BEGIN) || !SetEndOfFile(reinterpret_cast<HANDLE>(handle)))
    {
        return systemFileFailure(error, GetLastError());
    }
#else
    if (ftruncate(static_cast<int>(handle), static_cast<off_t>(bytes)) != 0)
    {
        return systemFileFailure(error, errno);
    }
#endif
    if (error)
    {
        *error = 0;
    }
    return true;
}

bool CSystem::closeFileHandle(intptr_t* handle, uint32_t* error)
{
    if (error)
    {
        *error = 0;
    }
    if (!handle)
    {
#ifdef _WIN32
        return systemFileFailure(error, ERROR_INVALID_PARAMETER);
#else
        return systemFileFailure(error, EINVAL);
#endif
    }
    if (*handle == -1)
    {
        return true;
    }
    const intptr_t value = *handle;
    *handle = -1;
    if (!isSystemFileHandle(value))
    {
#ifdef _WIN32
        return systemFileFailure(error, ERROR_INVALID_HANDLE);
#else
        return systemFileFailure(error, EBADF);
#endif
    }
#ifdef _WIN32
    if (!CloseHandle(reinterpret_cast<HANDLE>(value)))
    {
        return systemFileFailure(error, GetLastError());
    }
#else
    if (::close(static_cast<int>(value)) != 0)
    {
        return systemFileFailure(error, errno);
    }
#endif
    return true;
}

bool CSystem::publishFileUtf8(const char* source, const char* destination, bool replace)
{
    if (source == nullptr || destination == nullptr || source[0] == 0 || destination[0] == 0 || std::strlen(source) > 4096 ||
        std::strlen(destination) > 4096 || std::strcmp(source, destination) == 0)
    {
        return false;
    }
#ifdef _WIN32
    const std::wstring from = utf8PathToWide(source);
    const std::wstring to = utf8PathToWide(destination);
    if (from.empty() || to.empty())
    {
        return false;
    }
    const DWORD attributes = GetFileAttributesW(from.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0)
    {
        return false;
    }
    return MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_WRITE_THROUGH | (replace ? MOVEFILE_REPLACE_EXISTING : 0)) != FALSE;
#else
    struct stat information = {};
    if (lstat(source, &information) != 0 || !S_ISREG(information.st_mode))
    {
        return false;
    }
    if (replace)
    {
        if (::rename(source, destination) != 0)
        {
            return false;
        }
    }
    else
    {
        if (::link(source, destination) != 0)
        {
            return false;
        }
        // 目标已经原子发布，残留临时链接交宿主恢复时清理，不伪装成没有发布。
        ::unlink(source);
    }
    const std::string parent = CSystem::GetName(destination, 4);
    const int directory = ::open(parent.empty() ? "." : parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory < 0)
    {
        return false;
    }
    const bool durable = fsync(directory) == 0;
    ::close(directory);
    return durable;
#endif
}

#if defined(_WIN32)
bool CSystem::bindInheritedOutput()
{
#ifdef _WIN32
    const DWORD identifiers[] = { STD_OUTPUT_HANDLE, STD_ERROR_HANDLE };
    FILE* streams[] = { stdout, stderr };
    for (uint32_t index = 0; index < 2; ++index)
    {
        HANDLE source = GetStdHandle(identifiers[index]);
        if (source == nullptr || source == INVALID_HANDLE_VALUE)
        {
            continue;
        }
        const DWORD type = GetFileType(source);
        if (type != FILE_TYPE_DISK && type != FILE_TYPE_PIPE)
        {
            continue;
        }
        HANDLE copy = nullptr;
        if (!DuplicateHandle(GetCurrentProcess(), source, GetCurrentProcess(), &copy, 0, FALSE, DUPLICATE_SAME_ACCESS))
        {
            return false;
        }
        const int descriptor = _open_osfhandle(reinterpret_cast<intptr_t>(copy), _O_WRONLY | _O_BINARY);
        if (descriptor < 0)
        {
            CloseHandle(copy);
            return false;
        }
        const bool bound = _wfreopen(L"NUL", L"wb", streams[index]) != nullptr && _dup2(descriptor, _fileno(streams[index])) == 0;
        _close(descriptor);
        if (!bound)
        {
            return false;
        }
    }
#endif
    return true;
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
uint32_t CSystem::startDetachedProcess(const std::string& executable, const std::vector<std::string>& arguments,
    uint32_t& pid, uint64_t& monitor)
{
    return startOwnedProcess(executable, arguments, std::string(), pid, monitor);
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
uint32_t CSystem::startCapturedProcess(const std::string& executable, const std::vector<std::string>& arguments,
    const std::string& output, uint32_t& pid, uint64_t& monitor)
{
    if (output.empty())
    {
        pid = 0;
        monitor = 0;
        return 87;
    }
    return startOwnedProcess(executable, arguments, output, pid, monitor);
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
bool CSystem::queryProcessResources(uint64_t monitor, uint64_t& privateBytes, uint64_t& workingSet,
    uint64_t& cpuTime100ns, uint32_t& handles, uint32_t& threads)
{
    privateBytes = workingSet = cpuTime100ns = 0;
    handles = threads = 0;
#ifdef _WIN32
    HANDLE process = monitor == 0 ? GetCurrentProcess() : reinterpret_cast<HANDLE>(static_cast<uintptr_t>(monitor));
    PROCESS_MEMORY_COUNTERS_EX memory = {};
    memory.cb = sizeof(memory);
    FILETIME created = {}, exited = {}, kernel = {}, user = {};
    DWORD count = 0;
    const DWORD pid = GetProcessId(process);
    if (pid == 0 || !GetProcessMemoryInfo(process, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory)) ||
        !GetProcessTimes(process, &created, &exited, &kernel, &user) || !GetProcessHandleCount(process, &count))
    {
        return false;
    }
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    bool found = false;
    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (entry.th32ProcessID == pid)
            {
                threads = entry.cntThreads;
                found = true;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    if (!found)
    {
        return false;
    }
    privateBytes = memory.PrivateUsage;
    workingSet = memory.WorkingSetSize;
    cpuTime100ns = (static_cast<uint64_t>(kernel.dwHighDateTime) << 32 | kernel.dwLowDateTime) +
        (static_cast<uint64_t>(user.dwHighDateTime) << 32 | user.dwLowDateTime);
    handles = count;
    return true;
#else
    (void)monitor;
    return false;
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
uint64_t CSystem::openProcessMonitor(uint32_t pid)
{
#ifdef _WIN32
    return pid == 0 ? 0 : reinterpret_cast<uintptr_t>(OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
#else
    (void)pid;
    return 0;
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
int32_t CSystem::pollProcessMonitor(uint64_t monitor, int32_t& exitCode)
{
    exitCode = 0;
#ifdef _WIN32
    if (monitor == 0)
    {
        return -1;
    }
    HANDLE process = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(monitor));
    const DWORD state = WaitForSingleObject(process, 0);
    if (state == WAIT_TIMEOUT)
    {
        return 0;
    }
    DWORD code = 0;
    if (state != WAIT_OBJECT_0 || !GetExitCodeProcess(process, &code))
    {
        return -1;
    }
    exitCode = static_cast<int32_t>(code);
    return 1;
#else
    (void)monitor;
    return -1;
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
void CSystem::closeProcessMonitor(uint64_t& monitor)
{
#ifdef _WIN32
    if (monitor != 0)
    {
        CloseHandle(reinterpret_cast<HANDLE>(static_cast<uintptr_t>(monitor)));
    }
#endif
    monitor = 0;
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
uint32_t CSystem::setUserLoginStartup(const std::string& name, const std::string& command)
{
    if (name.empty() || name.size() > 80 ||
        name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") != std::string::npos ||
        command.size() > 4096 || command.find('\0') != std::string::npos)
    {
        return 87;
    }
#ifdef _WIN32
    const std::wstring value = utf8PathToWide(name);
    const std::wstring wide = utf8PathToWide(command);
    if ((!command.empty() && wide.empty()) || wide.size() > 260)
    {
        return ERROR_INVALID_PARAMETER;
    }
    HKEY key = nullptr;
    const wchar_t* path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    LONG result = command.empty() ? RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, KEY_SET_VALUE, &key) :
        RegCreateKeyExW(HKEY_CURRENT_USER, path, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
    if (command.empty() && result == ERROR_FILE_NOT_FOUND)
    {
        return ERROR_SUCCESS;
    }
    if (result != ERROR_SUCCESS)
    {
        return static_cast<uint32_t>(result);
    }
    result = command.empty() ? RegDeleteValueW(key, value.c_str()) :
        RegSetValueExW(key, value.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(wide.c_str()),
            static_cast<DWORD>((wide.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
    return command.empty() && result == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : static_cast<uint32_t>(result);
#else
    return 50;
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if CSYSTEM_HAS_DESKTOP_API
int32_t CSystem::acquireProcessLease(const std::string& name, bool replace, uint32_t timeoutMs,
    std::vector<uint64_t>& state)
{
    return acquireNamedProcessLease(name, name, replace, timeoutMs, state);
}

int32_t CSystem::acquireProcessLease(const std::string& name, bool replace, uint32_t timeoutMs,
    std::vector<uint64_t>& state, const std::string& isolated)
{
    if (isolated.empty()) { return acquireProcessLease(name, replace, timeoutMs, state); }
    if (name.size() > 80 || isolated.size() > 40 ||
        isolated.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") != std::string::npos)
    {
        return -1;
    }
    try { return acquireNamedProcessLease(name, name + ".test." + isolated, replace, timeoutMs, state); }
    catch (...) { return -1; }
}

int32_t CSystem::acquireNamedProcessLease(const std::string& name, const std::string& slot, bool replace,
    uint32_t timeoutMs, std::vector<uint64_t>& state)
{
    if (!state.empty() || name.empty() || name.size() > 80 || timeoutMs > 30000 ||
        name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") != std::string::npos)
    {
        return -1;
    }
    try
    {
        // 准备输出存储及全部对象名称后再取得系统资源；不能因异常遗留已拥有的租约。
        state.reserve(3);
#ifdef _WIN32
        // 固定Global名称保证不同安装目录使用同一个角色租约
        const std::string prefix = "Global\\xyls." + slot;
        const std::string ownerName = prefix + ".owner";
        const std::string stopName = prefix + ".stop";
        const std::string readyName = "Global\\xyls." + name + ".ready." + std::to_string(currentProcessPid());
        HANDLE owner = CreateMutexA(nullptr, FALSE, ownerName.c_str());
        if (owner == nullptr)
        {
            return -1;
        }
        DWORD acquired = WaitForSingleObject(owner, 0);
        if (acquired == WAIT_TIMEOUT && !replace)
        {
            CloseHandle(owner);
            return 0;
        }
        const auto start = std::chrono::steady_clock::now();
        while (acquired == WAIT_TIMEOUT && replace)
        {
            HANDLE stop = OpenEventA(EVENT_MODIFY_STATE, FALSE, stopName.c_str());
            if (stop != nullptr)
            {
                SetEvent(stop);
                CloseHandle(stop);
            }
            if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() >= timeoutMs)
            {
                CloseHandle(owner);
                return -1;
            }
            acquired = WaitForSingleObject(owner, 20);
        }
        if (acquired != WAIT_OBJECT_0 && acquired != WAIT_ABANDONED)
        {
            CloseHandle(owner);
            return -1;
        }
        HANDLE stop = CreateEventA(nullptr, TRUE, FALSE, stopName.c_str());
        HANDLE ready = CreateEventA(nullptr, TRUE, FALSE, readyName.c_str());
        if (stop == nullptr || ready == nullptr)
        {
            if (stop != nullptr)
            {
                CloseHandle(stop);
            }
            if (ready != nullptr)
            {
                CloseHandle(ready);
            }
            ReleaseMutex(owner);
            CloseHandle(owner);
            return -1;
        }
        ResetEvent(stop);
        ResetEvent(ready);
        try
        {
            state.push_back(reinterpret_cast<uint64_t>(owner));
            state.push_back(reinterpret_cast<uint64_t>(stop));
            state.push_back(reinterpret_cast<uint64_t>(ready));
        }
        catch (...)
        {
            state.clear(); CloseHandle(ready); CloseHandle(stop); ReleaseMutex(owner); CloseHandle(owner);
            return -1;
        }
        return 1;
#elif defined(__linux__) || defined(__APPLE__)
        const std::string file = "/tmp/xyls." + slot + ".lease";
        const uint32_t record[3] = { currentProcessPid(), 0, 0 };
        const std::string readyFile = "/tmp/xyls." + name + ".ready." + std::to_string(record[0]);
        const int descriptor = open(file.c_str(), O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0666);
        if (descriptor < 0)
        {
            return -1;
        }
        struct stat info;
        if (fstat(descriptor, &info) != 0 || !S_ISREG(info.st_mode) || info.st_nlink != 1)
        {
            close(descriptor);
            return -1;
        }
        const auto start = std::chrono::steady_clock::now();
        while (flock(descriptor, LOCK_EX | LOCK_NB) != 0)
        {
            if (errno != EWOULDBLOCK && errno != EAGAIN)
            {
                close(descriptor);
                return -1;
            }
            if (!replace)
            {
                close(descriptor);
                return 0;
            }
            const uint32_t requested = 1;
            if (pwrite(descriptor, &requested, 4, 8) != 4 ||
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() >= timeoutMs)
            {
                close(descriptor); return -1;
            }
            Sleep(20);
        }
        const int ready = open(readyFile.c_str(), O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0644);
        if (ready < 0 || fstat(ready, &info) != 0 || !S_ISREG(info.st_mode) || info.st_nlink != 1 ||
            flock(ready, LOCK_EX | LOCK_NB) != 0 || pwrite(descriptor, record, sizeof(record), 0) != sizeof(record))
        {
            if (ready >= 0)
            {
                close(ready);
            }
            close(descriptor); return -1;
        }
        const uint32_t zero = 0;
        if (pwrite(ready, &zero, 4, 0) != 4)
        {
            close(ready);
            close(descriptor);
            return -1;
        }
        try { state.push_back(descriptor); state.push_back(ready); state.push_back(0); }
        catch (...) { state.clear(); close(ready); close(descriptor); return -1; }
        return 1;
#else
        (void)replace; (void)slot;
        return -1;
#endif
    }
    catch (...)
    {
        // 此处只接到取得资源之前的准备异常；句柄后的提交仍有各平台就地回滚。
        state.clear();
        return -1;
    }
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if CSYSTEM_HAS_DESKTOP_API
bool CSystem::setProcessLeaseReady(const std::vector<uint64_t>& state, bool ready)
{
    if (state.size() != 3)
    {
        return false;
    }
#ifdef _WIN32
    return (ready ? SetEvent(reinterpret_cast<HANDLE>(state[2])) : ResetEvent(reinterpret_cast<HANDLE>(state[2]))) != 0;
#elif defined(__linux__) || defined(__APPLE__)
    const uint32_t value = ready ? 1 : 0;
    return pwrite(static_cast<int>(state[1]), &value, 4, 0) == 4;
#else
    (void)ready;
    return false;
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if CSYSTEM_HAS_DESKTOP_API
bool CSystem::processLeaseStopRequested(const std::vector<uint64_t>& state)
{
    if (state.size() != 3)
    {
        return true;
    }
#ifdef _WIN32
    return WaitForSingleObject(reinterpret_cast<HANDLE>(state[1]), 0) != WAIT_TIMEOUT;
#elif defined(__linux__) || defined(__APPLE__)
    uint32_t value = 0;
    return pread(static_cast<int>(state[0]), &value, 4, 8) != 4 || value != 0;
#else
    return true;
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if CSYSTEM_HAS_DESKTOP_API
void CSystem::releaseProcessLease(std::vector<uint64_t>& state)
{
    if (state.size() != 3)
    {
        state.clear();
        return;
    }
    setProcessLeaseReady(state, false);
#ifdef _WIN32
    CloseHandle(reinterpret_cast<HANDLE>(state[2]));
    CloseHandle(reinterpret_cast<HANDLE>(state[1]));
    ReleaseMutex(reinterpret_cast<HANDLE>(state[0]));
    CloseHandle(reinterpret_cast<HANDLE>(state[0]));
#elif defined(__linux__) || defined(__APPLE__)
    close(static_cast<int>(state[1]));
    close(static_cast<int>(state[0]));
#endif
    state.clear();
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if CSYSTEM_HAS_DESKTOP_API
bool CSystem::isProcessLeaseReady(const std::string& name, uint32_t pid)
{
    if (pid == 0 || pid > INT32_MAX || name.empty() || name.size() > 80 ||
        name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") != std::string::npos)
    {
        return false;
    }
#ifdef _WIN32
    const std::string key = "Global\\xyls." + name + ".ready." + std::to_string(pid);
    HANDLE ready = OpenEventA(SYNCHRONIZE, FALSE, key.c_str());
    if (ready == nullptr)
    {
        return false;
    }
    const bool result = WaitForSingleObject(ready, 0) == WAIT_OBJECT_0;
    CloseHandle(ready);
    return result && !processDefinitelyExited(pid);
#elif defined(__linux__) || defined(__APPLE__)
    const std::string key = "/tmp/xyls." + name + ".ready." + std::to_string(pid);
    const int descriptor = open(key.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (descriptor < 0)
    {
        return false;
    }
    uint32_t value = 0;
    struct stat info;
    const bool ownerHeld = flock(descriptor, LOCK_SH | LOCK_NB) != 0 && (errno == EWOULDBLOCK || errno == EAGAIN);
    const bool result = ownerHeld && fstat(descriptor, &info) == 0 && S_ISREG(info.st_mode) && info.st_nlink == 1 &&
        pread(descriptor, &value, 4, 0) == 4 && value == 1;
    close(descriptor);
    return result && (kill(static_cast<pid_t>(pid), 0) == 0 || errno == EPERM);
#else
    return false;
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

bool CSystem::secureRandomBytes(uint8_t* output, uint32_t size)
{
    if ((output == nullptr && size != 0) || size > 8192)
    {
        return false;
    }
    if (size == 0)
    {
        return true;
    }
    bool success = false;
#ifdef _WIN32
    success = BCryptGenRandom(nullptr, output, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
#elif defined(__APPLE__)
    success = SecRandomCopyBytes(kSecRandomDefault, size, output) == 0;
#elif defined(__linux__) && !defined(__EMSCRIPTEN__) && defined(SYS_getrandom)
    uint32_t offset = 0;
    uint32_t interruptions = 0;
    while (offset < size)
    {
        const uint32_t count = (std::min)(size - offset, 256U);
        const long received = syscall(SYS_getrandom, output + offset, count, GRND_NONBLOCK);
        if (received < 0 && errno == EINTR && interruptions < 8)
        {
            ++interruptions;
            continue;
        }
        if (received <= 0 || static_cast<uint64_t>(received) > count)
        {
            break;
        }
        offset += static_cast<uint32_t>(received);
    }
    success = offset == size;
#endif
    if (!success)
    {
        // 仅用于防止失败清理被优化掉，不作为任何跨线程同步手段
        volatile uint8_t* target = output;
        for (uint32_t index = 0; index < size; ++index)
        {
            target[index] = 0;
        }
    }
    return success;
}

int32_t CSystem::secureRandomBytesCallback(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    return secureRandomBytes(output, size) ? 0 : -1;
}

bool CSystem::setProcessDirectory(const std::string& directory)
{
#ifdef CSYSTEM_SHARED_IDENTITY
    if (processDirectorySealed().load())
    {
        return directory == processDirectory();
    }
    if (directory.empty() || directory.find('\0') != std::string::npos)
    {
        return false;
    }
#ifdef _WIN32
    const std::wstring path = utf8PathToWide(directory);
    const bool driveAbsolute = path.size() >= 3 && path[1] == L':' && (path[2] == L'\\' || path[2] == L'/');
    const bool networkAbsolute = path.size() >= 3 && path.compare(0, 2, L"\\\\") == 0;
    if (!driveAbsolute && !networkAbsolute)
    {
        return false;
    }
    const DWORD attributes = ::GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ||
        (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
    {
        return false;
    }
#else
    const int descriptor = openProcessDirectory(directory);
    if (descriptor < 0)
    {
        return false;
    }
    ::close(descriptor);
#endif
    processDirectory() = directory;
    return true;
#else
    return directory.empty();
#endif
}

// 按单字符分隔字符串，函数开始时清空输出数组。
void CSystem::split(std::vector<std::string>& result, const std::string& splitString, char separate_character)
{
    result.clear();
    size_t splitCount = 1;
    size_t lastPosition = 0;
    size_t index = splitString.find(separate_character, lastPosition);
    while (index != std::string::npos)
    {
        ++splitCount;
        lastPosition = index + 1;
        index = splitString.find(separate_character, lastPosition);
    }
    result.reserve(splitCount);

    lastPosition = 0;
    index = splitString.find(separate_character, lastPosition);
    while (index != std::string::npos)
    {
        result.emplace_back(splitString, lastPosition, index - lastPosition);
        lastPosition = index + 1;
        index = splitString.find(separate_character, lastPosition);
    }
    result.emplace_back(splitString, lastPosition);
}

// 按字符串分隔字符串，函数开始时清空输出数组。
void CSystem::split(std::vector<std::string>& result, const std::string& splitString, const std::string& separate_character)
{
    result.clear();
    if (separate_character.empty())
    {
        result.push_back(splitString);
        return;
    }

    size_t separate_characterLen = separate_character.length();
    size_t splitCount = 1;
    size_t lastPosition = 0;
    size_t index = splitString.find(separate_character, lastPosition);
    while (index != std::string::npos)
    {
        ++splitCount;
        lastPosition = index + separate_characterLen;
        index = splitString.find(separate_character, lastPosition);
    }
    result.reserve(splitCount);

    lastPosition = 0;
    index = splitString.find(separate_character, lastPosition);
    while (index != std::string::npos)
    {
        result.emplace_back(splitString, lastPosition, index - lastPosition);
        lastPosition = index + separate_characterLen;
        index = splitString.find(separate_character, lastPosition);
    }
    result.emplace_back(splitString, lastPosition);
}

// SetSystemError入参：errorText是可选错误输出对象，value是要写入的错误文本。
// SetSystemError出参：errorText不为空时写入错误文本。
// SetSystemError返回值：无。
void CSystem::setSystemError(std::string* errorText, const std::string& value)
{
    if (errorText != nullptr)
    {
        *errorText = value;
    }
}

std::string CSystem::widePathToUtf8(const std::wstring& text)
{
    try
    {
#ifdef _WIN32
        if (text.empty() || text.size() > static_cast<size_t>(INT32_MAX))
        {
            return "";
        }
        const int32_t count = static_cast<int32_t>(text.size());
        const int32_t required = ::WideCharToMultiByte(CP_UTF8, 0, text.data(), count, nullptr, 0, nullptr, nullptr);
        if (required <= 0)
        {
            return "";
        }
        std::string result(static_cast<size_t>(required), '\0');
        if (::WideCharToMultiByte(CP_UTF8, 0, text.data(), count, &result[0], required, nullptr, nullptr) != required)
        {
            return "";
        }
        return result;
#elif defined(__unix__) || defined(__APPLE__)
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(text);
#else
        return "";
#endif
    }
    catch (...)
    {
        return "";
    }
}

#ifdef _MSC_VER
// 枚举指定DirectShow设备类别，成功后调用方负责释放返回的枚举器。
HRESULT CSystem::enumerateDevices(REFGUID category, IEnumMoniker** ppEnum)
{
    if (ppEnum == nullptr)
    {
        return E_POINTER;
    }

    *ppEnum = nullptr;
    ICreateDevEnum* pDevEnum = nullptr;
    HRESULT hr = ::CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));
    if (SUCCEEDED(hr))
    {
        hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
        if (hr == S_FALSE)
        {
            hr = VFW_E_NOT_FOUND;
        }
        pDevEnum->Release();
    }
    return hr;
}
#endif

#ifdef _MSC_VER
// 从DirectShow设备枚举器读取设备显示名称。
void CSystem::displayDeviceInformation(IEnumMoniker* pEnum, std::vector<std::string>& list)
{
    if (pEnum == nullptr)
    {
        return;
    }

    IMoniker* pMoniker = nullptr;
    while (pEnum->Next(1, &pMoniker, nullptr) == S_OK)
    {
        IPropertyBag* pPropBag = nullptr;
        HRESULT hr = pMoniker->BindToStorage(nullptr, nullptr, IID_PPV_ARGS(&pPropBag));
        if (FAILED(hr))
        {
            pMoniker->Release();
            continue;
        }

        VARIANT var;
        ::VariantInit(&var);
        hr = pPropBag->Read(L"Description", &var, nullptr);
        if (FAILED(hr))
        {
            hr = pPropBag->Read(L"FriendlyName", &var, nullptr);
        }
        if (SUCCEEDED(hr))
        {
            if (var.vt == VT_BSTR && var.bstrVal != nullptr)
            {
                char* nameBuffer = _com_util::ConvertBSTRToString(var.bstrVal);
                if (nameBuffer != nullptr)
                {
                    list.push_back(nameBuffer);
                    delete[] nameBuffer;
                }
            }
            ::VariantClear(&var);
        }
        hr = pPropBag->Read(L"WaveInID", &var, nullptr);
        if (SUCCEEDED(hr))
        {
            ::VariantClear(&var);
        }
        hr = pPropBag->Read(L"DevicePath", &var, nullptr);
        if (SUCCEEDED(hr))
        {
            ::VariantClear(&var);
        }

        pPropBag->Release();
        pMoniker->Release();
    }
}
#endif

#ifdef _MSC_VER
// 获取当前代码所在模块句柄。
HMODULE CSystem::getCurrentModule()
{
    HMODULE hModule = nullptr;
    ::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)getCurrentModule, &hModule);
    return hModule;
}
#endif

#ifdef _MSC_VER
BOOL CALLBACK CSystem::findWindowCallback(HWND window, LPARAM context)
{
    std::pair<uint32_t, HWND>* query = reinterpret_cast<std::pair<uint32_t, HWND>*>(context);
    DWORD processId = 0;
    ::GetWindowThreadProcessId(window, &processId);
    if (processId == query->first)
    {
        query->second = window;
        return FALSE;
    }
    return TRUE;
}
#endif

#ifdef _MSC_VER
BOOL CALLBACK CSystem::findVisibleWindowCallback(HWND window, LPARAM context)
{
    std::pair<uint32_t, HWND>* query = reinterpret_cast<std::pair<uint32_t, HWND>*>(context);
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (processId == query->first && IsWindowVisible(window))
    {
        query->second = window;
        return FALSE;
    }
    return TRUE;
}
#endif

#ifdef _WIN32
//安全的取得真实系统信息
VOID CSystem::getNativeSystemInfo(__out LPSYSTEM_INFO lpSystemInfo)
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

#if CSYSTEM_HAS_DESKTOP_API
int CSystem::getConsoleCharacter()
{
#ifdef _WIN32
    return ::_getch();
#elif defined(__unix__) || defined(__APPLE__)
    struct termios previous = {}, current = {};
    if (::tcgetattr(STDIN_FILENO, &previous) != 0)
    {
        return ::getchar();
    }
    current = previous;
    current.c_lflag &= ~(ICANON | ECHO);
    if (::tcsetattr(STDIN_FILENO, TCSANOW, &current) != 0)
    {
        return EOF;
    }
    const int value = ::getchar();
    ::tcsetattr(STDIN_FILENO, TCSANOW, &previous);
    return value;
#else
    return EOF;
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

#ifdef CSYSTEM_SHARED_IDENTITY
uint64_t CSystem::mobileSelfIdentity()
{
    try
    {
        static uint64_t s_identity = 0;
#ifdef _WIN32
        static INIT_ONCE s_once = INIT_ONCE_STATIC_INIT;
        if (!::InitOnceExecuteOnce(&s_once, initializeProcessIdentity, &s_identity, nullptr))
        {
            return 0;
        }
#else
        static std::once_flag s_once;
        /** 初始化本进程登记，失败抛出以允许call_once下一次重试
        */
        std::call_once(s_once, []()
        {
            uint64_t identity = 0;
            if (!secureRandomBytes(reinterpret_cast<uint8_t*>(&identity), sizeof(identity)) || identity == 0 ||
                !publishProcessIdentity(identity))
            {
                throw std::runtime_error("process identity publication unavailable");
            }
            s_identity = identity;
        });
#endif
        return s_identity;
    }
    catch (...)
    {
        return 0;
    }
}
#endif

#if defined(CSYSTEM_SHARED_IDENTITY) && defined(_WIN32)
BOOL CALLBACK CSystem::initializeProcessIdentity(PINIT_ONCE once, PVOID parameter, PVOID* context)
{
    (void)once;
    (void)context;
    try
    {
        uint64_t identity = 0;
        if (parameter == nullptr || !secureRandomBytes(reinterpret_cast<uint8_t*>(&identity), sizeof(identity)) ||
            identity == 0 || !publishProcessIdentity(identity))
        {
            return FALSE;
        }
        *static_cast<uint64_t*>(parameter) = identity;
        return TRUE;
    }
    catch (...)
    {
        return FALSE;
    }
}
#endif

#if defined(__linux__)
/** 从同一次proc stat快照确认整个线程组退出，不能只看主线程的僵尸状态。
@param [in] line /proc/PID/stat完整记录；comm允许空格及括号
@return 仅Z/X且字段20明确为1时true；缺字段或仍有其他线程时false
*/
bool CSystem::linuxProcessStatExited(const std::string& line)
{
    size_t cursor = line.rfind(')');
    if (cursor == std::string::npos)
    {
        return false;
    }
    ++cursor;
    for (int32_t field = 3; field <= 20; ++field)
    {
        cursor = line.find_first_not_of(' ', cursor);
        if (cursor == std::string::npos)
        {
            return false;
        }
        const size_t end = line.find(' ', cursor);
        if (end == std::string::npos)
        {
            return false;
        }
        if (field == 3 && (end != cursor + 1 || (line[cursor] != 'Z' && line[cursor] != 'X')))
        {
            return false;
        }
        // 线程组首线程pthread_exit后仍可能为Z，直到最后一个工作线程退出才只剩首线程。
        // 不把0、截短或异常字段当作死亡；无法确认时交给kill(pid, 0)的ESRCH兜底。
        if (field == 20)
        {
            return end == cursor + 1 && line[cursor] == '1';
        }
        cursor = end + 1;
    }
    return false;
}
#endif

#if defined(__unix__) || defined(__APPLE__)
// 获取当前动态库所在目录，入参为空时返回nullptr。
char* CSystem::getModuleCurrentPath(char* sCurPath)
{
    if (sCurPath == nullptr)
    {
        return nullptr;
    }
    Dl_info dl_info;
    memset(&dl_info, 0, sizeof(dl_info));
    if (dladdr((void*)getModuleCurrentPath, &dl_info) == 0 || dl_info.dli_fname == nullptr)
    {
        sCurPath[0] = '\0';
        return sCurPath;
    }
    std::string path(dl_info.dli_fname);
    std::string wdir = path.substr(0, path.find_last_of('/') + 1);
    const size_t maxPathLen = 1024;
    ::strncpy(sCurPath, wdir.c_str(), maxPathLen - 1);
    sCurPath[maxPathLen - 1] = '\0';
    return sCurPath;
}
#endif

bool CSystem::plainDirectory(const std::string& path)
{
#ifdef _WIN32
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0;
#elif defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
    struct stat value;
    return lstat(path.c_str(), &value) == 0 && S_ISDIR(value.st_mode);
#else
    return false;
#endif
}

bool CSystem::plainFile(const std::string& path)
{
#ifdef _WIN32
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) == 0;
#elif defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
    struct stat value;
    return lstat(path.c_str(), &value) == 0 && S_ISREG(value.st_mode);
#else
    return false;
#endif
}

bool CSystem::equalFiles(const std::string& first, const std::string& second)
{
    std::ifstream a(first.c_str(), std::ios::binary), b(second.c_str(), std::ios::binary);
    if (!a.good() || !b.good())
    {
        return false;
    }
    char left[65536], right[65536];
    do
    {
        a.read(left, sizeof(left));
        b.read(right, sizeof(right));
        if (a.gcount() != b.gcount() || std::memcmp(left, right, static_cast<size_t>(a.gcount())) != 0)
        {
            return false;
        }
    } while (a.gcount() != 0);
    return a.eof() && b.eof() && !a.bad() && !b.bad();
}

bool CSystem::copyFileMetadata(const std::string& source, const std::string& destination)
{
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA attributes = {};
    if (!GetFileAttributesExA(source.c_str(), GetFileExInfoStandard, &attributes))
    {
        return false;
    }
    HANDLE handle = CreateFileA(destination.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    const bool result = SetFileTime(handle, &attributes.ftCreationTime, &attributes.ftLastAccessTime, &attributes.ftLastWriteTime) != FALSE;
    CloseHandle(handle);
    return result;
#elif defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
    struct stat attributes;
    if (stat(source.c_str(), &attributes) != 0)
    {
        return false;
    }
    struct utimbuf times;
    times.actime = attributes.st_atime;
    times.modtime = attributes.st_mtime;
    return utime(destination.c_str(), &times) == 0;
#else
    return false;
#endif
}

bool CSystem::migrateFile(const std::string& source, const std::string& destination, std::string& errorText)
{
    if (!plainFile(source) || (CSystem::DirOrFileExist(destination) && !plainFile(destination)))
    {
        errorText = "Migration encountered a linked or non-file entry";
        return false;
    }
    if (CSystem::DirOrFileExist(destination))
    {
        if (!equalFiles(source, destination))
        {
            errorText = "Migration destination has different content: " + destination;
            return false;
        }
        if (!copyFileMetadata(source, destination))
        {
            errorText = "Migration cannot preserve file times: " + destination;
            return false;
        }
    }
    else
    {
        const std::string temporary = destination + ".migration_" + CSystem::uuid(0);
        if (CSystem::DirOrFileExist(temporary))
        {
            errorText = "Migration temporary path already exists";
            return false;
        }
        bool copied = false;
        {
            std::ifstream input(source.c_str(), std::ios::binary);
            std::ofstream output(temporary.c_str(), std::ios::binary | std::ios::trunc);
            char buffer[65536];
            while (input.good() && output.good())
            {
                input.read(buffer, sizeof(buffer));
                output.write(buffer, input.gcount());
            }
            output.flush();
            copied = input.eof() && !input.bad() && output.good();
            output.close();
            copied = copied && !output.fail();
        }
        bool committed = false;
        if (copied && equalFiles(source, temporary) && copyFileMetadata(source, temporary))
        {
#ifdef _WIN32
            committed = MoveFileExA(temporary.c_str(), destination.c_str(), MOVEFILE_WRITE_THROUGH) != FALSE;
#elif defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
            committed = link(temporary.c_str(), destination.c_str()) == 0;
#endif
        }
        CSystem::deleteFile(temporary.c_str());
        if (!committed)
        {
            errorText = "Migration copy or verified commit failed: " + destination;
            return false;
        }
    }
    if (!CSystem::deleteFile(source.c_str()))
    {
        errorText = "Verified destination saved but source could not be removed: " + source;
        return false;
    }
    return true;
}

bool CSystem::migrateTree(const std::string& source, const std::string& destination, std::string& errorText)
{
    if (!CSystem::createDirectories(destination))
    {
        errorText = "Cannot create plain migration destination";
        return false;
    }
    std::vector<std::string> names;
#ifdef _WIN32
    WIN32_FIND_DATAA info;
    HANDLE search = FindFirstFileA(CSystem::joinPath(source, "*").c_str(), &info);
    if (search == INVALID_HANDLE_VALUE)
    {
        errorText = "Cannot enumerate migration source";
        return false;
    }
    do
    {
        const std::string name(info.cFileName);
        if (name != "." && name != "..")
        {
            names.push_back(name);
        }
    } while (FindNextFileA(search, &info));
    const DWORD enumerationError = GetLastError();
    FindClose(search);
    if (enumerationError != ERROR_NO_MORE_FILES)
    {
        errorText = "Migration enumeration failed";
        return false;
    }
#elif defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
    DIR* directory = opendir(source.c_str());
    if (directory == nullptr)
    {
        errorText = "Cannot enumerate migration source";
        return false;
    }
    struct dirent* entry;
    while ((entry = readdir(directory)) != nullptr)
    {
        const std::string name(entry->d_name);
        if (name != "." && name != "..")
        {
            names.push_back(name);
        }
    }
    closedir(directory);
#else
    errorText = "Directory migration is unsupported on this platform";
    return false;
#endif
    for (size_t i = 0; i < names.size(); ++i)
    {
#ifdef _WIN32
        const std::string from = joinPath(source, names[i]), to = joinPath(destination, names[i]);
#else
        // POSIX 的反斜杠可能是目录名称的一部分，子路径必须用斜杠连接。
        const std::string from = source + "/" + names[i], to = destination + "/" + names[i];
#endif
        if (!(plainDirectory(from) ? migrateTree(from, to, errorText) : migrateFile(from, to, errorText)))
        {
            return false;
        }
    }
#ifdef _WIN32
    const bool removed = RemoveDirectoryA(source.c_str()) != FALSE;
#elif defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
    const bool removed = rmdir(source.c_str()) == 0;
#else
    const bool removed = false;
#endif
    if (!removed)
    {
        errorText = "Cannot remove migrated empty source directory";
    }
    return removed;
}

/** 保存文件操作错误，成功由各方法清零
@param [out] error 可空的系统错误输出
@param [in] value Win32错误或errno
@return false
*/
bool CSystem::systemFileFailure(uint32_t* error, uint32_t value)
{
    if (error)
    {
        *error = value;
    }
    return false;
}

/** 检查原生句柄的数值范围，避免POSIX宽整数被截断为另一个fd
@param [in] handle 原生句柄
@return 是否有效数值
*/
bool CSystem::isSystemFileHandle(intptr_t handle)
{
#ifdef _WIN32
    return handle != 0 && handle != -1;
#else
    return handle >= 0 && handle <= INT_MAX;
#endif
}

/** 只接纳可寻址普通文件
@param [in] handle 已打开句柄
@param [out] error 可空系统错误
@return 是否适用
*/
bool CSystem::validateSystemFileHandle(intptr_t handle, uint32_t* error)
{
#ifdef _WIN32
    BY_HANDLE_FILE_INFORMATION information = {};
    if (!GetFileInformationByHandle(reinterpret_cast<HANDLE>(handle), &information))
    {
        return systemFileFailure(error, GetLastError());
    }
    if ((information.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0 ||
        GetFileType(reinterpret_cast<HANDLE>(handle)) != FILE_TYPE_DISK)
    {
        return systemFileFailure(error, ERROR_INVALID_DATA);
    }
#else
    struct stat information = {};
    if (fstat(static_cast<int>(handle), &information) != 0)
    {
        return systemFileFailure(error, errno);
    }
    if (!S_ISREG(information.st_mode) || information.st_size < 0)
    {
        return systemFileFailure(error, EINVAL);
    }
#endif
    return true;
}

#if defined(_WIN32)
uint32_t CSystem::startOwnedProcess(const std::string& executable, const std::vector<std::string>& arguments,
    const std::string& output, uint32_t& pid, uint64_t& monitor)
{
    pid = 0;
    monitor = 0;
    if (executable.empty() || arguments.size() > 64 || executable.find('\0') != std::string::npos)
    {
        return 87;
    }
#ifdef _WIN32
    const std::wstring path = utf8PathToWide(executable);
    if (path.size() > 32750)
    {
        return ERROR_INVALID_PARAMETER;
    }
    if (path.empty() || path.find(L'"') != std::wstring::npos ||
        !((path.size() > 2 && path[1] == L':' && (path[2] == L'\\' || path[2] == L'/')) ||
            (path.size() > 2 && path[0] == L'\\' && path[1] == L'\\'))) { return ERROR_INVALID_PARAMETER; }
    std::wstring command = L"\"" + path + L"\"";
    for (const std::string& argument : arguments)
    {
        if (argument.find('\0') != std::string::npos || argument.size() > 32760)
        {
            return ERROR_INVALID_PARAMETER;
        }
        const std::wstring wide = utf8PathToWide(argument);
        if (!argument.empty() && wide.empty())
        {
            return ERROR_INVALID_PARAMETER;
        }
        command += L" \"";
        size_t slashes = 0;
        for (wchar_t byte : wide)
        {
            if (byte == L'\\')
            {
                ++slashes;
                continue;
            }
            command.append(slashes * (byte == L'"' ? 2 : 1), L'\\');
            slashes = 0;
            if (byte == L'"')
            {
                command += L'\\';
            }
            command += byte;
        }
        command.append(slashes * 2, L'\\');
        command += L'"';
        if (command.size() > 32760)
        {
            return ERROR_INVALID_PARAMETER;
        }
    }
    std::vector<wchar_t> buffer(command.begin(), command.end());
    buffer.push_back(L'\0');
    const std::wstring directory = utf8PathToWide(GetName(executable, 4));
    STARTUPINFOEXW startup = {};
    startup.StartupInfo.cb = output.empty() ? sizeof(STARTUPINFOW) : sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startup.StartupInfo.wShowWindow = SW_HIDE;
    HANDLE outputFile = INVALID_HANDLE_VALUE;
    HANDLE inputFile = INVALID_HANDLE_VALUE;
    HANDLE inherited[2] = {};
    std::vector<uint8_t> attributes;
    DWORD error = ERROR_SUCCESS;
    bool attributeReady = false;
    if (!output.empty())
    {
        const std::wstring destination = utf8PathToWide(output);
        if (destination.empty() || destination.size() > 32750 || output.find('\0') != std::string::npos ||
            !((destination.size() > 2 && destination[1] == L':' && (destination[2] == L'\\' || destination[2] == L'/')) ||
                (destination.size() > 2 && destination[0] == L'\\' && destination[1] == L'\\')))
        {
            return ERROR_INVALID_PARAMETER;
        }
        SECURITY_ATTRIBUTES security = {};
        security.nLength = sizeof(security);
        security.bInheritHandle = TRUE;
        outputFile = CreateFileW(destination.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &security, CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (outputFile == INVALID_HANDLE_VALUE)
        {
            return GetLastError();
        }
        inputFile = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (inputFile == INVALID_HANDLE_VALUE)
        {
            error = GetLastError();
        }
        SIZE_T required = 0;
        if (error == ERROR_SUCCESS)
        {
            InitializeProcThreadAttributeList(nullptr, 1, 0, &required);
            try
            {
                attributes.resize(required);
            }
            catch (const std::bad_alloc&)
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        if (error == ERROR_SUCCESS)
        {
            startup.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
            attributeReady = InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &required) != FALSE;
            if (!attributeReady)
            {
                error = GetLastError();
            }
        }
        if (error == ERROR_SUCCESS)
        {
            inherited[0] = outputFile;
            inherited[1] = inputFile;
            if (!UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                inherited, sizeof(inherited), nullptr, nullptr)) { error = GetLastError(); }
            startup.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
            startup.StartupInfo.hStdOutput = outputFile;
            startup.StartupInfo.hStdError = outputFile;
            startup.StartupInfo.hStdInput = inputFile;
        }
    }
    PROCESS_INFORMATION process = {};
    if (error == ERROR_SUCCESS && !CreateProcessW(path.c_str(), buffer.data(), nullptr, nullptr, !output.empty(),
        CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP | (output.empty() ? 0 : EXTENDED_STARTUPINFO_PRESENT),
        nullptr, directory.c_str(), &startup.StartupInfo, &process)) { error = GetLastError(); }
    if (attributeReady)
    {
        DeleteProcThreadAttributeList(startup.lpAttributeList);
    }
    if (outputFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(outputFile);
    }
    if (inputFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(inputFile);
    }
    if (error != ERROR_SUCCESS)
    {
        return error;
    }
    CloseHandle(process.hThread);
    pid = process.dwProcessId;
    monitor = reinterpret_cast<uintptr_t>(process.hProcess);
    return ERROR_SUCCESS;
#else
    (void)output;
    return 50;
#endif
}
#endif // CSYSTEM_PLATFORM_CAPABILITY

std::wstring CSystem::utf8PathToWide(const std::string& text)
{
    try
    {
#ifdef _WIN32
        if (text.empty() || text.size() > static_cast<size_t>(INT32_MAX))
        {
            return L"";
        }
        const int32_t count = static_cast<int32_t>(text.size());
        const int32_t required = ::MultiByteToWideChar(CP_UTF8, 0, text.data(), count, nullptr, 0);
        if (required <= 0)
        {
            return L"";
        }
        std::wstring result(static_cast<size_t>(required), L'\0');
        if (::MultiByteToWideChar(CP_UTF8, 0, text.data(), count, &result[0], required) != required)
        {
            return L"";
        }
        return result;
#elif defined(__unix__) || defined(__APPLE__)
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(text);
#else
        return L"";
#endif
    }
    catch (...)
    {
        return L"";
    }
}

#ifdef CSYSTEM_SHARED_IDENTITY
std::string& CSystem::processDirectory()
{
    static std::string s_directory;
    return s_directory;
}
#endif

#ifdef CSYSTEM_SHARED_IDENTITY
std::atomic<bool>& CSystem::processDirectorySealed()
{
    static std::atomic<bool> s_sealed(false);
    return s_sealed;
}
#endif

#ifdef CSYSTEM_SHARED_IDENTITY
#ifndef _WIN32
int CSystem::openProcessDirectory(const std::string& directory)
{
    if (directory.empty() || directory[0] != '/' || directory.find('\0') != std::string::npos)
    {
        return -1;
    }
    const int descriptor = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (descriptor < 0)
    {
        return -1;
    }
    struct stat information = {};
    if (::fstat(descriptor, &information) != 0 || !S_ISDIR(information.st_mode) || information.st_uid != ::geteuid())
    {
        ::close(descriptor);
        return -1;
    }
    return descriptor;
}
#endif
#endif

#ifdef CSYSTEM_SHARED_IDENTITY
bool CSystem::publishProcessIdentity(uint64_t identity)
{
    if (identity == 0 || processDirectory().empty())
    {
        return false;
    }
    // 保留原来的m_P2I<PID>名称和8字节格式；只替换完整文件，读者不会见到半写入记录。
    const std::string name = "m_P2I" + std::to_string(currentProcessPid());
    const std::string temporary = name + "." + std::to_string(identity) + ".tmp";
    processDirectorySealed().store(true);
#ifdef _WIN32
    const std::wstring path = utf8PathToWide(joinPath(processDirectory(), name));
    const std::wstring staging = utf8PathToWide(joinPath(processDirectory(), temporary));
    if (path.empty() || staging.empty())
    {
        return false;
    }
    HANDLE file = ::CreateFileW(staging.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    DWORD written = 0;
    bool success = ::WriteFile(file, &identity, sizeof(identity), &written, nullptr) != FALSE && written == sizeof(identity);
    if (!::CloseHandle(file))
    {
        success = false;
    }
    if (success)
    {
        success = ::MoveFileExW(staging.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING) != FALSE;
    }
    if (!success)
    {
        ::DeleteFileW(staging.c_str());
    }
    return success;
#else
    const int directory = openProcessDirectory(processDirectory());
    if (directory < 0)
    {
        return false;
    }
    const int descriptor = ::openat(directory, temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (descriptor < 0)
    {
        ::close(directory);
        return false;
    }
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&identity);
    size_t offset = 0;
    while (offset < sizeof(identity))
    {
        const ssize_t written = ::write(descriptor, bytes + offset, sizeof(identity) - offset);
        if (written < 0 && errno == EINTR)
        {
            continue;
        }
        if (written <= 0)
        {
            break;
        }
        offset += static_cast<size_t>(written);
    }
    bool success = offset == sizeof(identity);
    if (::close(descriptor) != 0)
    {
        success = false;
    }
    if (success)
    {
        success = ::renameat(directory, temporary.c_str(), directory, name.c_str()) == 0;
    }
    if (!success)
    {
        ::unlinkat(directory, temporary.c_str(), 0);
    }
    ::close(directory);
    return success;
#endif
}
#endif

#ifdef CSYSTEM_SHARED_IDENTITY
bool CSystem::readProcessIdentity(uint32_t processId, uint64_t& identity)
{
    identity = 0;
    const std::string name = "m_P2I" + std::to_string(processId);
#ifdef _WIN32
    const std::wstring path = utf8PathToWide(joinPath(processDirectory(), name));
    if (path.empty())
    {
        return false;
    }
    HANDLE file = ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    BY_HANDLE_FILE_INFORMATION information = {};
    DWORD received = 0;
    const bool success = ::GetFileInformationByHandle(file, &information) != FALSE &&
        (information.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) == 0 &&
        information.nFileSizeHigh == 0 && information.nFileSizeLow == sizeof(identity) &&
        ::ReadFile(file, &identity, sizeof(identity), &received, nullptr) != FALSE && received == sizeof(identity);
    ::CloseHandle(file);
#else
    const int directory = openProcessDirectory(processDirectory());
    if (directory < 0)
    {
        return false;
    }
    const int descriptor = ::openat(directory, name.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    ::close(directory);
    if (descriptor < 0)
    {
        return false;
    }
    struct stat information = {};
    const bool valid = ::fstat(descriptor, &information) == 0 && S_ISREG(information.st_mode) &&
        information.st_uid == ::geteuid() && information.st_size == sizeof(identity);
    size_t offset = 0;
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&identity);
    while (valid && offset < sizeof(identity))
    {
        const ssize_t received = ::read(descriptor, bytes + offset, sizeof(identity) - offset);
        if (received < 0 && errno == EINTR)
        {
            continue;
        }
        if (received <= 0)
        {
            break;
        }
        offset += static_cast<size_t>(received);
    }
    const bool success = valid && offset == sizeof(identity);
    ::close(descriptor);
#endif
    if (!success)
    {
        identity = 0;
    }
    return success && identity != 0;
}
#endif
#endif // CSYSTEM_NATIVE_API

CSystemThreadPriorityResult CSystem::setThreadPriority(std::thread& thread, int32_t relative, int32_t& nativeError)
{
    nativeError = 0;
    if (!thread.joinable() || relative < -2 || relative > 2)
    {
        return CSystemThreadPriorityInvalid;
    }
    try
    {
#ifdef _WIN32
        const int levels[] = {THREAD_PRIORITY_IDLE, THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_NORMAL,
            THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST};
        if (!SetThreadPriority(thread.native_handle(), levels[relative + 2]))
        {
            nativeError = static_cast<int32_t>(GetLastError());
            return CSystemThreadPriorityFailed;
        }
#elif defined(__EMSCRIPTEN__)
        return CSystemThreadPriorityUnsupported;
#else
        if (relative < 0)
        {
            return CSystemThreadPriorityUnsupported;
        }
        const int policy = relative == 0 ? SCHED_OTHER : SCHED_RR;
        sched_param parameter = {};
        if (relative > 0)
        {
            const int minimum = sched_get_priority_min(policy);
            if (minimum < 0)
            {
                nativeError = errno;
                return CSystemThreadPriorityFailed;
            }
            const int maximum = sched_get_priority_max(policy);
            if (maximum < minimum)
            {
                nativeError = errno;
                return CSystemThreadPriorityFailed;
            }
            parameter.sched_priority = relative == 1 ? minimum : minimum + (maximum - minimum) / 2;
        }
        nativeError = pthread_setschedparam(thread.native_handle(), policy, &parameter);
        if (nativeError != 0)
        {
            return CSystemThreadPriorityFailed;
        }
#endif
        return CSystemThreadPriorityApplied;
    }
    catch (const std::system_error& error)
    {
        nativeError = error.code().value();
        return CSystemThreadPriorityFailed;
    }
    catch (...)
    {
        return CSystemThreadPriorityFailed;
    }
}

// 使用示例：在本翻译单元末尾启用以下 main，单独构建控制台程序
// #include <cstdio>
// #include <ctime>
//
// int main(int argc, char** argv)
// {
//     std::vector<std::string> args;
//     if (!CSystem::commandArguments(argc, argv, args))
//     {
//         return 1;
//     }
//     const auto begin = CSystem::GetHighTickCount();
//     const std::string leaf = CSystem::ensureFileExtension("session", ".txt");
//     const std::string example = CSystem::joinPath("logs", leaf);
//     std::printf("%s %s %s\n", CSystem::timetToStr(std::time(nullptr), false).c_str(),
//         CSystem::GetName(example, 1).c_str(), CSystem::GetName(example, 2).c_str());
//     int** counts = CSystem::CreateDyadicArray<int>(2, 3);
//     if (counts == nullptr)
//     {
//         return 2;
//     }
//     counts[0][0] = argc;
//     CSystem::DestroyDyadicArray(counts, 2);
//     bool valid = true;
// #if CSYSTEM_HAS_NATIVE_API
//     const uint32_t pid = CSystem::currentProcessPid();
//     const std::string unique = CSystem::uuid(0);
//     std::printf("user directory=%s\n", CSystem::GetSystemTempPath().c_str());
//     const std::string directory = CSystem::joinPath(".", "csdemo_" + unique);
//     if (unique.empty() || CSystem::DirOrFileExist(directory) || !CSystem::CreateDir(directory))
//     {
//         return 3;
//     }
//     const std::string first = CSystem::joinPath(directory, "draft.txt");
//     const std::string target = CSystem::joinPath(directory, leaf);
//     const uint8_t text[] = {'l', 'o', 'g'};
//     intptr_t writer = CSystem::openFileHandle(first.c_str(), 2);
//     intptr_t duplicate = CSystem::duplicateFileHandle(writer);
//     uint64_t size = 0;
//     uint8_t readback[3] = {};
//     uint32_t received = 0;
//     valid = writer != -1 && duplicate != -1 && CSystem::writeFileHandle(writer, 0, text, 3) &&
//         CSystem::flushFileHandle(writer) && CSystem::fileHandleSize(duplicate, &size) && size == 3 &&
//         CSystem::readFileHandle(duplicate, 0, readback, 3, &received) && received == 3 &&
//         CSystem::resizeFileHandle(writer, 3);
//     CSystem::closeFileHandle(&duplicate);
//     CSystem::closeFileHandle(&writer);
//     if (valid)
//     {
//         valid = CSystem::publishFileUtf8(first.c_str(), target.c_str()) && CSystem::plainFileUtf8(target);
//     }
//     uint64_t stamp = 0;
//     if (valid)
//     {
//         valid = CSystem::fileStampUtf8(target, &size, &stamp) && CSystem::readFile(target) == "log" &&
//             CSystem::isPathWithin(target, directory) && CSystem::findFilePath(directory, 1, leaf).size() == 1;
//     }
//     std::printf("directory=%s utf8=%s\n", CSystem::canonicalPath(directory).c_str(), CSystem::canonicalPathUtf8(directory).c_str());
//     const std::string second = CSystem::joinPath(directory, "copy.txt");
//     CSystem::saveFile("demo", second);
//     const std::string renamed = CSystem::joinPath(directory, "renamed.txt");
//     if (CSystem::rename(second, renamed))
//     {
//         CSystem::deleteFile(renamed.c_str());
//     }
//     CSystem::deleteFile(first.c_str());
//     CSystem::deleteFile(second.c_str());
//     CSystem::deleteFile(target.c_str());
//     valid = CSystem::DestroyDir(directory) && valid;
//     uint8_t nonce[16] = {};
//     valid = CSystem::secureRandomBytes(nonce, 8) && CSystem::secureRandomBytesCallback(nullptr, nonce + 8, 8) == 0 && valid;
//     std::printf("pid=%u thread=%u bits=%d cores=%d tick=%u\n", pid, CSystem::SystemThreadId(), CSystem::GetSystemBits(),
//         CSystem::GetCPUCoreCount(), CSystem::GetTickCount());
//     std::printf("host=%s user=%s exe=%s directory=%s file=%s utf8=%s\n",
//         CSystem::getComputerName().c_str(), CSystem::GetSysUserName().c_str(), CSystem::GetCurrentExeName().c_str(),
//         CSystem::GetCurrentExePath().c_str(), CSystem::GetCurrentExeFullName().c_str(), CSystem::currentExecutablePathUtf8().c_str());
//     uint64_t identity = 0;
//     CSystem::processIdentity(pid, identity);
//     CSystem::Sleep(1);
// #ifdef _WIN32
//     uint64_t monitor = CSystem::openProcessMonitor(pid);
//     uint64_t privateBytes = 0, resident = 0, cpu = 0;
//     uint32_t handles = 0, threads = 0;
//     if (monitor != 0)
//     {
//         int32_t exitCode = 0;
//         CSystem::queryProcessResources(monitor, privateBytes, resident, cpu, handles, threads);
//         CSystem::pollProcessMonitor(monitor, exitCode);
//         CSystem::closeProcessMonitor(monitor);
//     }
//     RECT screen = CSystem::rectValid(CSystem::GetWindowResolution());
//     const RECT taskbar = CSystem::GetTaskbarRect();
//     const POINT corner = CSystem::taskbarRightBottomPoint();
//     const POINT center = CSystem::screenCenterPoint();
//     std::printf("screen=%ldx%ld center=%ld,%ld taskbar=%ld,%ld corner=%ld,%ld visible=%d\n",
//         screen.right, screen.bottom, center.x, center.y, taskbar.left, taskbar.top, corner.x, corner.y, CSystem::GetVisibleHeight());
//     const std::wstring executable = CSystem::processNameW(pid);
//     std::printf("processes=%u modules=%u first=%u version=%d\n",
//         static_cast<unsigned>(CSystem::processPid(executable).size()), static_cast<unsigned>(CSystem::processMountDll(pid).size()),
//         CSystem::processFirstPid(executable), CSystem::GetSystemVersionNum());
//     std::printf("module=%s console=%p window=%p visible=%p\n", CSystem::GetCurrentDllPath().c_str(),
//         static_cast<void*>(CSystem::GetConsoleHwnd()), static_cast<void*>(CSystem::GetHwndByProcessId(pid)),
//         static_cast<void*>(CSystem::findVisibleWindowByProcessId(pid)));
//     std::printf("session=%s args=%u memory=%llu\n", CSystem::GetEnvironment("SESSIONNAME").c_str(),
//         static_cast<unsigned>(CSystem::exeParam(argc, argv).size()), static_cast<unsigned long long>(CSystem::currentMemory()));
// #endif
// #endif
//     const auto end = CSystem::GetHighTickCount();
//     std::printf("elapsed ms=%d us=%d ns=%lld (now %d/%d/%lld)\n", CSystem::GetMilliRunTime(end, begin),
//         CSystem::GetMicroRunTime(end, begin), static_cast<long long>(CSystem::GetNanoRunTime(end, begin)),
//         CSystem::GetHighTickCountMilliRunTime(begin), CSystem::GetHighTickCountMicroRunTime(begin),
//         static_cast<long long>(CSystem::GetHighTickCountNanoRunTime(begin)));
//     return valid ? 0 : 4;
// }