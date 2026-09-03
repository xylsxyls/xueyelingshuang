#include "LoopPlayerUtils.h"
#include "LoopPlayerConstants.h"
#include "LogManager/LogManagerAPI.h"

#include <stdarg.h>

namespace LoopPlayer
{
    // 当前进程是否启用调试日志
    static bool gLoggingEnabled = false;
    // LogManager是否已经完成初始化
    static bool gLogManagerInitialized = false;

    static std::wstring GetModuleDirectory();

    /** 判断宽字符字符串是否全部是数字
    @param [in] text 需要判断的字符串
    @return 全部是数字返回true，否则返回false
    */
    static bool IsWideDigitText(const std::wstring& text)
    {
        if (text.empty())
        {
            return false;
        }

        for (size_t i = 0; i < text.size(); ++i)
        {
            if (text[i] < L'0' || text[i] > L'9')
            {
                return false;
            }
        }
        return true;
    }

    /** 判断文件名是否是LogManager实体日志文件
    @param [in] fileName 文件名
    @return 是base.index.log格式返回true，否则返回false
    */
    static bool IsRollingEntityLogFileName(const std::wstring& fileName)
    {
        static const wchar_t kLogSuffix[] = L".log";
        const size_t suffixLength = ARRAYSIZE(kLogSuffix) - 1;
        if (fileName.size() <= suffixLength ||
            fileName.compare(fileName.size() - suffixLength, suffixLength, kLogSuffix) != 0)
        {
            return false;
        }

        const size_t indexEnd = fileName.size() - suffixLength;
        if (indexEnd == 0)
        {
            return false;
        }

        const size_t dotPos = fileName.rfind(L'.', indexEnd - 1);
        if (dotPos == std::string::npos || dotPos + 1 >= indexEnd)
        {
            return false;
        }

        return IsWideDigitText(fileName.substr(dotPos + 1, indexEnd - dotPos - 1));
    }

    /** 删除当前进程生成的LogManager当前日志链接，避免base.log和base.0.log同时显示
    */
    static void DeleteCurrentProcessLogLinkFiles()
    {
        const std::wstring logDir = GetModuleDirectory();
        const std::wstring findPattern = logDir + L"LoopPlayer*.log";

        wchar_t pidToken[64] = { 0 };
        StringCchPrintfW(pidToken, ARRAYSIZE(pidToken), L"_%lu_", GetCurrentProcessId());

        WIN32_FIND_DATAW findData = { 0 };
        HANDLE findHandle = FindFirstFileW(findPattern.c_str(), &findData);
        if (findHandle == INVALID_HANDLE_VALUE)
        {
            return;
        }

        do
        {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            {
                continue;
            }

            const std::wstring fileName = findData.cFileName;
            if (fileName.find(pidToken) == std::wstring::npos || IsRollingEntityLogFileName(fileName))
            {
                continue;
            }

            ULARGE_INTEGER fileSize = { 0 };
            fileSize.HighPart = findData.nFileSizeHigh;
            fileSize.LowPart = findData.nFileSizeLow;
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0 && fileSize.QuadPart != 0)
            {
                continue;
            }

            DeleteFileW((logDir + fileName).c_str());
        }
        while (FindNextFileW(findHandle, &findData));

        FindClose(findHandle);
    }

    /** 把宽字符文本转换成指定代码页的多字节文本
    @param [in] text 需要转换的宽字符文本
    @param [in] codePage 目标代码页
    @return 转换后的多字节文本
    */
    static std::string WideToMultiByteString(const std::wstring& text, UINT codePage)
    {
        if (text.empty())
        {
            return std::string();
        }

        const int byteCount = WideCharToMultiByte(codePage, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (byteCount <= 1)
        {
            return std::string();
        }

        std::string bytes;
        bytes.resize(byteCount - 1);
        WideCharToMultiByte(codePage, 0, text.c_str(), -1, &bytes[0], byteCount, nullptr, nullptr);
        return bytes;
    }

    /** 读取当前EXE所在目录，并保证末尾带反斜杠
    @return 返回EXE所在目录
    */
    static std::wstring GetModuleDirectory()
    {
        wchar_t path[MAX_PATH] = { 0 };
        DWORD length = GetModuleFileNameW(nullptr, path, ARRAYSIZE(path));
        if (length == 0 || length >= ARRAYSIZE(path))
        {
            return L".\\";
        }

        PathRemoveFileSpecW(path);
        PathAddBackslashW(path);
        return path;
    }

    std::wstring FormatTime(REFERENCE_TIME value)
    {
        if (value < 0)
        {
            return L"--:--.---";
        }

        const LONGLONG totalMs = value / 10000;
        const int ms = static_cast<int>(totalMs % 1000);
        const LONGLONG totalSeconds = totalMs / 1000;
        const int seconds = static_cast<int>(totalSeconds % 60);
        const int minutes = static_cast<int>((totalSeconds / 60) % 60);
        const LONGLONG hours = totalSeconds / 3600;

        wchar_t buffer[64] = { 0 };
        if (hours > 0)
        {
            StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"%I64d:%02d:%02d.%03d", hours, minutes, seconds, ms);
        }
        else
        {
            StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"%02d:%02d.%03d", minutes, seconds, ms);
        }

        return buffer;
    }

    std::wstring FormatClockTime(REFERENCE_TIME value)
    {
        if (value < 0)
        {
            return L"--:--:--";
        }

        const LONGLONG totalSeconds = value / ONE_SECOND;
        const int seconds = static_cast<int>(totalSeconds % 60);
        const int minutes = static_cast<int>((totalSeconds / 60) % 60);
        const LONGLONG hours = totalSeconds / 3600;

        wchar_t buffer[64] = { 0 };
        StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"%02I64d:%02d:%02d", hours, minutes, seconds);
        return buffer;
    }

    std::wstring HResultText(HRESULT hr)
    {
        wchar_t buffer[256] = { 0 };
        StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"Error 0x%08X", static_cast<unsigned int>(hr));
        return buffer;
    }

    std::wstring GetLogFilePath()
    {
        return GetModuleDirectory() + L"LoopPlayer*.log";
    }

    void SetLoggingEnabled(bool enabled)
    {
        gLoggingEnabled = enabled;
        if (!gLoggingEnabled && gLogManagerInitialized)
        {
            LogManager::instance().uninitAll();
            gLogManagerInitialized = false;
        }
    }

    bool IsLoggingEnabled()
    {
        return gLoggingEnabled;
    }

    void ResetLogFile()
    {
        if (!gLoggingEnabled)
        {
            return;
        }

        if (gLogManagerInitialized)
        {
            LogManager::instance().uninitAll();
            gLogManagerInitialized = false;
        }

        LogManagerConfig config;
        config.m_fileId = 0;
        config.m_path = WideToMultiByteString(GetModuleDirectory(), CP_ACP);
        config.m_maxFileBytes = 20LL * 1024LL * 1024LL;
        config.m_maxFileCount = 8;
        config.m_checkFileSizeInterval = 1;
        config.m_outputConsole = false;
        config.m_archiveOldLog = false;
        LogManager::instance().set(true, true);
        LogManager::instance().init(config);
        gLogManagerInitialized = true;
    }

    void ShutdownLog()
    {
        gLoggingEnabled = false;
        if (gLogManagerInitialized)
        {
            LogManager::instance().uninitAll();
            gLogManagerInitialized = false;
            DeleteCurrentProcessLogLinkFiles();
        }
    }

    void LogfImpl(const char* file, const char* function, const wchar_t* format, ...)
    {
        if (!gLoggingEnabled)
        {
            return;
        }

        if (!gLogManagerInitialized)
        {
            ResetLogFile();
        }

        wchar_t message[4096] = { 0 };

        va_list args;
        va_start(args, format);
        StringCchVPrintfW(message, ARRAYSIZE(message), format, args);
        va_end(args);

        if (!gLogManagerInitialized)
        {
            return;
        }

        // LogManager按窄字节文本落盘，这里跟随Windows本地代码页，避免中文日志在常见工具里显示乱码。
        const std::string text = WideToMultiByteString(message, CP_ACP);
        LogManager::instance().print(0,
                                     LogManager::LOG_INFO,
                                     file ? file : "",
                                     function ? function : "",
                                     "",
                                     "",
                                     0,
                                     "%s",
                                     text.c_str());
    }

    void InitInt64PropVariant(PROPVARIANT& value, LONGLONG number)
    {
        PropVariantInit(&value);
        value.vt = VT_I8;
        value.hVal.QuadPart = number;
    }

    bool ReadInt64PropVariant(const PROPVARIANT& value, LONGLONG& number)
    {
        if (value.vt == VT_I8)
        {
            number = value.hVal.QuadPart;
            return true;
        }
        if (value.vt == VT_UI8)
        {
            number = static_cast<LONGLONG>(value.uhVal.QuadPart);
            return true;
        }
        return false;
    }
}