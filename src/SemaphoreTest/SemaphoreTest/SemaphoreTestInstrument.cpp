#include "SemaphoreTestInstrument.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <utility>
#ifdef _WIN32
#include <Windows.h>
#else
#include <cstdlib>
#include <limits.h>
#include <sys/stat.h>
#endif

int32_t SemaphoreTestInstrument::run(const std::string& sourceDirectory, const std::string& outputDirectory)
{
    std::string sourcePath;
    std::string outputPath;
    if (!resolveDirectory(sourceDirectory, sourcePath) || !resolveDirectory(outputDirectory, outputPath) ||
        sourcePath == outputPath)
    {
        std::fprintf(stderr, "Instrumentation needs distinct existing source and output directories\n");
        return 1;
    }
    const char* files[] = {"Semaphore.cpp", "SemaphoreWake.cpp", "SemaphoreEventPool.cpp", "SemaphoreCount.cpp"};
    std::string generated[4];
    for (size_t i = 0; i < 4; ++i)
    {
        std::string source;
        if (!read(sourceDirectory + "/" + files[i], source) || !instrument(files[i], source, generated[i], true))
        {
            std::fprintf(stderr, "Cannot instrument %s\n", files[i]);
            return 1;
        }
        generated[i].insert(0, "#include \"SemaphoreTestHooks.h\"\n");
    }
    // 所有输入先验证；构建目标每次执行本工具，失败不会继续编译上次副本
    for (size_t i = 0; i < 4; ++i)
    {
        if (!write(outputDirectory + "/" + files[i], generated[i]))
        {
            std::fprintf(stderr, "Cannot write instrumented %s\n", files[i]);
            return 1;
        }
    }
    std::printf("Generated 4 instrumented sources from current production files\n");
    return 0;
}

int32_t SemaphoreTestInstrument::check(const std::string& sourceDirectory)
{
    size_t count = 0;
    const SemaphoreTestInjection* entries = rules(count);
    for (size_t i = 0; i < count; ++i)
    {
        std::string source;
        std::string result;
        if (!read(sourceDirectory + "/" + entries[i].file, source) || !instrument(entries[i].file, source, result, true))
        {
            return 1;
        }
        const size_t position = source.find(entries[i].context);
        std::string missing = source;
        missing.replace(position, std::strlen(entries[i].context), "changed production context\n");
        if (instrument(entries[i].file, missing, result, false) ||
            instrument(entries[i].file, source + entries[i].context, result, false))
        {
            std::fprintf(stderr, "Stale or ambiguous rule was accepted: %u\n", static_cast<unsigned>(i));
            return 1;
        }
    }
    std::printf("Checked %u rules: normal, missing and duplicate contexts\n", static_cast<unsigned>(count));
    return 0;
}

bool SemaphoreTestInstrument::read(const std::string& path, std::string& text)
{
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input)
    {
        return false;
    }
    std::string bytes;
    char buffer[4096];
    while (input.read(buffer, sizeof(buffer)) || input.gcount() != 0)
    {
        bytes.append(buffer, static_cast<size_t>(input.gcount()));
    }
    if (!input.eof() || input.bad())
    {
        return false;
    }
    if (bytes.compare(0, 3, "\xEF\xBB\xBF") == 0)
    {
        bytes.erase(0, 3);
    }
    std::string normalized;
    normalized.reserve(bytes.size());
    for (size_t i = 0; i < bytes.size(); ++i)
    {
        if (bytes[i] != '\r' || i + 1 == bytes.size() || bytes[i + 1] != '\n')
        {
            normalized += bytes[i];
        }
    }
    text.swap(normalized);
    return true;
}

bool SemaphoreTestInstrument::write(const std::string& path, const std::string& text)
{
    std::string previous;
    if (read(path, previous) && previous == text)
    {
        return true;
    }
    std::ofstream output(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!output)
    {
        return false;
    }
    output.write("\xEF\xBB\xBF", 3);
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '\n')
        {
            output.put('\r');
        }
        output.put(text[i]);
    }
    output.close();
    return !output.fail();
}

bool SemaphoreTestInstrument::instrument(const char* file, const std::string& source, std::string& result, bool report)
{
    size_t count = 0;
    const SemaphoreTestInjection* entries = rules(count);
    std::vector<std::pair<size_t, const char*>> positions;
    for (size_t i = 0; i < count; ++i)
    {
        if (std::strcmp(file, entries[i].file) != 0)
        {
            continue;
        }
        const size_t position = source.find(entries[i].context);
        if (entries[i].context[0] == 0 || entries[i].offset > std::strlen(entries[i].context) ||
            position == std::string::npos || source.find(entries[i].context, position + 1) != std::string::npos)
        {
            if (report)
            {
                std::fprintf(stderr, "Missing or ambiguous instrumentation context: %s rule %u\n",
                    file, static_cast<unsigned>(i));
            }
            return false;
        }
        positions.push_back(std::make_pair(position + entries[i].offset, entries[i].text));
    }
    if (positions.empty())
    {
        return false;
    }
    /** 按字节位置倒序插入，保证其他规则仍指向原始生产源码中的同一位置
    @param [in] left 左侧注入点
    @param [in] right 右侧注入点
    @return 左侧应先插入
    */
    std::sort(positions.begin(), positions.end(), [](const std::pair<size_t, const char*>& left,
        const std::pair<size_t, const char*>& right)
    {
        return left.first > right.first;
    });
    std::string generated = source;
    for (size_t i = 0; i < positions.size(); ++i)
    {
        if (i != 0 && positions[i].first == positions[i - 1].first)
        {
            return false;
        }
        generated.insert(positions[i].first, positions[i].second);
    }
    result.swap(generated);
    return true;
}

bool SemaphoreTestInstrument::resolveDirectory(const std::string& path, std::string& resolved)
{
#ifdef _WIN32
    HANDLE directory = ::CreateFileA(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (directory == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    char buffer[32768];
    const DWORD length = ::GetFinalPathNameByHandleA(directory, buffer, sizeof(buffer), FILE_NAME_NORMALIZED);
    const bool closed = ::CloseHandle(directory) != FALSE;
    if (!closed || length == 0 || length >= sizeof(buffer))
    {
        return false;
    }
    // 关闭句柄后才进行可能申请内存的字符串赋值；Windows路径按不区分大小写比较
    for (DWORD i = 0; i < length; ++i)
    {
        if (buffer[i] >= 'A' && buffer[i] <= 'Z')
        {
            buffer[i] += 'a' - 'A';
        }
    }
    resolved.assign(buffer, length);
#else
    char buffer[PATH_MAX];
    struct stat information;
    if (::realpath(path.c_str(), buffer) == nullptr || ::stat(buffer, &information) != 0 || !S_ISDIR(information.st_mode))
    {
        return false;
    }
    resolved = buffer;
#endif
    return true;
}

/** 测试专用源码生成工具；失败正常返回，不允许构建继续使用过期副本
@param [in] argc 参数数量
@param [in] argv 生产目录及临时目录，或--check加生产目录
@return 0成功，非0为错误
*/
int32_t main(int32_t argc, char** argv)
{
    try
    {
        if (argc == 3 && std::strcmp(argv[1], "--check") == 0)
        {
            return SemaphoreTestInstrument::check(argv[2]);
        }
        if (argc == 3)
        {
            return SemaphoreTestInstrument::run(argv[1], argv[2]);
        }
        std::fprintf(stderr, "Usage: SemaphoreTestInstrument <source-dir> <output-dir> | --check <source-dir>\n");
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "Instrumentation failed: %s\n", error.what());
    }
    catch (...)
    {
        std::fprintf(stderr, "Instrumentation failed with an unknown exception\n");
    }
    return 1;
}