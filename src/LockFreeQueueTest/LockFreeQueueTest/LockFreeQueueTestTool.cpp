#include "LockFreeQueueTestTool.h"
#include "LockFreeQueueTestWhitebox.h"
#include "LockFreeQueueTestRunner.h"
#include "LockFreeQueueTestResults.h"
#include "../../CSystem/CSystem/src/CSystem.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#elif defined(__APPLE__)
#include <CommonCrypto/CommonDigest.h>
#else
#include <openssl/sha.h>
#endif

LockFreeQueueTestTool::LockFreeQueueTestTool(int32_t argc, char** argv) :
m_command(argc > 1 ? argv[1] : "help")
{
    const std::set<std::string> flags = {"matrix", "consumers", "projects", "resume",
        "build-only", "prepare-only", "measure-only", "plan"};
    const std::set<std::string> values = {"root", "source", "output", "batch", "cases", "arch",
        "configuration", "repeat", "stress-seconds", "peer-snapshot-dir", "vcvars",
        "calls", "rounds",
        "allocation-rounds", "affinity", "items", "producers", "modes", "duration-ms", "masks",
        "capacities", "suite", "profiles"};
    for (int32_t i = 2; i < argc; ++i)
    {
        std::string key(argv[i]);
        if (key.compare(0, 2, "--") != 0)
        {
            throw std::runtime_error("Expected --option, got " + key);
        }
        key = key.substr(2);
        if (m_options.count(key))
        {
            throw std::runtime_error("Duplicate option: " + key);
        }
        if (flags.count(key))
        {
            m_options[key] = "true";
        }
        else if (values.count(key) && i + 1 < argc && std::string(argv[i + 1]).compare(0, 2, "--") != 0)
        {
            m_options[key] = argv[++i];
        }
        else
        {
            throw std::runtime_error("Unknown option or missing value: " + key);
        }
    }
    std::set<std::string> permitted = {"root"};
    if (m_command == "whitebox")
    {
        permitted.insert("source");
        permitted.insert("output");
    }
    else if (m_command == "self-test" || m_command == "summarize")
    {
        permitted.insert("batch");
    }
    else if (m_command != "help" && m_command != "--help")
    {
        permitted.insert({"batch", "arch", "vcvars", "affinity", "resume", "plan", "build-only", "prepare-only", "measure-only"});
        if (m_command == "tests")
        {
            permitted.insert({"cases", "configuration", "matrix", "consumers", "projects", "repeat", "stress-seconds", "peer-snapshot-dir"});
        }
        else
        {
            permitted.insert("rounds");
            if (m_command == "performance")
            {
                permitted.insert({"calls", "allocation-rounds", "suite", "profiles"});
            }
            else if (m_command == "scheduling")
            {
                permitted.insert({"duration-ms", "masks", "modes", "capacities", "producers"});
            }
            else if (m_command == "shared-ptr")
            {
                permitted.insert({"items", "producers", "modes"});
            }
        }
    }
    for (std::map<std::string, std::string>::const_iterator it = m_options.begin(); it != m_options.end(); ++it)
    {
        if (!permitted.count(it->first))
        {
            throw std::runtime_error("Option does not apply to " + m_command + ": " + it->first);
        }
    }
    m_root = path(option("root", parent(parent(parent(parent(__FILE__))))));
    if (!exists(join(m_root, "src/LockFreeQueueTest/LockFreeQueueTest/whitebox-patches.json")))
    {
        throw std::runtime_error("Cannot locate checkout; pass --root");
    }
}

std::string LockFreeQueueTestTool::option(const std::string& key, const std::string& fallback) const
{
    std::map<std::string, std::string>::const_iterator found = m_options.find(key);
    return found == m_options.end() ? fallback : found->second;
}

std::vector<std::string> LockFreeQueueTestTool::list(const std::string& key, const std::string& fallback) const
{
    std::vector<std::string> result = split(option(key, fallback), ',');
    std::set<std::string> unique(result.begin(), result.end());
    if (unique.size() != result.size() || unique.count(""))
    {
        throw std::runtime_error("Empty or duplicate selection: " + key);
    }
    return result;
}

int32_t LockFreeQueueTestTool::number(const std::string& key, int32_t fallback, int32_t low, int32_t high) const
{
    std::string text = option(key, integer(fallback));
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
    {
        throw std::runtime_error("Expected decimal integer: " + key);
    }
    double value = std::strtod(text.c_str(), nullptr);
    if (value < low || value > high)
    {
        throw std::runtime_error("Integer out of range: " + key);
    }
    return static_cast<int32_t>(value);
}

bool LockFreeQueueTestTool::flag(const std::string& key) const
{
    return option(key) == "true";
}

std::string LockFreeQueueTestTool::root() const
{
    return m_root;
}

std::string LockFreeQueueTestTool::tests() const
{
    return join(m_root, "src/LockFreeQueueTest/LockFreeQueueTest");
}

std::string LockFreeQueueTestTool::source() const
{
    return join(m_root, "src/LockFreeQueue/LockFreeQueue/src");
}

std::string LockFreeQueueTestTool::batch() const
{
    std::string temporary = join(m_root, "src/temp/LockFreeQueueTest");
    std::string value = option("batch");
    if (value.empty())
    {
        throw std::runtime_error("Supply a unique --batch below src/temp/LockFreeQueueTest");
    }
    std::string output = path(join(temporary, value), false);
    inside(output, temporary);
    return output;
}

std::string LockFreeQueueTestTool::join(const std::string& base, const std::string& file)
{
    return CSystem::joinPath(base, file);
}

std::string LockFreeQueueTestTool::parent(const std::string& value)
{
    std::string result(value);
    while (!result.empty() && (result.back() == '/' || result.back() == '\\'))
    {
        result.pop_back();
    }
    size_t position = result.find_last_of("/\\");
    return position == std::string::npos ? "." : result.substr(0, position);
}

std::string LockFreeQueueTestTool::name(const std::string& value)
{
    return CSystem::GetName(value, 1);
}

std::string LockFreeQueueTestTool::path(const std::string& value, bool existing)
{
    std::string result = CSystem::canonicalPathUtf8(value, existing);
    if (result.empty())
    {
        throw std::runtime_error("Invalid path: " + value);
    }
    return result;
}

bool LockFreeQueueTestTool::exists(const std::string& value)
{
    return !CSystem::canonicalPathUtf8(value, true).empty();
}

void LockFreeQueueTestTool::directory(const std::string& value)
{
    if (!CSystem::createDirectoriesUtf8(value))
    {
        throw std::runtime_error("Cannot create directory: " + value);
    }
}

void LockFreeQueueTestTool::inside(const std::string& value, const std::string& allowed)
{
    std::string current = path(value, false), base = path(allowed, false);
    if (!CSystem::isPathWithin(current, base, false))
    {
        throw std::runtime_error("Output escapes test temp: " + value);
    }
    // Resolve every existing ancestor too, so a junction cannot redirect the output.
    while (!exists(current) && current != parent(current))
    {
        current = parent(current);
    }
    std::string actual = path(current);
    std::string lexical = path(current, false);
    if (actual != lexical)
    {
        throw std::runtime_error("Output ancestor resolves through a link: " + current);
    }
}

std::vector<std::string> LockFreeQueueTestTool::sources(const std::string& directoryName)
{
#ifdef _WIN32
    // CSystem's legacy finder is recursive/ANSI. Whitebox inputs require one UTF8 directory,
    // with reparse entries rejected; keep that narrower test-input policy here.
    std::string pattern = join(directoryName, "*");
    int32_t count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pattern.c_str(), -1, nullptr, 0);
    if (!count)
    {
        throw std::runtime_error("Invalid source directory encoding");
    }
    std::vector<wchar_t> wide(count);
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pattern.c_str(), -1, wide.data(), count);
    WIN32_FIND_DATAW entry = {};
    HANDLE search = FindFirstFileW(wide.data(), &entry);
    if (search == INVALID_HANDLE_VALUE)
    {
        throw std::runtime_error("Cannot list source directory: " + directoryName);
    }
    std::vector<std::string> result;
    try
    {
        do
        {
            if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                continue;
            }
            int32_t size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, entry.cFileName, -1, nullptr, 0, nullptr, nullptr);
            if (!size)
            {
                throw std::runtime_error("Invalid source filename");
            }
            std::vector<char> utf8(size);
            WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, entry.cFileName, -1, utf8.data(), size, nullptr, nullptr);
            std::string filename(utf8.data()), suffix = filename.substr(filename.find_last_of('.') == std::string::npos ? filename.size() : filename.find_last_of('.'));
            if (suffix != ".h" && suffix != ".inl" && suffix != ".cpp")
            {
                continue;
            }
            if (entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            {
                throw std::runtime_error("Source entry is a reparse point");
            }
            result.push_back(join(directoryName, filename));
        }
        while (FindNextFileW(search, &entry));
        if (GetLastError() != ERROR_NO_MORE_FILES)
        {
            throw std::runtime_error("Incomplete source directory listing");
        }
    }
    catch (...)
    {
        FindClose(search);
        throw;
    }
    FindClose(search);
#else
    std::vector<std::string> all = CSystem::findFilePath(directoryName, 2, "h.inl.cpp");
    std::vector<std::string> result;
    std::string base = path(directoryName);
    for (size_t i = 0; i < all.size(); ++i)
    {
        if (path(parent(all[i])) == base)
        {
            result.push_back(all[i]);
        }
    }
#endif
    std::sort(result.begin(), result.end());
    return result;
}

std::string LockFreeQueueTestTool::read(const std::string& file)
{
    intptr_t handle = CSystem::openFileHandle(file.c_str(), 1);
    if (handle == -1)
    {
        throw std::runtime_error("Cannot open input: " + file);
    }
    std::string data;
    try
    {
        uint64_t size = 0;
        if (!CSystem::fileHandleSize(handle, &size) || size > 256 * 1024 * 1024)
        {
            throw std::runtime_error("Input is unreadable or exceeds 256MiB: " + file);
        }
        data.resize(static_cast<size_t>(size));
        for (uint64_t offset = 0; offset < size;)
        {
            uint32_t count = static_cast<uint32_t>((std::min)(uint64_t(16 * 1024 * 1024), size - offset)), received = 0;
            if (!CSystem::readFileHandle(handle, offset, reinterpret_cast<uint8_t*>(&data[static_cast<size_t>(offset)]), count, &received) || received != count)
            {
                throw std::runtime_error("Incomplete input: " + file);
            }
            offset += count;
        }
    }
    catch (...)
    {
        CSystem::closeFileHandle(&handle);
        throw;
    }
    if (!CSystem::closeFileHandle(&handle))
    {
        throw std::runtime_error("Input close failed: " + file);
    }
    return data;
}

void LockFreeQueueTestTool::write(const std::string& file, const std::string& data)
{
    directory(parent(file));
    intptr_t handle = CSystem::openFileHandle(file.c_str(), exists(file) ? 3 : 2);
    if (handle == -1)
    {
        throw std::runtime_error("Cannot open output: " + file);
    }
    bool valid = CSystem::resizeFileHandle(handle, data.size());
    for (size_t offset = 0; valid && offset < data.size();)
    {
        uint32_t count = static_cast<uint32_t>((std::min)(size_t(16 * 1024 * 1024), data.size() - offset));
        valid = CSystem::writeFileHandle(handle, offset, reinterpret_cast<const uint8_t*>(data.data() + offset), count);
        offset += count;
    }
    valid = CSystem::flushFileHandle(handle) && valid;
    valid = CSystem::closeFileHandle(&handle) && valid;
    if (!valid || read(file) != data)
    {
        throw std::runtime_error("Write verification failed: " + file);
    }
}

std::string LockFreeQueueTestTool::plain(const std::string& data)
{
    std::string result;
    size_t begin = data.compare(0, 3, "\xef\xbb\xbf") == 0 ? 3 : 0;
    for (size_t i = begin; i < data.size(); ++i)
    {
        if (data[i] == '\r')
        {
            result += '\n';
            if (i + 1 < data.size() && data[i + 1] == '\n')
            {
                ++i;
            }
        }
        else
        {
            result += data[i];
        }
    }
    return result;
}

std::string LockFreeQueueTestTool::compiled(const std::string& text)
{
    std::string result("\xef\xbb\xbf"), normalized = plain(text);
    for (size_t i = 0; i < normalized.size(); ++i)
    {
        if (normalized[i] == '\n')
        {
            result += '\r';
        }
        result += normalized[i];
    }
    return result;
}

std::string LockFreeQueueTestTool::digest(const std::string& data)
{
    unsigned char hash[32] = {};
#ifdef _WIN32
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE object = nullptr;
    NTSTATUS code = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (code >= 0)
    {
        code = BCryptCreateHash(algorithm, &object, nullptr, 0, nullptr, 0, 0);
    }
    if (code >= 0 && data.size() <= (std::numeric_limits<ULONG>::max)())
    {
        code = BCryptHashData(object, reinterpret_cast<PUCHAR>(const_cast<char*>(data.data())), static_cast<ULONG>(data.size()), 0);
    }
    else if (code >= 0)
    {
        code = -1;
    }
    if (code >= 0)
    {
        code = BCryptFinishHash(object, hash, sizeof(hash), 0);
    }
    if (object)
    {
        BCryptDestroyHash(object);
    }
    if (algorithm)
    {
        BCryptCloseAlgorithmProvider(algorithm, 0);
    }
    if (code < 0)
    {
        throw std::runtime_error("SHA256 failed");
    }
#elif defined(__APPLE__)
    if (data.size() > (std::numeric_limits<CC_LONG>::max)() || !CC_SHA256(data.data(), static_cast<CC_LONG>(data.size()), hash))
    {
        throw std::runtime_error("SHA256 failed");
    }
#else
    if (!SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), hash))
    {
        throw std::runtime_error("SHA256 failed");
    }
#endif
    std::ostringstream output;
    for (size_t i = 0; i < sizeof(hash); ++i)
    {
        output << std::hex << std::setfill('0') << std::setw(2) << static_cast<int32_t>(hash[i]);
    }
    return output.str();
}

Json::Value LockFreeQueueTestTool::load(const std::string& file)
{
    Json::Value result;
    Json::Reader reader;
    if (!reader.parse(plain(read(file)), result, false))
    {
        throw std::runtime_error("Invalid JSON: " + file);
    }
    return result;
}

void LockFreeQueueTestTool::save(const std::string& file, const Json::Value& value)
{
    write(file, Json::StyledWriter().write(value));
}

std::string LockFreeQueueTestTool::replaceOnce(const std::string& text, const std::string& before, const std::string& after)
{
    size_t pos = text.find(before);
    if (before.empty() || pos == std::string::npos || text.find(before, pos + before.size()) != std::string::npos)
    {
        throw std::runtime_error("Patch context must occur exactly once: " + before.substr(0, 100));
    }
    std::string result(text);
    result.replace(pos, before.size(), after);
    return result;
}

std::vector<std::string> LockFreeQueueTestTool::split(const std::string& text, char delimiter)
{
    std::vector<std::string> result;
    size_t begin = 0, end = 0;
    while ((end = text.find(delimiter, begin)) != std::string::npos)
    {
        result.push_back(text.substr(begin, end - begin));
        begin = end + 1;
    }
    result.push_back(text.substr(begin));
    return result;
}

std::string LockFreeQueueTestTool::integer(int64_t value)
{
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

std::string LockFreeQueueTestTool::executable()
{
    return CSystem::currentExecutablePathUtf8();
}

std::string LockFreeQueueTestTool::quote(const std::string& value)
{
    if (value.find_first_of("\r\n\"%!") != std::string::npos)
    {
        throw std::runtime_error("Unsafe cmd argument: " + value);
    }
    std::string result = "\"" + value;
    for (size_t i = value.size(); i && value[i - 1] == '\\'; --i)
    {
        result += '\\';
    }
    return result + "\"";
}

Json::Value LockFreeQueueTestTool::process(const std::vector<std::string>& command, const std::string& cwd,
    const std::string& log, int32_t timeoutSeconds)
{
#ifdef _WIN32
    if (command.empty())
    {
        throw std::runtime_error("Empty command");
    }
    directory(parent(log));
    std::vector<std::string> arguments(command.begin() + 1, command.end());
    uint32_t pid = 0;
    uint64_t monitor = 0;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    uint32_t error = CSystem::startCapturedProcess(command[0], arguments, log, pid, monitor);
    if (error)
    {
        throw std::runtime_error("Child launch failed " + integer(error) + ": " + command[0]);
    }
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    HANDLE child = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(monitor));
    if (!job || !SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits)) || !AssignProcessToJobObject(job, child))
    {
        if (job)
        {
            CloseHandle(job);
        }
        TerminateProcess(child, 125);
        CSystem::closeProcessMonitor(monitor);
        throw std::runtime_error("Cannot establish child-process cleanup boundary");
    }
    int32_t code = 0;
    bool timeout = false;
    for (;;)
    {
        int32_t state = CSystem::pollProcessMonitor(monitor, code);
        if (state != 0)
        {
            if (state < 0)
            {
                code = 125;
            }
            break;
        }
        if (std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count() > timeoutSeconds)
        {
            TerminateJobObject(job, 124);
            WaitForSingleObject(child, 5000);
            timeout = true;
            code = 124;
            break;
        }
        CSystem::Sleep(10);
    }
    CSystem::closeProcessMonitor(monitor);
    CloseHandle(job);
    Json::Value result;
    result["exit"] = code;
    result["seconds"] = std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
    result["timeout"] = timeout;
    // CSystem launches in the executable directory. cwd is the workload's
    // logical directory (build scripts explicitly cd there), not a launch option.
    result["cwd"] = cwd;
    result["launch_cwd"] = path(parent(command[0]));
    result["log"] = log;
    for (size_t i = 0; i < command.size(); ++i)
    {
        result["command"].append(command[i]);
    }
    return result;
#else
    (void)command;
    (void)cwd;
    (void)log;
    (void)timeoutSeconds;
    throw std::runtime_error("The VS2013 host matrix requires Windows. Use native CMake/CTest on other hosts.");
#endif
}

uint64_t LockFreeQueueTestTool::affinity()
{
#ifdef _WIN32
    DWORD_PTR actual = 0, system = 0;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &actual, &system))
    {
        throw std::runtime_error("Get affinity failed");
    }
    return actual;
#else
    return 0;
#endif
}

void LockFreeQueueTestTool::affinity(uint64_t mask)
{
#ifdef _WIN32
    DWORD_PTR actual = 0, system = 0;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &actual, &system) || !mask || (mask & system) != mask ||
        !SetProcessAffinityMask(GetCurrentProcess(), static_cast<DWORD_PTR>(mask)))
    {
        throw std::runtime_error("Unavailable CPU affinity");
    }
    if (affinity() != mask)
    {
        throw std::runtime_error("Affinity verification failed");
    }
#else
    (void)mask;
    throw std::runtime_error("Affinity matrix is Windows-only");
#endif
}

int32_t LockFreeQueueTestTool::run()
{
    if (m_command == "help" || m_command == "--help")
    {
        std::cout << "QueueTest C++ host: whitebox | tests | performance | scheduling | shared-ptr | summarize | self-test\n"
            "All outputs: --batch NAME under src/temp/LockFreeQueueTest. Lists use commas.\n"
            "tests --cases all|instant|stress|1,3-8 --matrix --consumers --projects\n"
            "whitebox --source DIR --output PRIVATE_TEMP_DIR\n"
            "--arch both|x64|Win32 --rounds N --plan (validate/list matrix without measuring)\n"
            "--prepare-only then --measure-only --resume freezes source and binary identities.\n"
            "See LockFreeQueueTest.h for all cases, defaults and interpretation.\n";
        return 0;
    }
    if (m_command == "whitebox")
    {
        LockFreeQueueTestWhitebox::prepare(*this, option("source", source()), option("output"));
        return 0;
    }
    if (m_command == "self-test")
    {
        selfTest();
        return 0;
    }
    LockFreeQueueTestRunner runner(*this);
    return runner.run(m_command);
}

/** Execute this isolated test program and report failure through its exit code
@param [in] argc Number of command-line arguments including the program name
@param [in] argv Argument array borrowed until main returns
@return Zero on success; nonzero for invalid arguments or failed verification
*/
int main(int argc, char** argv)
{
    try
    {
        std::vector<std::string> utf8Arguments;
        if (!CSystem::commandArguments(argc, argv, utf8Arguments))
        {
            std::cerr << "HARNESS_FAIL Invalid command-line encoding" << std::endl;
            return 1;
        }
        utf8Arguments.insert(utf8Arguments.begin(), LockFreeQueueTestTool::executable());
        std::vector<char*> pointers;
        for (size_t i = 0; i < utf8Arguments.size(); ++i)
        {
            pointers.push_back(const_cast<char*>(utf8Arguments[i].c_str()));
        }
        argc = static_cast<int32_t>(pointers.size());
        argv = pointers.data();
#ifdef _WIN32
        if (argc == 3 && std::string(argv[1]) == "run-script")
        {
            // CSystem intentionally quotes every argv element; cmd.exe does not use CRT argv parsing.
            // Keep this compiler-only adapter in the host test tool. Scripts are generated and hashed.
            try
            {
                std::string command = "call " + LockFreeQueueTestTool::quote(argv[2]);
                int32_t count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, command.c_str(), -1, nullptr, 0);
                if (!count)
                {
                    throw std::runtime_error("Compiler script path is not UTF8");
                }
                std::vector<wchar_t> wide(count);
                if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, command.c_str(), -1, wide.data(), count))
                {
                    throw std::runtime_error("Compiler script path conversion failed");
                }
                return _wsystem(wide.data());
            }
            catch (const std::exception& error)
            {
                std::cerr << error.what() << std::endl;
                return 1;
            }
        }
#endif
        if (argc == 2 && std::string(argv[1]) == "self-test-child")
        {
            std::cout << "CHILD_CWD=" << LockFreeQueueTestTool::path(".") << std::endl;
            CSystem::Sleep(5000);
            return 0;
        }
        return LockFreeQueueTestTool(argc, argv).run();
    }
    catch (const std::exception& error)
    {
        std::cerr << "HARNESS_FAIL " << error.what() << std::endl;
        return 1;
    }
}

std::string LockFreeQueueTestTool::usageDemo(const std::string& source)
{
    std::vector<std::string> lines = split(plain(source), '\n');
    bool inside = false, complete = false;
    std::string result = "#include \"LockFreeQueueAPI.h\"\n";
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (lines[i] == "// BEGIN LOCKFREEQUEUE USAGE DEMO")
        {
            if (inside || complete)
            {
                throw std::runtime_error("Duplicate usage demo");
            }
            inside = true;
        }
        else if (lines[i] == "// END LOCKFREEQUEUE USAGE DEMO")
        {
            if (!inside)
            {
                throw std::runtime_error("Unexpected demo end");
            }
            inside = false;
            complete = true;
        }
        else if (inside && !lines[i].empty())
        {
            if (lines[i].compare(0, 2, "//") != 0)
            {
                throw std::runtime_error("Uncommented source inside demo");
            }
            result += lines[i].substr(2) + '\n';
        }
    }
    if (!complete || inside || result.find("int main(") == std::string::npos)
    {
        throw std::runtime_error("Incomplete usage demo");
    }
    return result;
}

void LockFreeQueueTestTool::selfTest() const
{
    int32_t checks = 0;
    /** Count an independent successful check or report its failure through main
    @param [in] condition Whether the independently expected condition holds
    @param [in] description Non-null failure label, borrowed for this call
    */
    const std::function<void(bool, const char*)> check = [&checks](bool condition, const char* description)
    {
        if (!condition)
        {
            throw std::runtime_error(std::string("Self-test failed: ") + description);
        }
        ++checks;
    };
    check(digest("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "SHA256 known vector");
    check(digest("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "SHA256 empty vector");
    check(plain(compiled("a\nb")) == "a\nb", "BOM/CRLF round trip");
    // VS2013 may select std::string's iterator constructor for a two-literal
    // braced insert; exercise the actual command parser, including both pairs.
    std::vector<std::string> whiteboxArguments = {executable(), "whitebox", "--root", root(),
        "--source", source(), "--output", join(batch(), "cli-whitebox")};
    std::vector<char*> whiteboxPointers;
    for (size_t i = 0; i < whiteboxArguments.size(); ++i)
    {
        whiteboxPointers.push_back(&whiteboxArguments[i][0]);
    }
    LockFreeQueueTestTool whiteboxTool(static_cast<int32_t>(whiteboxPointers.size()), whiteboxPointers.data());
    check(whiteboxTool.option("source") == source() && whiteboxTool.option("output") == join(batch(), "cli-whitebox"), "whitebox CLI option pair");
    check(LockFreeQueueTestRunner::select("all").size() == 49, "registry count");
    check(LockFreeQueueTestRunner::select("instant").size() == 34, "instant registry");
    check(LockFreeQueueTestRunner::select("stress").size() == 15, "pressure registry");
    check(LockFreeQueueTestRunner::select("1,3-5,5").size() == 4, "range deduplication");
    check(LockFreeQueueTestRunner::select("47").size() == 1, "retry contract registered");
    check(LockFreeQueueTestRunner::select("48-49").size() == 2, "new public contracts registered");
    check(usageDemo(read(join(source(), "LockFreeQueue.cpp"))).find("int main(") != std::string::npos, "public demo extracted");
    const char* invalid[] = {"", "0", "51", "24", "3-1", "1,,2", "1-2-3", "99999999999999", "x", "-1"};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        bool rejected = false;
        try
        {
            LockFreeQueueTestRunner::select(invalid[i]);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        check(rejected, "invalid selection rejected");
    }
    check(LockFreeQueueTestWhitebox::strip("a\n#ifdef LOCK_FREE_QUEUE_TESTING\n#if X\ntest\n#endif\n#else\nb\n#endif\nc") == "a\nb\nc", "nested reversible patch");
    check(LockFreeQueueTestResults::median({1, 9, 3, 7}) == 5, "even median");
    check(LockFreeQueueTestResults::median({9, 1, 3}) == 3, "odd median");
    check(std::string(LockFreeQueueTestResults::rangeEvidence(1, 2, 2, 3, 4, 6)) == "single_sample", "one sample cannot establish spread");
    check(std::string(LockFreeQueueTestResults::rangeEvidence(3, 2, 4, 3, 4, 6)) == "overlapping_ranges", "touching sample ranges overlap");
    check(std::string(LockFreeQueueTestResults::rangeEvidence(3, 1, 3, 3, 4, 6)) == "separate_ranges", "lower sample range is separate");
    check(std::string(LockFreeQueueTestResults::rangeEvidence(3, 7, 9, 3, 4, 6)) == "separate_ranges", "higher sample range is separate");
    Json::Value sample;
    sample["items"] = 100;
    sample["elapsed_ns"] = 100.0;
    Json::Value rates = LockFreeQueueTestResults::rates(sample, 100);
    check(rates["rate_per_second"].asDouble() == 1e9 && rates["complete_rate_per_second"].asDouble() == 5e8, "separate maintenance cost");
    const double invalidCosts[] = {-1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()};
    for (size_t i = 0; i < 3; ++i)
    {
        bool refused = false;
        try
        {
            LockFreeQueueTestResults::rates(sample, invalidCosts[i]);
        }
        catch (const std::exception&)
        {
            refused = true;
        }
        check(refused, "invalid extra maintenance duration rejected");
    }
    Json::Value core;
    core["kind"] = "core";
    core["ids"].append(1);
    check(LockFreeQueueTestResults::parse("PASS id=1\n", core, 0)["samples"].empty(), "exact core PASS accepted");
    const char* badCore[] = {"PASS id=1.5\n", "PASS id=0\n", "PASS id=51\n", "PASS id=2\n", "PASS\n", "PASS id=1\nPASS id=1\n", "PASS id=1\nPASS\n", ""};
    for (size_t i = 0; i < sizeof(badCore) / sizeof(badCore[0]); ++i)
    {
        bool refused = false;
        try
        {
            LockFreeQueueTestResults::parse(badCore[i], core, 0);
        }
        catch (const std::exception&)
        {
            refused = true;
        }
        check(refused, "fractional/wrong/missing/duplicate core ID rejected");
    }
    core["require_summary"] = true;
    const std::string goodCore = "PASS id=1\nSUMMARY executed=1 failed=0\n";
    check(LockFreeQueueTestResults::parse(goodCore, core, 0)["samples"].empty(), "complete core summary accepted");
    const std::string badSummaries[] = {"PASS id=1\n", goodCore + "SUMMARY executed=1 failed=0\n",
        "PASS id=1\nSUMMARY executed=2 failed=0\n", "PASS id=1\nSUMMARY executed=1 failed=1\n"};
    for (size_t i = 0; i < sizeof(badSummaries) / sizeof(badSummaries[0]); ++i)
    {
        bool refused = false;
        try
        {
            LockFreeQueueTestResults::parse(badSummaries[i], core, 0);
        }
        catch (const std::exception&)
        {
            refused = true;
        }
        check(refused, "missing, duplicate or contradictory core summary rejected");
    }
    Json::Value backlog;
    backlog["kind"] = "core";
    backlog["ids"].append(22);
    std::string backlogRows;
    const char* backlogCounts[] = {"1", "1024", "65536", "262144"};
    for (size_t i = 0; i < 4; ++i)
    {
        backlogRows += std::string("BACKLOG count=") + backlogCounts[i] + " first_pop_ns=1 valid=1\n" +
            "LATENCY operation=pop_backlog samples=" + backlogCounts[i] + " p50_ns=1 p95_ns=1 p99_ns=1 max_ns=1\n";
    }
    const std::string backlogPass = "PASS id=22\n";
    const Json::Value backlogData = LockFreeQueueTestResults::parse(backlogRows + backlogPass, backlog, 0);
    check(backlogData["latencies"].size() == 4 && backlogData["samples"][0u]["case"].asString() == "backlog1" &&
        backlogData["latencies"][3u]["case"].asString() == "backlog262144",
        "all four backlog distributions recorded");
    const std::string badBacklogs[] = {"", backlogRows + backlogRows,
        replaceOnce(backlogRows, "samples=1024 ", "samples=1 "),
        replaceOnce(backlogRows, "count=65536 ", "count=1024 "),
        replaceOnce(backlogRows, "samples=262144 p50_ns=1", "samples=262144 p50_ns=2"),
        replaceOnce(backlogRows, "samples=1 p50_ns=1", "samples=1 p50_ns=0"),
        replaceOnce(backlogRows, "samples=65536 p50_ns=1", "samples=65536")};
    for (size_t i = 0; i < sizeof(badBacklogs) / sizeof(badBacklogs[0]); ++i)
    {
        bool refused = false;
        try
        {
            LockFreeQueueTestResults::parse(badBacklogs[i] + backlogPass, backlog, 0);
        }
        catch (const std::exception&)
        {
            refused = true;
        }
        check(refused, "incomplete, duplicate or contradictory backlog evidence rejected");
    }
    Json::Value notification;
    notification["kind"] = "core";
    notification["ids"].append(9);
    const std::string wakeCounts = "NOTIFICATION received=6000 wake_permits=191 coalesced_batch=32 finite_wait_ms=5 valid=1\n";
    const std::string wakePass = "PASS id=9\n";
    check(LockFreeQueueTestResults::parse(wakeCounts + wakePass, notification, 0)["samples"].empty(),
        "all notification permits accounted for");
    const std::string badWakes[] = {wakePass, wakeCounts + wakeCounts + wakePass,
        replaceOnce(wakeCounts, "191", "189") + wakePass, replaceOnce(wakeCounts, "191", "192") + wakePass,
        replaceOnce(wakeCounts, "6000", "5999") + wakePass};
    for (size_t i = 0; i < sizeof(badWakes) / sizeof(badWakes[0]); ++i)
    {
        bool refused = false;
        try
        {
            LockFreeQueueTestResults::parse(badWakes[i], notification, 0);
        }
        catch (const std::exception&)
        {
            refused = true;
        }
        check(refused, "missing, surplus or inconsistent notification data rejected");
    }
    Json::Value evidence;
    const std::string module = "PASS module_consumer engines=2 cross_module_inline=1 producer_context=1 pointer_bytes=8\n";
    const std::string demo = "LockFreeQueue usage demo passed\n";
    for (size_t target = 0; target < 2; ++target)
    {
        const std::string marker = target == 0 ? demo : module;
        evidence["kind"] = target == 0 ? "demo" : "module";
        evidence["pointer_bytes"] = 8;
        LockFreeQueueTestResults::parse(marker, evidence, 0);
        const std::string invalid[] = {"", marker + marker, target == 0 ? module : demo, marker};
        for (size_t i = 0; i < 4; ++i)
        {
            bool refused = false;
            try
            {
                LockFreeQueueTestResults::parse(invalid[i], evidence, i == 3 ? 1 : 0);
            }
            catch (const std::exception&)
            {
                refused = true;
            }
            check(refused, "missing/wrong/duplicate marker or child failure rejected");
        }
    }
    evidence["pointer_bytes"] = 4;
    bool wrongWidth = false;
    try
    {
        LockFreeQueueTestResults::parse(module, evidence, 0);
    }
    catch (const std::exception&)
    {
        wrongWidth = true;
    }
    check(wrongWidth, "module CPU width mismatch rejected");
    Json::Value invalidPassMeta;
    invalidPassMeta["kind"] = "performance";
    invalidPassMeta["suite"] = "38";
    const char* invalidPasses[] = {"PASS id=1e100\n", "PASS id=-1e100\n", "PASS id=38.5\n"};
    for (size_t i = 0; i < sizeof(invalidPasses) / sizeof(invalidPasses[0]); ++i)
    {
        bool refused = false;
        try
        {
            LockFreeQueueTestResults::parse(invalidPasses[i], invalidPassMeta, 0);
        }
        catch (const std::exception&)
        {
            refused = true;
        }
        check(refused, "benchmark PASS integer checked before narrowing");
    }
    Json::Value meta;
    meta["kind"] = "shared-ptr";
    meta["items"] = 16.0;
    meta["producers"] = 1.0;
    const std::string sharedMetrics = " case=46 status=0 items=16 producers=1 producer_seconds=2 consumer_seconds=1 producer_items_per_second=8 consumer_items_per_second=16 pool_allocations=0 pool_grows=0 valid=1\n";
    meta["mode"] = "plain";
    const std::string setup = "SETUP case=46 items=16 producers=1 capacity=17 bound=0 prefix=0 distributed=0 reserved=0\n";
    std::string rows = setup + "RESULT phase=separate" + sharedMetrics + "RESULT phase=concurrent" + sharedMetrics;
    const std::string incomplete = rows;
    const char* sharedPhases[] = {"separate", "concurrent"};
    for (size_t i = 0; i < 2; ++i)
    {
        const std::string lane = std::string("LANE phase=") + sharedPhases[i];
        rows += lane + " index=0 consumer=0 cpu_milliseconds=0\n" + lane + " index=1 consumer=1 cpu_milliseconds=2\n";
    }
    const Json::Value parsedShared = LockFreeQueueTestResults::parse(rows, meta, 0);
    check(parsedShared["samples"].size() == 2 && parsedShared["lanes"].size() == 4, "both phases and every lane");
    const char* modes[] = {"bound", "prefix", "distributed", "reserved", "distributed_pop"};
    const char* settings[] = {"1 prefix=0 distributed=0 reserved=0", "1 prefix=1 distributed=0 reserved=0",
        "1 prefix=1 distributed=1 reserved=0", "1 prefix=1 distributed=1 reserved=1", "1 prefix=0 distributed=1 reserved=0"};
    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i)
    {
        Json::Value requested = meta;
        requested["mode"] = modes[i];
        const std::string actual = replaceOnce(rows, "bound=0 prefix=0 distributed=0 reserved=0",
            std::string("bound=") + settings[i]);
        check(LockFreeQueueTestResults::parse(actual, requested, 0)["samples"].size() == 2, "shared-ptr mode accepted");
    }
    const std::string invalidLanes[] = {rows.substr(setup.size()), setup + rows,
        replaceOnce(rows, "phase=separate case=46", "phase=separate case=45"), replaceOnce(rows, "phase=separate case=46 status=0", "phase=separate case=46 status=-1"),
        replaceOnce(rows, "bound=0", "bound=1"), replaceOnce(rows, "prefix=0", "prefix=1"),
        replaceOnce(rows, "distributed=0", "distributed=1"), replaceOnce(rows, "reserved=0", "reserved=1"),
        replaceOnce(rows, "capacity=17", "capacity=16"), incomplete, rows + "LANE phase=separate index=0 consumer=0 cpu_milliseconds=0\n",
        replaceOnce(rows, "phase=separate index=1 consumer=1", "phase=separate index=1 consumer=0")};
    for (size_t i = 0; i < sizeof(invalidLanes) / sizeof(invalidLanes[0]); ++i)
    {
        bool refused = false;
        try
        {
            LockFreeQueueTestResults::parse(invalidLanes[i], meta, 0);
        }
        catch (const std::exception&)
        {
            refused = true;
        }
        check(refused, "wrong shared-ptr setup, result or lane rejected");
    }
    bool rejected = false;
    try
    {
        LockFreeQueueTestResults::parse(replaceOnce(rows, "RESULT phase=separate" + sharedMetrics, ""), meta, 0);
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    check(rejected, "missing shared-ptr phase rejected");
    const char* invalidMetrics[] = {"producer_seconds=0", "producer_seconds=-1", "producer_seconds=bad", "producer_seconds=1"};
    for (size_t i = 0; i < sizeof(invalidMetrics) / sizeof(invalidMetrics[0]); ++i)
    {
        std::string broken = rows;
        broken.replace(broken.find("producer_seconds=2"), 18, invalidMetrics[i]);
        rejected = false;
        try
        {
            LockFreeQueueTestResults::parse(broken, meta, 0);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        check(rejected, "invalid or inconsistent shared-ptr time rejected");
    }
    rejected = false;
    try
    {
        LockFreeQueueTestResults::parse("RESULT phase=separate items=16 producers=1 pool_allocations=0 pool_grows=0 valid=1\n"
            "RESULT phase=concurrent items=16 producers=1 pool_allocations=0 pool_grows=0 valid=1\n", meta, 0);
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    check(rejected, "missing shared-ptr metrics rejected");
    rejected = false;
    try
    {
        replaceOnce("same same", "same", "new");
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    check(rejected, "ambiguous patch rejected");
    rejected = false;
    try
    {
        inside(source(), join(root(), "src/temp/LockFreeQueueTest"));
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    check(rejected, "production output rejected");
    // The fixture counts are independently fixed by the retained workload contracts.
    Json::Value perf;
    perf["kind"] = "performance";
    perf["suite"] = "35";
    perf["version"] = "push";
    perf["profile"] = "fixed";
    perf["reference"] = false;
    perf["counted"] = true;
    perf["expected_variant"] = 1;
    perf["expected_profile"] = 0;
    const std::string bench = "BENCH variant=1 profile=0 pure_items=4000000 concurrent_items=1000000\n";
    std::string sufficient = bench;
    for (int32_t p = 1; p <= 16; p *= 2)
    {
        sufficient += "SCENARIO phase=producer producers=" + integer(p) + "\nREGISTERED_PRODUCERS producers=" + integer(p) + " items=4000000 elapsed_ns=100 producer_elapsed_ns=100 items_per_second=4e13 allocations=0 valid=1\nPOOL_VERIFY allocation_delta=0 grow_delta=0\n";
        sufficient += "SCENARIO phase=mpsc producers=" + integer(p) + "\nREGISTERED_MPSC producers=" + integer(p) + " items=1000000 elapsed_ns=100 producer_elapsed_ns=110 items_per_second=1e13 allocations=0 valid=1\nPOOL_VERIFY allocation_delta=0 grow_delta=0\n";
    }
    sufficient += "PASS id=35\n";
    check(LockFreeQueueTestResults::parse(sufficient, perf, 0)["samples"].size() == 10, "sufficient pool row/verification contract");
    std::vector<std::string> badLogs;
    badLogs.push_back(sufficient.substr(0, sufficient.rfind("PASS")));
    badLogs.push_back(sufficient + "POOL_VERIFY allocation_delta=0 grow_delta=0\n");
    std::string allocationFailure = sufficient;
    allocationFailure.replace(allocationFailure.find("allocations=0"), 13, "allocations=1");
    badLogs.push_back(allocationFailure);
    badLogs.push_back(replaceOnce(sufficient, "REGISTERED_MPSC producers=1 items=1000000 elapsed_ns=100 producer_elapsed_ns=110 items_per_second=1e13",
        "REGISTERED_MPSC producers=1 items=1000000 elapsed_ns=100 producer_elapsed_ns=110 items_per_second=2e13"));
    badLogs.push_back(replaceOnce(sufficient, "REGISTERED_MPSC producers=1 items=1000000 elapsed_ns=100 producer_elapsed_ns=110 items_per_second=1e13",
        "REGISTERED_MPSC producers=1 items=1000000 elapsed_ns=100 producer_elapsed_ns=0 items_per_second=1e13"));
    badLogs.push_back(replaceOnce(sufficient, "REGISTERED_PRODUCERS producers=1 items=4000000 elapsed_ns=100 producer_elapsed_ns=100 items_per_second=4e13",
        "REGISTERED_PRODUCERS producers=1 items=4000000 elapsed_ns=100 producer_elapsed_ns=200 items_per_second=4e13"));
    badLogs.push_back(sufficient.substr(bench.size()));
    badLogs.push_back(bench + sufficient);
    badLogs.push_back(replaceOnce(sufficient, "BENCH variant=1", "BENCH variant=3"));
    badLogs.push_back(replaceOnce(sufficient, "profile=0", "profile=1"));
    badLogs.push_back(replaceOnce(sufficient, "pure_items=4000000", "pure_items=400000"));
    badLogs.push_back(replaceOnce(sufficient, "concurrent_items=1000000", "concurrent_items=100000"));
    badLogs.push_back(replaceOnce(sufficient, "REGISTERED_MPSC producers=16 ", "REGISTERED_PRODUCERS producers=16 "));
    badLogs.push_back(replaceOnce(sufficient, "REGISTERED_PRODUCERS producers=8 ", "REGISTERED_MPSC producers=8 "));
    badLogs.push_back(replaceOnce(sufficient, "REGISTERED_MPSC producers=4 ", "REGISTERED_MPSC producers=2 "));
    badLogs.push_back(replaceOnce(sufficient, "REGISTERED_PRODUCERS producers=2 ", "REGISTERED_PRODUCERS "));
    badLogs.push_back(replaceOnce(sufficient, "REGISTERED_MPSC producers=1 ", "POLICY_RESULT producers=1 "));
    const char* brokenBefore[] = {"producers=16", " items=4000000", "elapsed_ns=100", "elapsed_ns=100", "id=35", "producers=1\n"};
    const char* brokenAfter[] = {"producers=1", " items=3999999", "missing=100", "elapsed_ns=0", "id=38", "producers=1.5\n"};
    for (size_t i = 0; i < 6; ++i)
    {
        std::string broken = sufficient;
        broken.replace(broken.find(brokenBefore[i]), std::string(brokenBefore[i]).size(), brokenAfter[i]);
        badLogs.push_back(broken);
    }
    for (size_t i = 0; i < badLogs.size(); ++i)
    {
        rejected = false;
        try
        {
            LockFreeQueueTestResults::parse(badLogs[i], perf, 0);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        check(rejected, ("invalid registered benchmark fixture " + integer(i)).c_str());
    }
    Json::Value poolMeta = perf;
    poolMeta["suite"] = "38";
    std::string poolLog = bench;
    const char* poolCases[] = {"default_zero", "default_positive", "finite_fixed_zero", "finite_fixed_positive",
        "finite_proportional", "shrink_regrow_zero", "shrink_regrow_positive"};
    for (size_t kind = 0; kind < 7; ++kind)
    {
        for (int32_t cycle = 0; cycle < (kind >= 5 ? 8 : 4); ++cycle)
        {
            poolLog += std::string("POLICY case=") + poolCases[kind] + " phase=" +
                (kind >= 5 || cycle == 0 ? "growth" : "reuse") + " cycle=" + integer(cycle) + "\n";
            poolLog += std::string("POLICY_RESULT operations=") + (kind < 2 ? "32768" : "4095") + " elapsed_ns=100 valid=1\n";
            poolLog += "MAINTENANCE phase=reclaim elapsed_ns=10 valid=1\n";
            if (kind >= 5)
            {
                poolLog += "MAINTENANCE phase=shrink elapsed_ns=20 valid=1\n";
            }
        }
    }
    for (int32_t threshold = 0; threshold < 2; ++threshold)
    {
        for (int32_t producers = 1; producers <= 16; producers *= 4)
        {
            poolLog += "POLICY case=bounded" + integer(threshold) + "_p" + integer(producers) + " phase=pressure cycle=0\n"
                "MAINTENANCE phase=reclaim elapsed_ns=10 valid=1\n"
                "POLICY_RESULT operations=200000 elapsed_ns=100 valid=1\n";
        }
    }
    poolLog += "PASS id=38\n";
    check(LockFreeQueueTestResults::parse(poolLog, poolMeta, 0)["maintenance"].size() == 58, "every reclaim and shrink cost retained");
    const std::string reclaimLine = "MAINTENANCE phase=reclaim elapsed_ns=10 valid=1\n";
    for (size_t mutation = 0; mutation < 2; ++mutation)
    {
        std::string broken = poolLog;
        broken.replace(broken.find(reclaimLine), reclaimLine.size(), mutation ? reclaimLine + reclaimLine : "");
        rejected = false;
        try
        {
            LockFreeQueueTestResults::parse(broken, poolMeta, 0);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        check(rejected, "omitted or duplicated maintenance cannot change complete throughput silently");
    }
    Json::Value waveMeta = perf;
    waveMeta["suite"] = "40";
    std::string waveLog = bench;
    for (int32_t threshold = 0; threshold <= 8; threshold += 8)
    {
        for (int32_t producers = 1; producers <= 16; producers *= 2)
        {
            waveLog += "WAVE case=wave750_t" + integer(threshold) + "_p" + integer(producers) + "\n";
            for (int32_t cycle = 0; cycle < 4; ++cycle)
            {
                for (int32_t phase = 0; phase < 3; ++phase)
                {
                    waveLog += "WAVE_STAGE cycle=" + integer(cycle) + " phase=" + integer(phase) +
                        " backlog=" + integer(phase == 0 ? 1000 : 500) + " capacity=1134\n";
                }
            }
            waveLog += "WAVE_RESULT operations=1182148 elapsed_ns=100 valid=1\n";
        }
    }
    waveLog += "PASS id=40\n";
    const Json::Value wave = LockFreeQueueTestResults::parse(waveLog, waveMeta, 0);
    check(wave["pools"].size() == 120 && wave["samples"].size() == 10, "all wave stages retained");
    for (Json::Value::UInt stage = 0; stage < 120; ++stage)
    {
        check(!wave["pools"][stage]["cycle"].isNull() &&
            wave["pools"][stage]["cycle"].asUInt() == stage % 12 / 3 &&
            wave["pools"][stage]["phase"].asUInt() == stage % 3,
            "wave export preserves the original cycle and phase");
    }
    const char* waveBefore[] = {"backlog=1000", "capacity=1134", "cycle=0 phase=1", "cycle=0 phase=0"};
    const char* waveAfter[] = {"backlog=1000.5", "capacity=1134.5", "cycle=0 phase=0", "cycle=0.5 phase=0"};
    for (size_t mutation = 0; mutation < 4; ++mutation)
    {
        std::string broken = waveLog;
        broken.replace(broken.find(waveBefore[mutation]), std::string(waveBefore[mutation]).size(), waveAfter[mutation]);
        rejected = false;
        try
        {
            LockFreeQueueTestResults::parse(broken, waveMeta, 0);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        check(rejected, "fractional wave counts and duplicate stages rejected");
    }
    Json::Value extra = perf;
    extra["suite"] = "extra";
    std::string extraLog = bench + "BURST width=1 items=2000000 elapsed_ns=100 valid=1\n"
        "BURST width=4096 items=1048576 elapsed_ns=100 valid=1\n"
        "BURST width=65536 items=1048576 elapsed_ns=100 valid=1\n";
    const char* observers[] = {"size_empty", "empty_empty", "pop_empty", "size_one", "push_pop_empty_cycle"};
    const int32_t highs[] = {1, 16, 128};
    for (size_t high = 0; high < 3; ++high)
    {
        extraLog += "OBSERVER record_high_water=" + integer(highs[high]) + "\n";
        for (size_t operation = 0; operation < 5; ++operation)
        {
            extraLog += std::string("EXTRA operation=") + observers[operation] + " operations=" +
                (operation == 4 ? "10000" : "20000000") + " elapsed_ns=100 valid=1\n";
        }
    }
    const char* extras[] = {"default_construct_destroy", "configured_destroy_init", "clear_65536", "destroy_65536",
        "full_pool_failure", "context_acquire_release", "payload_inline256", "payload_string64", "payload_string1024",
        "payload_string16384", "payload_shared256kb"};
    const int32_t counts[] = {2000, 2000, 65536, 65536, 10000, 5000000, 262144, 262144, 131072, 8192, 262144};
    for (size_t operation = 0; operation < 11; ++operation)
    {
        extraLog += std::string("EXTRA operation=") + extras[operation] + " operations=" + integer(counts[operation]) +
            " elapsed_ns=100 valid=1\n";
    }
    extraLog += "PASS suite=extra\n";
    check(LockFreeQueueTestResults::parse(extraLog, extra, 0)["samples"].size() == 29, "extended short-operation windows");
    const char* oldWindows[] = {"operations=20000000", "operations=5000000"};
    for (size_t window = 0; window < 2; ++window)
    {
        std::string broken = extraLog;
        broken.replace(broken.find(oldWindows[window]), std::string(oldWindows[window]).size(), "operations=1000000");
        rejected = false;
        try
        {
            LockFreeQueueTestResults::parse(broken, extra, 0);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        check(rejected, "old short-operation window cannot enter new result batch");
    }
    Json::Value schedule;
    schedule["kind"] = "scheduling";
    schedule["producers"] = 1.0;
    schedule["duration_ms"] = 2000;
    const std::string lanes = "LANE index=0 consumer=0 priority=0 count=1 active_count=1 active_attempts=1 cycles=1 sampled_cpu_gap_ms=1800\n"
        "LANE index=1 consumer=1 priority=0 count=1 active_count=1 active_attempts=1 cycles=1 sampled_cpu_gap_ms=10\n";
    const std::string noProgress = lanes + "SCHEDULE producers=1 capacity=65536 items=1 consumed=1 max_cpu_gap_ms=500 data_valid=1 same_priority=1 progress_valid=0 valid=0\n";
    schedule["version"] = "current";
    rejected = false;
    try
    {
        LockFreeQueueTestResults::parse(noProgress, schedule, 3);
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    check(rejected, "candidate no-progress is failure");
    rejected = false;
    try
    {
        LockFreeQueueTestResults::parse(lanes + "SCHEDULE producers=1 capacity=65536 items=1 consumed=1 max_cpu_gap_ms=500 data_valid=1 same_priority=1 progress_valid=1 valid=1\n", schedule, 0);
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    check(rejected, "long lane stall cannot claim fairness success");
    schedule["negative_control"] = true;
    const std::string controlled = noProgress.substr(0, noProgress.size() - 1) + " negative_control=1\n";
    check(LockFreeQueueTestResults::parse(controlled, schedule, 3)["samples"].size() == 1, "explicit stall negative control accepted");
    schedule["negative_control"] = false;
    const std::string waitingLanes = "LANE index=0 consumer=0 priority=0 count=1 active_count=0 active_attempts=0 cycles=1 sampled_cpu_gap_ms=10\n"
        "LANE index=1 consumer=1 priority=0 count=1 active_count=1 active_attempts=1 cycles=1 sampled_cpu_gap_ms=10\n";
    check(LockFreeQueueTestResults::parse(waitingLanes +
        "SCHEDULE producers=1 mode=7 capacity=256 items=1 consumed=1 max_cpu_gap_ms=500 data_valid=1 same_priority=1 progress_valid=1 valid=1\n",
        schedule, 0)["samples"].size() == 1, "extended ordinary push may finish its capacity wait during drain");
    Json::Value configured = schedule;
    configured["mode"] = 7;
    configured["capacity"] = 256;
    configured["cancelable"] = 0;
    const std::string exactSchedule = waitingLanes +
        "SCHEDULE producers=1 mode=7 capacity=256 cancelable=0 items=1 consumed=1 max_cpu_gap_ms=500 data_valid=1 same_priority=1 progress_valid=1 valid=1\n";
    check(LockFreeQueueTestResults::parse(exactSchedule, configured, 0)["samples"].size() == 1, "scheduling actual configuration matches request");
    const char* beforeFields[] = {"producers=1", "mode=7", "capacity=256", "cancelable=0", "index=0", "consumer=0", "active_count=0", "priority=0", "items=1", "consumed=1", "cycles=1"};
    const char* afterFields[] = {"producers=2", "mode=0", "capacity=65536", "cancelable=1", "index=0.5", "consumer=0.5", "active_count=2", "priority=1", "items=2", "consumed=2", "cycles=1.5"};
    for (size_t i = 0; i < sizeof(beforeFields) / sizeof(beforeFields[0]); ++i)
    {
        std::string broken = exactSchedule;
        broken.replace(broken.find(beforeFields[i]), std::string(beforeFields[i]).size(), afterFields[i]);
        rejected = false;
        try
        {
            LockFreeQueueTestResults::parse(broken, configured, 0);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        check(rejected, "mismatched configuration/fractional lane/impossible active count rejected");
    }
    const char* nonwaitingModes[] = {"2", "6"};
    for (size_t i = 0; i < 2; ++i)
    {
        rejected = false;
        try
        {
            LockFreeQueueTestResults::parse(waitingLanes + "SCHEDULE producers=1 mode=" + nonwaitingModes[i] +
                " capacity=256 items=1 consumed=1 max_cpu_gap_ms=500 data_valid=1 same_priority=1 progress_valid=1 valid=1\n", schedule, 0);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        check(rejected, "nonwaiting API still requires an active-window returned call");
    }
    rejected = false;
    try
    {
        LockFreeQueueTestResults::fields("ROW elapsed_ns=NaN");
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    check(rejected, "nonfinite evidence rejected");
    rejected = false;
    try
    {
        LockFreeQueueTestResults::fields("ROW valid=1 valid=0");
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    check(rejected, "duplicate evidence field rejected");
    std::string destination = batch();
    directory(destination);
    Json::Value manifest = LockFreeQueueTestWhitebox::prepare(*this, source(), join(destination, "whitebox"));
    check(manifest["files"].size() == sources(source()).size(), "all production files instrumented");
    write(join(destination, "io.bin"), std::string("a\0b\r\n", 5));
    check(read(join(destination, "io.bin")) == std::string("a\0b\r\n", 5), "binary file IO");
#ifdef _WIN32
    std::string timeoutLog = "timeout-" + integer(std::chrono::steady_clock::now().time_since_epoch().count()) + ".raw.log";
    Json::Value child = process({CSystem::currentExecutablePathUtf8(), "self-test-child"}, destination, join(destination, timeoutLog), 1);
    check(child["exit"].asInt() == 124 && child["timeout"].asBool(), "child deadline/cleanup");
    check(child["launch_cwd"].asString() == path(parent(executable())), "actual child launch directory");
    check(read(join(destination, timeoutLog)).find("CHILD_CWD=" + child["launch_cwd"].asString()) != std::string::npos,
        "child confirms launch directory independently of requested workload directory");
#endif
    std::cout << "SELF_TEST PASS checks=" << checks << std::endl;
}