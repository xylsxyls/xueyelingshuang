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
    const std::set<std::string> flags = {"matrix", "consumers", "projects", "resume", "wave",
        "build-only", "prepare-only", "measure-only", "after-only", "candidate-only", "plan"};
    const std::set<std::string> values = {"root", "source", "output", "batch", "cases", "arch",
        "configuration", "repeat", "stress-seconds", "baseline-dir", "peer-snapshot-dir", "vcvars",
        "before-dir", "after-dir", "previous-dir", "calls", "rounds", "supplement-rounds", "git",
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
            permitted.insert({"cases", "configuration", "matrix", "consumers", "projects", "repeat", "stress-seconds", "baseline-dir", "peer-snapshot-dir", "git"});
        }
        else
        {
            permitted.insert("rounds");
            if (m_command == "performance" || m_command == "three-way")
            {
                permitted.insert({"baseline-dir", "supplement-rounds", "allocation-rounds", "suite", "profiles", "git"});
                if (m_command == "three-way")
                {
                    permitted.insert("wave");
                    permitted.insert("previous-dir");
                }
            }
            else
            {
                permitted.insert("before-dir");
                permitted.insert("after-dir");
                if (m_command == "revision")
                {
                    permitted.insert({"calls", "allocation-rounds", "suite", "profiles"});
                }
                else if (m_command == "scheduling")
                {
                    permitted.insert({"duration-ms", "masks", "modes", "capacities", "after-only", "producers"});
                }
                else if (m_command == "shared-ptr")
                {
                    permitted.insert({"items", "producers", "modes", "candidate-only"});
                }
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
    result["cwd"] = cwd;
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
        std::cout << "QueueTest C++ host: whitebox | tests | performance | three-way | revision | scheduling | shared-ptr | summarize | self-test\n"
            "All outputs: --batch NAME under src/temp/LockFreeQueueTest. Lists use commas.\n"
            "tests --cases all|instant|stress|1,3-8 --matrix --consumers --projects\n"
            "whitebox --source DIR --output PRIVATE_TEMP_DIR\n"
            "performance/three-way --baseline-dir DIR; revision/scheduling/shared-ptr --before-dir DIR\n"
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

void LockFreeQueueTestTool::selfTest() const
{
    int32_t checks = 0;
    // Each check has an independent expected value; failures return through main.
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
    std::vector<std::string> revisionArguments = {executable(), "revision", "--root", root(),
        "--before-dir", source(), "--after-dir", source(), "--batch", "parser-only"};
    std::vector<char*> revisionPointers;
    for (size_t i = 0; i < revisionArguments.size(); ++i)
    {
        revisionPointers.push_back(&revisionArguments[i][0]);
    }
    LockFreeQueueTestTool revisionTool(static_cast<int32_t>(revisionPointers.size()), revisionPointers.data());
    check(revisionTool.option("before-dir") == source() && revisionTool.option("after-dir") == source(), "revision CLI option pair");
    check(LockFreeQueueTestRunner::select("all").size() == 46, "registry count");
    check(LockFreeQueueTestRunner::select("instant").size() == 31, "instant registry");
    check(LockFreeQueueTestRunner::select("stress").size() == 15, "pressure registry");
    check(LockFreeQueueTestRunner::select("1,3-5,5").size() == 4, "range deduplication");
    const char* invalid[] = {"", "0", "47", "3-1", "1,,2", "1-2-3", "99999999999999", "x", "-1"};
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
    Json::Value sample;
    sample["items"] = 100;
    sample["elapsed_ns"] = 100.0;
    Json::Value rates = LockFreeQueueTestResults::rates(sample, 100);
    check(rates["rate_per_second"].asDouble() == 1e9 && rates["complete_rate_per_second"].asDouble() == 5e8, "separate maintenance cost");
    Json::Value meta;
    meta["kind"] = "shared-ptr";
    meta["items"] = 16.0;
    meta["producers"] = 1.0;
    std::string rows = "RESULT phase=separate items=16 producers=1 pool_allocations=0 pool_grows=0 valid=1\nRESULT phase=concurrent items=16 producers=1 pool_allocations=0 pool_grows=0 valid=1\n";
    check(LockFreeQueueTestResults::parse(rows, meta, 0)["samples"].size() == 2, "both shared-ptr phases");
    bool rejected = false;
    try
    {
        LockFreeQueueTestResults::parse(rows.substr(rows.find('\n') + 1), meta, 0);
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    check(rejected, "missing shared-ptr phase rejected");
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
    std::string sufficient;
    for (int32_t i = 0; i < 10; ++i)
    {
        sufficient += "SCENARIO phase=mpsc producers=" + integer(i + 1) + "\nREGISTERED_MPSC items=100 elapsed_ns=100 allocations=0 valid=1\nPOOL_VERIFY allocation_delta=0 grow_delta=0\n";
    }
    sufficient += "PASS id=35\n";
    check(LockFreeQueueTestResults::parse(sufficient, perf, 0)["samples"].size() == 10, "sufficient pool row/verification contract");
    std::vector<std::string> badLogs;
    badLogs.push_back(sufficient.substr(0, sufficient.rfind("PASS")));
    badLogs.push_back(sufficient + "POOL_VERIFY allocation_delta=0 grow_delta=0\n");
    std::string allocationFailure = sufficient;
    allocationFailure.replace(allocationFailure.find("allocations=0"), 13, "allocations=1");
    badLogs.push_back(allocationFailure);
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
        check(rejected, "missing PASS/extra pool window/timed allocation rejected");
    }
    Json::Value schedule;
    schedule["kind"] = "scheduling";
    schedule["version"] = "before";
    schedule["producers"] = 1.0;
    const std::string lanes = "LANE index=0 consumer=0 count=0\nLANE index=1 consumer=1 count=0\n";
    const std::string noProgress = lanes + "SCHEDULE data_valid=1 same_priority=1 progress_valid=0 valid=0\n";
    check(LockFreeQueueTestResults::parse(noProgress, schedule, 3)["lanes"].size() == 2, "baseline-only no-progress evidence");
    schedule["version"] = "after";
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
#endif
    std::cout << "SELF_TEST PASS checks=" << checks << std::endl;
}