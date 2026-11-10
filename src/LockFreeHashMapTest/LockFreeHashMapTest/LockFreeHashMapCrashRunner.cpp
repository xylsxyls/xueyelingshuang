#include "LockFreeHashMapCrashRunner.h"
#include <stdint.h>
#include <stddef.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <vector>
#if defined(_WIN32)
#include <DbgHelp.h>
#include <TlHelp32.h>
#pragma comment(lib, "dbghelp.lib")

bool LockFreeHashMapCrashRunner::capture(HANDLE process, DWORD processId, DWORD threadId,
    const EXCEPTION_DEBUG_INFO* exception, const std::string& executable, const std::string& prefix)
{
    typedef std::unique_ptr<void, decltype(&CloseHandle)> Handle;
    Handle thread(OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, threadId), &CloseHandle);
    CONTEXT context = {};
    context.ContextFlags = CONTEXT_FULL;
    if (!thread || !GetThreadContext(thread.get(), &context))
    {
        return false;
    }
    std::ofstream output((prefix + "-stack.txt").c_str());
    if (!output)
    {
        return false;
    }
    if (exception != nullptr)
    {
        output << "exception=0x" << std::hex << exception->ExceptionRecord.ExceptionCode
            << " address=" << exception->ExceptionRecord.ExceptionAddress << " thread=" << std::dec << threadId << '\n';
        for (DWORD i = 0; i < exception->ExceptionRecord.NumberParameters; ++i)
        {
            output << "parameter[" << i << "]=0x" << std::hex << exception->ExceptionRecord.ExceptionInformation[i] << '\n';
        }
    }
    else
    {
        output << "reason=timeout thread=" << std::dec << threadId << '\n';
    }
#if defined(_M_X64)
    output << "rip=" << std::hex << context.Rip << " rsp=" << context.Rsp << " rbp=" << context.Rbp
        << " rax=" << context.Rax << " rbx=" << context.Rbx << " rcx=" << context.Rcx << " rdx=" << context.Rdx
        << " rsi=" << context.Rsi << " rdi=" << context.Rdi << " r8=" << context.R8 << " r9=" << context.R9
        << " r10=" << context.R10 << " r11=" << context.R11 << " r12=" << context.R12 << " r13=" << context.R13
        << " r14=" << context.R14 << " r15=" << context.R15 << '\n';
    uintptr_t stackPointer = static_cast<uintptr_t>(context.Rsp);
#else
    output << "eip=" << std::hex << context.Eip << " esp=" << context.Esp << " ebp=" << context.Ebp
        << " eax=" << context.Eax << " ebx=" << context.Ebx << " ecx=" << context.Ecx << " edx=" << context.Edx
        << " esi=" << context.Esi << " edi=" << context.Edi << '\n';
    uintptr_t stackPointer = context.Esp;
#endif
    // 只读取本次子进程的现场，保留调用者栈，便于核对优化后的寄存器还原。
    for (size_t i = 0; i < 256; ++i)
    {
        uintptr_t address = stackPointer + i * sizeof(uintptr_t);
        uintptr_t value = 0;
        SIZE_T copied = 0;
        if (!ReadProcessMemory(process, reinterpret_cast<const void*>(address), &value, sizeof(value), &copied) || copied != sizeof(value))
        {
            break;
        }
        output << "stack[" << address << "]=" << value << '\n';
    }
    Handle dump(CreateFileA((prefix + ".dmp").c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr), &CloseHandle);
    bool saved = false;
    if (dump.get() != INVALID_HANDLE_VALUE)
    {
        EXCEPTION_RECORD record = {};
        if (exception != nullptr)
        {
            record = exception->ExceptionRecord;
        }
        EXCEPTION_POINTERS pointers = { &record, &context };
        // 异常结构在调试器地址空间；寄存器及异常地址仍是目标进程的原始现场。
        MINIDUMP_EXCEPTION_INFORMATION info = { threadId, &pointers, FALSE };
        saved = MiniDumpWriteDump(process, processId, dump.get(),
            static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithThreadInfo), exception == nullptr ? nullptr : &info, nullptr, nullptr) != FALSE;
        output << "dump_saved=" << saved << " error=" << std::dec << (saved ? 0 : GetLastError()) << '\n';
    }
    else
    {
        dump.release();
        output << "dump_open_error=" << std::dec << GetLastError() << '\n';
    }
    std::string directory = executable.substr(0, executable.find_last_of("\\/"));
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_FAIL_CRITICAL_ERRORS);
    bool symbols = SymInitialize(process, directory.c_str(), TRUE) != FALSE;
    output << "symbols_initialized=" << symbols << '\n';
    if (symbols)
    {
        try
        {
            STACKFRAME64 frame = {};
            DWORD machine = 0;
#if defined(_M_X64)
            machine = IMAGE_FILE_MACHINE_AMD64;
            frame.AddrPC.Offset = context.Rip;
            frame.AddrStack.Offset = context.Rsp;
            frame.AddrFrame.Offset = context.Rbp;
#else
            machine = IMAGE_FILE_MACHINE_I386;
            frame.AddrPC.Offset = context.Eip;
            frame.AddrStack.Offset = context.Esp;
            frame.AddrFrame.Offset = context.Ebp;
#endif
            frame.AddrPC.Mode = frame.AddrStack.Mode = frame.AddrFrame.Mode = AddrModeFlat;
            std::vector<unsigned char> storage(sizeof(SYMBOL_INFO) + MAX_SYM_NAME);
            SYMBOL_INFO* symbol = new (storage.data()) SYMBOL_INFO();
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;
            for (size_t depth = 0; depth < 64 && frame.AddrPC.Offset != 0; ++depth)
            {
                DWORD64 displacement = 0;
                output << '#' << std::dec << depth << " 0x" << std::hex << frame.AddrPC.Offset
                    << " sp=" << frame.AddrStack.Offset << " frame=" << frame.AddrFrame.Offset;
                if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol))
                {
                    output << ' ' << symbol->Name << "+0x" << displacement << " module=0x" << symbol->ModBase;
                }
                IMAGEHLP_LINE64 line = {};
                line.SizeOfStruct = sizeof(line);
                DWORD lineDisplacement = 0;
                if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &lineDisplacement, &line))
                {
                    output << ' ' << line.FileName << ':' << std::dec << line.LineNumber;
                }
                output << '\n';
                if (!StackWalk64(machine, process, thread.get(), &frame, &context, nullptr,
                    SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
                {
                    break;
                }
            }
        }
        catch (...)
        {
            SymCleanup(process);
            throw;
        }
        SymCleanup(process);
    }
    output.close();
    return saved && bool(output);
}

bool LockFreeHashMapCrashRunner::captureTimeout(HANDLE process, DWORD processId,
    const std::string& executable, const std::string& prefix)
{
    typedef std::unique_ptr<void, decltype(&CloseHandle)> Handle;
    Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0), &CloseHandle);
    if (snapshot.get() == INVALID_HANDLE_VALUE)
    {
        snapshot.release();
        return false;
    }
    THREADENTRY32 entry = {};
    entry.dwSize = sizeof(entry);
    bool saved = false;
    for (BOOL available = Thread32First(snapshot.get(), &entry); available; available = Thread32Next(snapshot.get(), &entry))
    {
        if (entry.th32OwnerProcessID != processId)
        {
            continue;
        }
        Handle thread(OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION, FALSE, entry.th32ThreadID), &CloseHandle);
        if (!thread || GetProcessIdOfThread(thread.get()) != processId || SuspendThread(thread.get()) == DWORD(-1))
        {
            continue;
        }
        // 无论符号加载或文本分配是否抛异常，都只撤销本诊断增加的一次暂停。
        try
        {
            bool captured = capture(process, processId, entry.th32ThreadID, nullptr, executable,
                prefix + "-thread" + std::to_string(entry.th32ThreadID));
            saved = captured || saved;
        }
        catch (...)
        {
            ResumeThread(thread.get());
            throw;
        }
        if (ResumeThread(thread.get()) == DWORD(-1))
        {
            return false;
        }
    }
    return saved;
}
#endif

bool LockFreeHashMapCrashRunner::inspect(const std::string& path, const std::string& executable)
{
#if defined(_WIN32)
    std::ifstream input(path.c_str(), std::ios::binary | std::ios::ate);
    if (!input || input.tellg() <= 0 || input.tellg() > 16 * 1024 * 1024)
    {
        return false;
    }
    std::vector<unsigned char> bytes(static_cast<size_t>(input.tellg()));
    input.seekg(0);
    input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    if (!input || bytes.size() < sizeof(MINIDUMP_HEADER))
    {
        return false;
    }
    // 转储中的偏移不作为 C++ 对象地址使用，按值复制后再核对长度和索引。
    MINIDUMP_HEADER header = {};
    std::memcpy(&header, bytes.data(), sizeof(header));
    if (header.Signature != MINIDUMP_SIGNATURE || header.StreamDirectoryRva > bytes.size() ||
        header.NumberOfStreams > (bytes.size() - header.StreamDirectoryRva) / sizeof(MINIDUMP_DIRECTORY))
    {
        return false;
    }
    DWORD64 imageBase = 0;
    DWORD imageSize = 0;
    bool contextMatches = false;
    size_t stackWordSize = sizeof(DWORD64);
    for (ULONG32 stream = 0; stream < header.NumberOfStreams; ++stream)
    {
        MINIDUMP_DIRECTORY directory = {};
        std::memcpy(&directory, bytes.data() + header.StreamDirectoryRva + stream * sizeof(directory), sizeof(directory));
        if (directory.Location.Rva > bytes.size() || directory.Location.DataSize > bytes.size() - directory.Location.Rva)
        {
            return false;
        }
        if (directory.StreamType == SystemInfoStream)
        {
            if (directory.Location.DataSize < sizeof(MINIDUMP_SYSTEM_INFO))
            {
                return false;
            }
            MINIDUMP_SYSTEM_INFO system = {};
            std::memcpy(&system, bytes.data() + directory.Location.Rva, sizeof(system));
            stackWordSize = system.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ? sizeof(DWORD) : sizeof(DWORD64);
#if defined(_M_X64)
            contextMatches = system.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64;
#else
            contextMatches = system.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL;
#endif
            std::cout << "context_matches=" << contextMatches << " architecture=" << system.ProcessorArchitecture << '\n';
        }
        else if (directory.StreamType == ThreadInfoListStream)
        {
            if (directory.Location.DataSize < sizeof(MINIDUMP_THREAD_INFO_LIST))
            {
                return false;
            }
            MINIDUMP_THREAD_INFO_LIST list = {};
            std::memcpy(&list, bytes.data() + directory.Location.Rva, sizeof(list));
            if (list.SizeOfHeader < sizeof(list) || list.SizeOfHeader > directory.Location.DataSize ||
                list.SizeOfEntry < sizeof(MINIDUMP_THREAD_INFO) ||
                list.NumberOfEntries > (directory.Location.DataSize - list.SizeOfHeader) / list.SizeOfEntry)
            {
                return false;
            }
            for (ULONG32 i = 0; i < list.NumberOfEntries; ++i)
            {
                MINIDUMP_THREAD_INFO info = {};
                std::memcpy(&info, bytes.data() + directory.Location.Rva + list.SizeOfHeader + i * list.SizeOfEntry, sizeof(info));
                std::cout << "thread_info=" << std::dec << info.ThreadId << " user_ms=" << double(info.UserTime) / 10000
                    << " kernel_ms=" << double(info.KernelTime) / 10000 << " affinity=" << std::hex << info.Affinity
                    << " flags=" << info.DumpFlags << " error=" << info.DumpError << '\n';
            }
        }
        else if (directory.StreamType == ModuleListStream)
        {
            const size_t firstModule = offsetof(MINIDUMP_MODULE_LIST, Modules);
            if (directory.Location.DataSize < firstModule)
            {
                return false;
            }
            ULONG32 count = 0;
            std::memcpy(&count, bytes.data() + directory.Location.Rva, sizeof(count));
            if (count > (directory.Location.DataSize - firstModule) / sizeof(MINIDUMP_MODULE))
            {
                return false;
            }
            for (ULONG32 i = 0; i < count; ++i)
            {
                MINIDUMP_MODULE module = {};
                std::memcpy(&module, bytes.data() + directory.Location.Rva + firstModule + i * sizeof(module), sizeof(module));
                std::cout << "module=" << std::dec << i << " base=" << std::hex << module.BaseOfImage
                    << " size=" << module.SizeOfImage << " timestamp=" << module.TimeDateStamp << '\n';
                if (i == 0)
                {
                    imageBase = module.BaseOfImage;
                    imageSize = module.SizeOfImage;
                }
            }
        }
    }
    // 只加载调用方明确给出的匹配EXE/PDB；不联网查符号，不把栈中任意整数认作调用帧。
    std::unique_ptr<void, decltype(&SymCleanup)> symbols(nullptr, &SymCleanup);
    if (!executable.empty() && imageBase != 0 && imageSize != 0)
    {
        std::string directory = executable.substr(0, executable.find_last_of("\\/"));
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_FAIL_CRITICAL_ERRORS);
        if (SymInitialize(GetCurrentProcess(), directory.c_str(), FALSE))
        {
            symbols.reset(GetCurrentProcess());
            std::cout << "symbol_image_base=" << std::hex << SymLoadModuleEx(GetCurrentProcess(), nullptr,
                executable.c_str(), nullptr, imageBase, imageSize, nullptr, 0) << '\n';
        }
    }
    std::vector<unsigned char> storage(sizeof(SYMBOL_INFO) + MAX_SYM_NAME);
    SYMBOL_INFO* symbol = new (storage.data()) SYMBOL_INFO();
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;
    for (ULONG32 stream = 0; stream < header.NumberOfStreams; ++stream)
    {
        MINIDUMP_DIRECTORY directory = {};
        std::memcpy(&directory, bytes.data() + header.StreamDirectoryRva + stream * sizeof(directory), sizeof(directory));
        if (directory.Location.Rva > bytes.size() || directory.Location.DataSize > bytes.size() - directory.Location.Rva)
        {
            return false;
        }
        if (directory.StreamType != ThreadListStream)
        {
            continue;
        }
        const size_t threadOffset = offsetof(MINIDUMP_THREAD_LIST, Threads);
        if (directory.Location.DataSize < threadOffset)
        {
            return false;
        }
        ULONG32 threadCount = 0;
        std::memcpy(&threadCount, bytes.data() + directory.Location.Rva, sizeof(threadCount));
        if (threadCount > (directory.Location.DataSize - threadOffset) / sizeof(MINIDUMP_THREAD))
        {
            return false;
        }
        for (ULONG32 index = 0; index < threadCount; ++index)
        {
            MINIDUMP_THREAD thread = {};
            std::memcpy(&thread, bytes.data() + directory.Location.Rva + threadOffset + index * sizeof(thread), sizeof(thread));
            if (thread.Stack.Memory.Rva > bytes.size() || thread.Stack.Memory.DataSize > bytes.size() - thread.Stack.Memory.Rva)
            {
                return false;
            }
            std::cout << "thread=" << std::dec << thread.ThreadId << '\n';
            if (thread.ThreadContext.Rva > bytes.size() || thread.ThreadContext.DataSize > bytes.size() - thread.ThreadContext.Rva)
            {
                return false;
            }
            if (contextMatches && thread.ThreadContext.DataSize >= sizeof(CONTEXT))
            {
                CONTEXT context = {};
                std::memcpy(&context, bytes.data() + thread.ThreadContext.Rva, sizeof(context));
#if defined(_M_X64)
                std::cout << "rip=" << std::hex << context.Rip << " rsp=" << context.Rsp << " rax=" << context.Rax
                    << " rbx=" << context.Rbx << " rcx=" << context.Rcx << " rdx=" << context.Rdx << '\n';
#else
                std::cout << "eip=" << std::hex << context.Eip << " esp=" << context.Esp << " eax=" << context.Eax
                    << " ebx=" << context.Ebx << " ecx=" << context.Ecx << " edx=" << context.Edx << '\n';
#endif
            }
            for (size_t offset = 0; offset + stackWordSize <= thread.Stack.Memory.DataSize; offset += stackWordSize)
            {
                DWORD64 value = 0;
                std::memcpy(&value, bytes.data() + thread.Stack.Memory.Rva + offset, stackWordSize);
                std::cout << "stack[" << std::hex << thread.Stack.StartOfMemoryRange + offset << "]=" << value;
                DWORD64 displacement = 0;
                if (symbols && value >= imageBase && value - imageBase < imageSize &&
                    SymFromAddr(GetCurrentProcess(), value, &displacement, symbol))
                {
                    std::cout << " possible_symbol=" << symbol->Name << "+0x" << displacement;
                }
                std::cout << '\n';
            }
        }
    }
    return bool(std::cout);
#else
    (void)path;
    (void)executable;
    return false;
#endif
}

int LockFreeHashMapCrashRunner::run(const std::string& executable, const std::string& argument, const std::string& prefix)
{
#if defined(_WIN32)
    typedef std::unique_ptr<void, decltype(&CloseHandle)> Handle;
    std::string command = '"' + executable + "\" " + argument;
    std::vector<char> buffer(command.begin(), command.end());
    buffer.push_back(0);
    SECURITY_ATTRIBUTES security = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    Handle output(CreateFileA((prefix + ".log").c_str(), GENERIC_WRITE, FILE_SHARE_READ, &security,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr), &CloseHandle);
    if (output.get() == INVALID_HANDLE_VALUE)
    {
        output.release();
        return -1;
    }
    Handle input(CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr), &CloseHandle);
    if (input.get() == INVALID_HANDLE_VALUE)
    {
        input.release();
        return -1;
    }
    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input.get();
    startup.hStdOutput = startup.hStdError = output.get();
    PROCESS_INFORMATION child = {};
    if (!CreateProcessA(executable.c_str(), buffer.data(), nullptr, nullptr, TRUE,
        DEBUG_ONLY_THIS_PROCESS | CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child))
    {
        return -1;
    }
    Handle process(child.hProcess, &CloseHandle);
    Handle initialThread(child.hThread, &CloseHandle);
    bool sharedProcessHandle = false, sharedThreadHandle = false;
    bool exited = false, timedOut = false, initialBreakpoint = false;
    int result = -1;
    ULONGLONG begin = GetTickCount64();
    try
    {
        while (!exited)
        {
            if (GetTickCount64() - begin > 120000 && !timedOut)
            {
                timedOut = true;
                bool captured = captureTimeout(process.get(), child.dwProcessId, executable, prefix + "-timeout");
                std::cout << "TIMEOUT captured=" << captured << " prefix=" << prefix << std::endl;
                if (!TerminateProcess(process.get(), 124))
                {
                    break;
                }
            }
            if (GetTickCount64() - begin > 125000)
            {
                break;
            }
            DEBUG_EVENT event = {};
            if (!WaitForDebugEvent(&event, 100))
            {
                if (GetLastError() != ERROR_SEM_TIMEOUT)
                {
                    break;
                }
                continue;
            }
            DWORD continuation = DBG_CONTINUE;
            if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
            {
                sharedProcessHandle = event.u.CreateProcessInfo.hProcess == process.get();
                sharedThreadHandle = event.u.CreateProcessInfo.hThread == initialThread.get();
                if (event.u.CreateProcessInfo.hFile != nullptr)
                {
                    CloseHandle(event.u.CreateProcessInfo.hFile);
                }
            }
            else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT && event.u.LoadDll.hFile != nullptr)
            {
                CloseHandle(event.u.LoadDll.hFile);
            }
            else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
            {
                // 只接收加载器的首次断点，不能把被测程序后续的断点异常吞掉。
                if (!initialBreakpoint && event.u.Exception.dwFirstChance &&
                    event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                {
                    initialBreakpoint = true;
                }
                else
                {
                    continuation = DBG_EXCEPTION_NOT_HANDLED;
                    if (!event.u.Exception.dwFirstChance || event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
                    {
                        bool captured = capture(process.get(), child.dwProcessId, event.dwThreadId,
                            &event.u.Exception, executable, prefix + (event.u.Exception.dwFirstChance ? "-first" : ""));
                        std::cout << "CRASH captured=" << captured << " first=" << event.u.Exception.dwFirstChance
                            << " prefix=" << prefix << std::endl;
                    }
                }
            }
            else if (event.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT && event.dwThreadId == child.dwThreadId && sharedThreadHandle)
            {
                initialThread.release();
            }
            else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
            {
                result = static_cast<int>(event.u.ExitProcess.dwExitCode);
                exited = true;
                // 调试事件句柄由 ContinueDebugEvent 关闭；创建接口若返回同一值就不再重复关闭。
                if (sharedProcessHandle)
                {
                    process.release();
                }
                if (sharedThreadHandle)
                {
                    initialThread.release();
                }
            }
            if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, continuation))
            {
                result = -1;
                break;
            }
        }
    }
    catch (...)
    {
        if (process)
        {
            TerminateProcess(process.get(), 125);
        }
        DebugActiveProcessStop(child.dwProcessId);
        if (sharedProcessHandle)
        {
            process.release();
        }
        if (sharedThreadHandle)
        {
            initialThread.release();
        }
        throw;
    }
    if (!exited)
    {
        if (process)
        {
            TerminateProcess(process.get(), 125);
        }
        DebugActiveProcessStop(child.dwProcessId);
        if (sharedProcessHandle)
        {
            process.release();
        }
        if (sharedThreadHandle)
        {
            initialThread.release();
        }
    }
    return FlushFileBuffers(output.get()) ? (timedOut ? -2 : result) : -1;
#else
    (void)executable;
    (void)argument;
    (void)prefix;
    return -1;
#endif
}

/** 对原 EXE 重复复现；不修改原程序，不把调试耗时当性能数据
@param [in] argc 参数数量
@param [in] argv EXE、数字参数、新输出前缀、最多 100 轮
@return 0 全部正常返回，1 子进程失败，2 参数或设施错误
*/
int main(int argc, char** argv)
{
    try
    {
        if ((argc == 3 || argc == 4) && std::string(argv[1]) == "dump")
        {
            return LockFreeHashMapCrashRunner::inspect(argv[2], argc == 4 ? argv[3] : "") ? 0 : 2;
        }
        if (argc != 5 || std::string(argv[1]).find('"') != std::string::npos)
        {
            return 2;
        }
        char* end = nullptr;
        errno = 0;
        unsigned long argument = std::strtoul(argv[2], &end, 10);
        if (errno != 0 || end == argv[2] || *end != 0 || argument == 0 || argument > 64999)
        {
            return 2;
        }
        unsigned long rounds = std::strtoul(argv[4], &end, 10);
        if (errno != 0 || end == argv[4] || *end != 0 || rounds == 0 || rounds > 100)
        {
            return 2;
        }
        for (unsigned long round = 1; round <= rounds; ++round)
        {
            std::string prefix = std::string(argv[3]) + "-r" + std::to_string(round);
            int code = LockFreeHashMapCrashRunner::run(argv[1], argv[2], prefix);
            std::cout << "DEBUG_RUN round=" << round << " exit=" << code << std::endl;
            if (code != 0)
            {
                return 1;
            }
        }
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "DEBUG_RUN_ERROR " << error.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "DEBUG_RUN_ERROR unknown" << std::endl;
    }
    return 2;
}