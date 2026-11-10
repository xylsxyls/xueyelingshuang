#ifdef CTTM_HANG_SAMPLER_MAIN
#include <Windows.h>
#include <TlHelp32.h>
#include <DbgHelp.h>
#include <cstdio>
#include <cstdlib>
#include <type_traits>

/** 采集一个由诊断脚本创建的同位宽测试进程的线程栈，并立即恢复线程
@param [in] argc 命令行参数数量，必须为2
@param [in] argv 参数1为测试子进程PID，不用于采集其他应用
@return 0表示枚举完成；非零表示参数或系统调用失败
*/
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    char* end = nullptr;
    const DWORD processId = std::strtoul(argv[1], &end, 10);
    if (processId == 0 || end == argv[1] || *end != '\0')
    {
        return 2;
    }
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (process == nullptr)
    {
        return 3;
    }
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(process, nullptr, TRUE))
    {
        CloseHandle(process);
        return 4;
    }
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    THREADENTRY32 entry = {};
    entry.dwSize = sizeof(entry);
    BOOL available = snapshot != INVALID_HANDLE_VALUE && Thread32First(snapshot, &entry);
    while (available)
    {
        if (entry.th32OwnerProcessID == processId)
        {
            HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION,
                FALSE, entry.th32ThreadID);
            if (thread != nullptr && SuspendThread(thread) != DWORD(-1))
            {
                CONTEXT context = {};
                context.ContextFlags = CONTEXT_FULL;
                if (GetThreadContext(thread, &context))
                {
                    STACKFRAME64 frame = {};
#ifdef _WIN64
                    const DWORD machine = IMAGE_FILE_MACHINE_AMD64;
                    frame.AddrPC.Offset = context.Rip;
                    frame.AddrFrame.Offset = context.Rbp;
                    frame.AddrStack.Offset = context.Rsp;
#else
                    const DWORD machine = IMAGE_FILE_MACHINE_I386;
                    frame.AddrPC.Offset = context.Eip;
                    frame.AddrFrame.Offset = context.Ebp;
                    frame.AddrStack.Offset = context.Esp;
                    unsigned __int64 value = 0;
                    SIZE_T read = 0;
                    const BOOL memory = ReadProcessMemory(process, reinterpret_cast<const void*>(context.Esi),
                        &value, sizeof(value), &read);
                    std::printf("REG esi=%lx edi=%lx eax=%lx edx=%lx ebx=%lx ecx=%lx flags=%lx memory=%d value=%I64x\n",
                        context.Esi, context.Edi, context.Eax, context.Edx, context.Ebx, context.Ecx, context.EFlags,
                        memory != FALSE, value);
#endif
                    frame.AddrPC.Mode = AddrModeFlat;
                    frame.AddrFrame.Mode = AddrModeFlat;
                    frame.AddrStack.Mode = AddrModeFlat;
                    std::printf("THREAD %lu\n", entry.th32ThreadID);
                    for (size_t depth = 0; depth < 32 && frame.AddrPC.Offset != 0; ++depth)
                    {
                        std::aligned_storage<sizeof(SYMBOL_INFO) + 1024, std::alignment_of<SYMBOL_INFO>::value>::type storage = {};
                        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(&storage);
                        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
                        symbol->MaxNameLen = 1024;
                        DWORD64 offset = 0;
                        const BOOL found = SymFromAddr(process, frame.AddrPC.Offset, &offset, symbol);
                        IMAGEHLP_LINE64 line = {};
                        line.SizeOfStruct = sizeof(line);
                        DWORD displacement = 0;
                        SymGetLineFromAddr64(process, frame.AddrPC.Offset, &displacement, &line);
                        std::printf("%I64x %s + %I64u line=%lu\n", frame.AddrPC.Offset,
                            found ? symbol->Name : "unknown", offset, line.LineNumber);
                        if (!StackWalk64(machine, process, thread, &frame, &context, nullptr,
                            SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
                        {
                            break;
                        }
                    }
                }
                ResumeThread(thread);
            }
            if (thread != nullptr)
            {
                CloseHandle(thread);
            }
        }
        available = Thread32Next(snapshot, &entry);
    }
    if (snapshot != INVALID_HANDLE_VALUE)
    {
        CloseHandle(snapshot);
    }
    SymCleanup(process);
    CloseHandle(process);
    return 0;
}
#endif