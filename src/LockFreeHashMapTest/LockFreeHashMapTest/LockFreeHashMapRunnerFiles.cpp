#include "LockFreeHashMapRunner.h"
#include <fstream>
#if defined(_WIN32)
#include <Windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

bool LockFreeHashMapRunner::openReport(const std::string& path, std::ofstream& output)
{
    if (output.is_open())
    {
        return false;
    }
#if defined(_WIN32)
    HANDLE file = CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    if (!CloseHandle(file))
    {
        return false;
    }
#else
    int file = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (file < 0 || close(file) != 0)
    {
        return false;
    }
#endif
    output.open(path.c_str(), std::ios::out);
    return bool(output);
}