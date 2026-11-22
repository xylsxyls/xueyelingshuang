#include "mdump.h"
#ifdef _WIN32
#include <windows.h>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4091)
#endif
#include <dbghelp.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <cstdio>
#include <cwchar>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>

// 安装完成后路径和函数地址一直有效，DbgHelp 与过滤器均驻留到进程退出
static wchar_t s_prefix[MAX_PATH] = {};
static decltype(&MiniDumpWriteDump) s_writeDump = nullptr;
static decltype(&StackWalk64) s_walk = nullptr;
static decltype(&SymInitialize) s_initSymbols = nullptr;
static decltype(&SymCleanup) s_cleanupSymbols = nullptr;
static PFUNCTION_TABLE_ACCESS_ROUTINE64 s_functions = nullptr;
static PGET_MODULE_BASE_ROUTINE64 s_moduleBase = nullptr;
// DbgHelp 不支持在多个故障线程中同时调用
static LONG s_writing = 0;

/** 从故障线程的上下文记录地址，转储另含完整线程上下文供调试器解析
@param [in] log 已打开的本次日志
@param [in] fault 系统传入的异常记录及线程上下文，仅在回调期间借用
*/
static void WriteExceptionLog(FILE* log, EXCEPTION_POINTERS* fault)
{
    CONTEXT context = *fault->ContextRecord;
    const EXCEPTION_RECORD& record = *fault->ExceptionRecord;
    MEMORYSTATUSEX memory = {};
    memory.dwLength = sizeof(memory);
    if (GlobalMemoryStatusEx(&memory))
    {
        std::fprintf(log, "Memory: [Available]%I64uM / [Total]%I64uM\n",
            memory.ullAvailPhys >> 20, memory.ullTotalPhys >> 20);
    }
    MEMORY_BASIC_INFORMATION region = {};
    char module[MAX_PATH * 3] = {};
    if (VirtualQuery(record.ExceptionAddress, &region, sizeof(region)) && region.Type == MEM_IMAGE)
    {
        wchar_t path[MAX_PATH];
        const DWORD size = GetModuleFileNameW(static_cast<HMODULE>(region.AllocationBase), path, MAX_PATH);
        if (size && size < MAX_PATH)
        {
            WideCharToMultiByte(CP_UTF8, 0, path, -1, module, sizeof(module), nullptr, nullptr);
        }
    }
    std::fprintf(log, "Error Module: %s\nCode: 0x%08lx\nFlag: 0x%08lx\nAddr: %p\n",
        module, record.ExceptionCode, record.ExceptionFlags, record.ExceptionAddress);
    for (DWORD i = 0; i < record.NumberParameters && i < EXCEPTION_MAXIMUM_PARAMETERS; ++i)
    {
        std::fprintf(log, "Parameter %lu: %p\n", i, reinterpret_cast<void*>(record.ExceptionInformation[i]));
    }
    STACKFRAME64 frame = {};
#ifdef _WIN64
    const DWORD machine = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset = context.Rip;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrFrame.Offset = context.Rbp;
    std::fprintf(log, "RAX=%I64x RBX=%I64x RCX=%I64x RDX=%I64x RSI=%I64x RDI=%I64x\n",
        context.Rax, context.Rbx, context.Rcx, context.Rdx, context.Rsi, context.Rdi);
#else
    const DWORD machine = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset = context.Eip;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrFrame.Offset = context.Ebp;
    std::fprintf(log, "EAX=%lx EBX=%lx ECX=%lx EDX=%lx ESI=%lx EDI=%lx\n",
        context.Eax, context.Ebx, context.Ecx, context.Edx, context.Esi, context.Edi);
#endif
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    std::fprintf(log, "SP=%I64x FP=%I64x EFLAGS=%lx\nCall Stack:\n#0 0x%I64x\n",
        frame.AddrStack.Offset, frame.AddrFrame.Offset, context.EFlags, frame.AddrPC.Offset);
    HANDLE process = nullptr;
    if (!s_walk || !s_functions || !s_moduleBase || !s_initSymbols || !s_cleanupSymbols ||
        !DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
            &process, 0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        return;
    }
    if (s_initSymbols(process, nullptr, TRUE))
    {
        for (unsigned i = 1; i < 128; ++i)
        {
            if (!s_walk(machine, process, GetCurrentThread(), &frame, &context, nullptr,
                s_functions, s_moduleBase, nullptr) || !frame.AddrPC.Offset)
            {
                break;
            }
            std::fprintf(log, "#%u 0x%I64x\n", i, frame.AddrPC.Offset);
        }
        s_cleanupSymbols(process);
    }
    CloseHandle(process);
}

/** 写出 minidump 和同名日志，将异常的最终处置交还系统
@param [in] fault 系统异常信息，缺少上下文时交给系统继续处理
@return 转储成功为 EXCEPTION_EXECUTE_HANDLER，其余情况为 EXCEPTION_CONTINUE_SEARCH
*/
static LONG WINAPI WriteDump(EXCEPTION_POINTERS* fault)
{
    if (!fault || !fault->ExceptionRecord || !fault->ContextRecord ||
        InterlockedCompareExchange(&s_writing, 1, 0) != 0)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    SYSTEMTIME now;
    GetLocalTime(&now);
    wchar_t path[MAX_PATH];
    const int length = _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%ls_%04u%02u%02u_%02u%02u%02u_%03u_%lu_%lu.dmp",
        s_prefix, now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
        now.wMilliseconds, GetCurrentProcessId(), GetCurrentThreadId());
    BOOL saved = FALSE;
    if (length > 4)
    {
        HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE)
        {
            MINIDUMP_EXCEPTION_INFORMATION exception = {};
            exception.ThreadId = GetCurrentThreadId();
            exception.ExceptionPointers = fault;
            saved = s_writeDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal,
                &exception, nullptr, nullptr);
            CloseHandle(file);
            if (!saved)
            {
                DeleteFileW(path);
            }
        }
        std::wmemcpy(path + length - 3, L"log", 4);
        int descriptor = -1;
        if (_wsopen_s(&descriptor, path, _O_WRONLY | _O_CREAT | _O_EXCL | _O_TEXT,
            _SH_DENYWR, _S_IREAD | _S_IWRITE) == 0)
        {
            FILE* log = _fdopen(descriptor, "w");
            if (log)
            {
                WriteExceptionLog(log, fault);
                std::fprintf(log, "MiniDumpWriteDump: %s\n", saved ? "OK" : "failed");
                std::fclose(log);
            }
            else
            {
                _close(descriptor);
            }
        }
    }
    InterlockedExchange(&s_writing, 0);
    return saved ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

bool InstallWindowsDump(const std::string& directory)
{
    wchar_t image[MAX_PATH];
    const DWORD imageSize = GetModuleFileNameW(nullptr, image, MAX_PATH);
    if (!imageSize || imageSize >= MAX_PATH)
    {
        return false;
    }
    const std::wstring exe(image, imageSize);
    const size_t slash = exe.find_last_of(L"/\\");
    if (slash == std::wstring::npos)
    {
        return false;
    }
    wchar_t absolute[MAX_PATH];
    if (directory.empty())
    {
        std::wmemcpy(absolute, image, slash + 1);
        absolute[slash + 1] = L'\0';
    }
    else
    {
        // 只转换调用者传入的窄字符串，当前目录和可执行文件名保留系统原始字符
        wchar_t input[MAX_PATH];
        if (!MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, directory.c_str(), -1, input, MAX_PATH))
        {
            return false;
        }
        const DWORD size = GetFullPathNameW(input, MAX_PATH, absolute, nullptr);
        if (!size || size >= MAX_PATH)
        {
            return false;
        }
    }
    const DWORD attributes = GetFileAttributesW(absolute);
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        return false;
    }
    std::wstring prefix(absolute);
    if (prefix.back() != L'\\' && prefix.back() != L'/')
    {
        prefix += L'\\';
    }
    const std::wstring name = exe.substr(slash + 1);
    prefix += name.substr(0, name.rfind(L'.'));
    // 最长后缀含日期、毫秒、两个十位进程/线程号、扩展名及结束符
    if (prefix.size() + sizeof("_99999999_999999_999_4294967295_4294967295.dmp") > MAX_PATH)
    {
        return false;
    }
    wchar_t system[MAX_PATH];
    const UINT count = GetSystemDirectoryW(system, MAX_PATH);
    if (!count || count >= MAX_PATH)
    {
        return false;
    }
    HMODULE helper = LoadLibraryW((std::wstring(system, count) + L"\\dbghelp.dll").c_str());
    if (!helper)
    {
        return false;
    }
    s_writeDump = reinterpret_cast<decltype(s_writeDump)>(GetProcAddress(helper, "MiniDumpWriteDump"));
    if (!s_writeDump)
    {
        FreeLibrary(helper);
        return false;
    }
    s_walk = reinterpret_cast<decltype(s_walk)>(GetProcAddress(helper, "StackWalk64"));
    s_functions = reinterpret_cast<decltype(s_functions)>(GetProcAddress(helper, "SymFunctionTableAccess64"));
    s_moduleBase = reinterpret_cast<decltype(s_moduleBase)>(GetProcAddress(helper, "SymGetModuleBase64"));
    s_initSymbols = reinterpret_cast<decltype(s_initSymbols)>(GetProcAddress(helper, "SymInitialize"));
    s_cleanupSymbols = reinterpret_cast<decltype(s_cleanupSymbols)>(GetProcAddress(helper, "SymCleanup"));
    std::wmemcpy(s_prefix, prefix.c_str(), prefix.size() + 1);
    SetUnhandledExceptionFilter(WriteDump);
    return true;
}
#endif