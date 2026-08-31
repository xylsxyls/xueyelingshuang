#include "SplitViewerCommon.h"
#include "TempFileGuard.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

namespace SplitViewer
{


    int RectWidth(const RECT& rc)
    {
        return rc.right - rc.left;
    }



    int RectHeight(const RECT& rc)
    {
        return rc.bottom - rc.top;
    }



    RECT MakeRect(int left, int top, int right, int bottom)
    {
        RECT rc = { left, top, right, bottom };
        return rc;
    }



    bool IsRectUsable(const RECT& rc)
    {
        return RectWidth(rc) > 0 && RectHeight(rc) > 0;
    }



    bool AreRectsEqual(const RECT& left, const RECT& right)
    {
        return left.left == right.left &&
               left.top == right.top &&
               left.right == right.right &&
               left.bottom == right.bottom;
    }



    bool PointInRect(const RECT& rc, POINT pt)
    {
        return pt.x >= rc.left && pt.x < rc.right && pt.y >= rc.top && pt.y < rc.bottom;
    }



    RECT InflatedRect(RECT rc, int dx, int dy)
    {
        InflateRect(&rc, dx, dy);
        return rc;
    }



    double ClampDouble(double value, double low, double high)
    {
        if (value < low)
        {
            return low;
        }
        if (value > high)
        {
            return high;
        }
        return value;
    }



    int ClampInt(int value, int low, int high)
    {
        if (value < low)
        {
            return low;
        }
        if (value > high)
        {
            return high;
        }
        return value;
    }



    std::wstring FormatInt(int value)
    {
        wchar_t buffer[64] = { 0 };
        StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"%d", value);
        return buffer;
    }



    std::wstring FormatDouble(double value)
    {
        wchar_t buffer[128] = { 0 };
        StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"%.10f", value);
        return buffer;
    }



    bool SameText(const wchar_t* left, const wchar_t* right)
    {
        return _wcsicmp(left, right) == 0;
    }



    bool HasExtension(const std::wstring& path)
    {
        const wchar_t* ext = PathFindExtensionW(path.c_str());
        return ext && ext[0] != 0;
    }



    std::wstring MakeTimestampName()
    {
        SYSTEMTIME now = { 0 };
        GetLocalTime(&now);

        wchar_t buffer[64] = { 0 };
        StringCchPrintfW(buffer,
                         ARRAYSIZE(buffer),
                         L"%04u%02u%02u%02u%02u%02u%03u",
                         now.wYear,
                         now.wMonth,
                         now.wDay,
                         now.wHour,
                         now.wMinute,
                         now.wSecond,
                         now.wMilliseconds);
        return buffer;
    }



    std::wstring WithDefaultExtension(const std::wstring& path, const wchar_t* ext)
    {
        if (HasExtension(path))
        {
            return path;
        }

        std::wstring result = path;
        result += ext;
        return result;
    }



    bool IsConfigPath(const wchar_t* path)
    {
        if (!path || !path[0])
        {
            return false;
        }

        const wchar_t* ext = PathFindExtensionW(path);
        return ext && SameText(ext, L".sv");
    }



    int GetEncoderClsid(const WCHAR* format, CLSID* clsid)
    {
        UINT num = 0;
        UINT size = 0;
        Gdiplus::GetImageEncodersSize(&num, &size);
        if (size == 0)
        {
            return -1;
        }

        std::vector<BYTE> buffer(size);
        Gdiplus::ImageCodecInfo* codecInfo = reinterpret_cast<Gdiplus::ImageCodecInfo*>(&buffer[0]);
        Gdiplus::GetImageEncoders(num, size, codecInfo);

        for (UINT i = 0; i < num; ++i)
        {
            if (wcscmp(codecInfo[i].MimeType, format) == 0)
            {
                *clsid = codecInfo[i].Clsid;
                return static_cast<int>(i);
            }
        }

        return -1;
    }



    const WCHAR* ImageMimeForPath(const std::wstring& path)
    {
        const wchar_t* ext = PathFindExtensionW(path.c_str());
        if (ext && (SameText(ext, L".jpg") || SameText(ext, L".jpeg")))
        {
            return L"image/jpeg";
        }
        if (ext && SameText(ext, L".bmp"))
        {
            return L"image/bmp";
        }
        if (ext && SameText(ext, L".gif"))
        {
            return L"image/gif";
        }
        if (ext && (SameText(ext, L".tif") || SameText(ext, L".tiff")))
        {
            return L"image/tiff";
        }
        return L"image/png";
    }



    std::wstring ProfileSectionName(const std::wstring& prefix, int id)
    {
        std::wstring section = prefix;
        section += L"Node";
        section += FormatInt(id);
        return section;
    }



    void WriteProfileString(const std::wstring& file, const std::wstring& section, const wchar_t* key, const std::wstring& value)
    {
        WritePrivateProfileStringW(section.c_str(), key, value.c_str(), file.c_str());
    }



    void WriteProfileInt(const std::wstring& file, const std::wstring& section, const wchar_t* key, int value)
    {
        WriteProfileString(file, section, key, FormatInt(value));
    }



    void WriteProfileDouble(const std::wstring& file, const std::wstring& section, const wchar_t* key, double value)
    {
        WriteProfileString(file, section, key, FormatDouble(value));
    }



    std::wstring ReadProfileString(const std::wstring& file, const std::wstring& section, const wchar_t* key, const wchar_t* defaultValue)
    {
        wchar_t buffer[8192] = { 0 };
        GetPrivateProfileStringW(section.c_str(), key, defaultValue, buffer, ARRAYSIZE(buffer), file.c_str());
        return buffer;
    }



    int ReadProfileIntValue(const std::wstring& file, const std::wstring& section, const wchar_t* key, int defaultValue)
    {
        return GetPrivateProfileIntW(section.c_str(), key, defaultValue, file.c_str());
    }



    double ReadProfileDoubleValue(const std::wstring& file, const std::wstring& section, const wchar_t* key, double defaultValue)
    {
        std::wstring text = ReadProfileString(file, section, key, L"");
        if (text.empty())
        {
            return defaultValue;
        }
        return _wtof(text.c_str());
    }



    bool CreateUtf16ProfileFile(const std::wstring& file)
    {
        HANDLE handle = CreateFileW(file.c_str(),
                                    GENERIC_WRITE,
                                    0,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
        if (handle == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        const WORD bom = 0xFEFF;
        DWORD written = 0;
        const BOOL ok = WriteFile(handle, &bom, sizeof(bom), &written, NULL);
        CloseHandle(handle);
        return ok && written == sizeof(bom);
    }



    bool ReadFileBytes(const std::wstring& file, std::vector<BYTE>& bytes)
    {
        bytes.clear();
        HANDLE handle = CreateFileW(file.c_str(),
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
        if (handle == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        LARGE_INTEGER size = { 0 };
        if (!GetFileSizeEx(handle, &size) || size.QuadPart < 0 || size.QuadPart > 64 * 1024 * 1024)
        {
            CloseHandle(handle);
            return false;
        }

        bytes.resize(static_cast<size_t>(size.QuadPart));
        DWORD read = 0;
        bool ok = true;
        if (!bytes.empty())
        {
            ok = ReadFile(handle, &bytes[0], static_cast<DWORD>(bytes.size()), &read, NULL) != FALSE &&
                 read == bytes.size();
        }
        CloseHandle(handle);
        return ok;
    }



    bool WriteFileBytes(const std::wstring& file, const BYTE* bytes, size_t size)
    {
        HANDLE handle = CreateFileW(file.c_str(),
                                    GENERIC_WRITE,
                                    0,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
        if (handle == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        DWORD written = 0;
        const BOOL ok = size == 0 ||
                        WriteFile(handle, bytes, static_cast<DWORD>(size), &written, NULL);
        CloseHandle(handle);
        return ok && written == size;
    }



    bool CreateTempProfilePath(std::wstring& path)
    {
        wchar_t tempDir[MAX_PATH] = { 0 };
        wchar_t tempFile[MAX_PATH] = { 0 };
        if (GetTempPathW(ARRAYSIZE(tempDir), tempDir) == 0)
        {
            return false;
        }
        if (GetTempFileNameW(tempDir, L"sv", 0, tempFile) == 0)
        {
            return false;
        }

        path = tempFile;
        return true;
    }



    bool StartsWithPng(const std::vector<BYTE>& bytes)
    {
        return bytes.size() >= ARRAYSIZE(kPngSignature) &&
               std::memcmp(&bytes[0], kPngSignature, ARRAYSIZE(kPngSignature)) == 0;
    }



    bool FindBytes(const std::vector<BYTE>& bytes, const BYTE* pattern, size_t patternSize, size_t& index)
    {
        if (!pattern || patternSize == 0 || bytes.size() < patternSize)
        {
            return false;
        }

        for (size_t i = 0; i <= bytes.size() - patternSize; ++i)
        {
            if (std::memcmp(&bytes[i], pattern, patternSize) == 0)
            {
                index = i;
                return true;
            }
        }
        return false;
    }



    DWORD ReadPngUInt32(const std::vector<BYTE>& bytes, size_t index)
    {
        return (static_cast<DWORD>(bytes[index]) << 24) |
               (static_cast<DWORD>(bytes[index + 1]) << 16) |
               (static_cast<DWORD>(bytes[index + 2]) << 8) |
               static_cast<DWORD>(bytes[index + 3]);
    }



    void AppendPngUInt32(std::vector<BYTE>& bytes, DWORD value)
    {
        bytes.push_back(static_cast<BYTE>((value >> 24) & 0xFF));
        bytes.push_back(static_cast<BYTE>((value >> 16) & 0xFF));
        bytes.push_back(static_cast<BYTE>((value >> 8) & 0xFF));
        bytes.push_back(static_cast<BYTE>(value & 0xFF));
    }



    DWORD UpdatePngCrc32(DWORD crc, const BYTE* bytes, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
        {
            crc ^= bytes[i];
            for (int bit = 0; bit < 8; ++bit)
            {
                crc = (crc & 1) ? (0xEDB88320u ^ (crc >> 1)) : (crc >> 1);
            }
        }
        return crc;
    }



    DWORD PngChunkCrc32(const BYTE* chunkType, const BYTE* data, size_t dataSize)
    {
        DWORD crc = 0xFFFFFFFFu;
        crc = UpdatePngCrc32(crc, chunkType, 4);
        if (data && dataSize > 0)
        {
            crc = UpdatePngCrc32(crc, data, dataSize);
        }
        return crc ^ 0xFFFFFFFFu;
    }



    bool AppendPngChunk(std::vector<BYTE>& bytes, const BYTE* chunkType, const std::vector<BYTE>& data)
    {
        if (!chunkType || data.size() > static_cast<size_t>(0xFFFFFFFFu))
        {
            return false;
        }

        AppendPngUInt32(bytes, static_cast<DWORD>(data.size()));
        bytes.insert(bytes.end(), chunkType, chunkType + 4);
        if (!data.empty())
        {
            bytes.insert(bytes.end(), data.begin(), data.end());
        }

        const BYTE* dataPtr = data.empty() ? NULL : &data[0];
        AppendPngUInt32(bytes, PngChunkCrc32(chunkType, dataPtr, data.size()));
        return true;
    }



    bool BuildPngConfigPackage(const std::vector<BYTE>& thumbnailPng,
                               const std::vector<BYTE>& configBytes,
                               std::vector<BYTE>& packageBytes)
    {
        packageBytes.clear();
        if (!StartsWithPng(thumbnailPng))
        {
            return false;
        }

        size_t pos = ARRAYSIZE(kPngSignature);
        while (pos + 12 <= thumbnailPng.size())
        {
            const size_t chunkStart = pos;
            const DWORD length = ReadPngUInt32(thumbnailPng, pos);
            const size_t chunkTotal = static_cast<size_t>(length) + 12;
            if (chunkTotal > thumbnailPng.size() - chunkStart)
            {
                return false;
            }

            const BYTE* chunkType = &thumbnailPng[chunkStart + 4];
            if (std::memcmp(chunkType, kPngIendChunkType, 4) == 0)
            {
                packageBytes.assign(thumbnailPng.begin(), thumbnailPng.begin() + chunkStart);
                if (!AppendPngChunk(packageBytes, kPngConfigChunkType, configBytes))
                {
                    packageBytes.clear();
                    return false;
                }
                packageBytes.insert(packageBytes.end(), thumbnailPng.begin() + chunkStart, thumbnailPng.end());
                return true;
            }

            pos += chunkTotal;
        }

        return false;
    }



    bool ExtractPngChunkConfig(const std::vector<BYTE>& bytes, std::vector<BYTE>& configBytes)
    {
        configBytes.clear();
        if (!StartsWithPng(bytes))
        {
            return false;
        }

        size_t pos = ARRAYSIZE(kPngSignature);
        while (pos + 12 <= bytes.size())
        {
            const size_t chunkStart = pos;
            const DWORD length = ReadPngUInt32(bytes, chunkStart);
            const size_t chunkTotal = static_cast<size_t>(length) + 12;
            if (chunkTotal > bytes.size() - chunkStart)
            {
                return false;
            }

            const BYTE* chunkType = &bytes[chunkStart + 4];
            if (std::memcmp(chunkType, kPngConfigChunkType, 4) == 0)
            {
                const size_t dataStart = chunkStart + 8;
                configBytes.assign(bytes.begin() + dataStart, bytes.begin() + dataStart + length);
                return true;
            }
            if (std::memcmp(chunkType, kPngIendChunkType, 4) == 0)
            {
                break;
            }

            pos += chunkTotal;
        }

        return false;
    }



    bool ExtractTrailingConfig(const std::vector<BYTE>& bytes, std::vector<BYTE>& configBytes)
    {
        configBytes.clear();
        if (!StartsWithPng(bytes))
        {
            return false;
        }

        size_t markerIndex = 0;
        const BYTE* marker = reinterpret_cast<const BYTE*>(kConfigMarker);
        const size_t markerSize = sizeof(kConfigMarker) - 1;
        if (!FindBytes(bytes, marker, markerSize, markerIndex))
        {
            return false;
        }

        const size_t configStart = markerIndex + markerSize;
        if (configStart >= bytes.size())
        {
            return false;
        }

        configBytes.assign(bytes.begin() + configStart, bytes.end());
        return true;
    }



    bool ExtractEmbeddedConfig(const std::vector<BYTE>& bytes, std::vector<BYTE>& configBytes)
    {
        return ExtractPngChunkConfig(bytes, configBytes) ||
               ExtractTrailingConfig(bytes, configBytes);
    }



    bool WriteConfigPackage(const std::wstring& file,
                            const std::vector<BYTE>& thumbnailPng,
                            const std::vector<BYTE>& configBytes)
    {
        std::vector<BYTE> packageBytes;
        if (!BuildPngConfigPackage(thumbnailPng, configBytes, packageBytes))
        {
            return false;
        }

        return WriteFileBytes(file, packageBytes.empty() ? NULL : &packageBytes[0], packageBytes.size());
    }





    bool PrepareProfileReadFile(const std::wstring& sourcePath,
                                std::wstring& profilePath,
                                TempFileGuard& tempProfile)
    {
        profilePath = sourcePath;

        std::vector<BYTE> fileBytes;
        if (!ReadFileBytes(sourcePath, fileBytes))
        {
            return true;
        }

        std::vector<BYTE> configBytes;
        if (!ExtractEmbeddedConfig(fileBytes, configBytes))
        {
            return true;
        }

        if (!CreateTempProfilePath(tempProfile.path))
        {
            return false;
        }

        if (!WriteFileBytes(tempProfile.path, configBytes.empty() ? NULL : &configBytes[0], configBytes.size()))
        {
            DeleteFileW(tempProfile.path.c_str());
            tempProfile.path.clear();
            return false;
        }

        profilePath = tempProfile.path;
        return true;
    }



    bool SetRegStringValue(HKEY root, const std::wstring& subKey, const wchar_t* valueName, const std::wstring& value)
    {
        HKEY key = NULL;
        LONG result = RegCreateKeyExW(root,
                                      subKey.c_str(),
                                      0,
                                      NULL,
                                      REG_OPTION_NON_VOLATILE,
                                      KEY_SET_VALUE,
                                      NULL,
                                      &key,
                                      NULL);
        if (result != ERROR_SUCCESS)
        {
            return false;
        }

        const BYTE* data = reinterpret_cast<const BYTE*>(value.c_str());
        const DWORD size = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
        result = RegSetValueExW(key, valueName, 0, REG_SZ, data, size);
        RegCloseKey(key);
        return result == ERROR_SUCCESS;
    }



    bool IsVerifyExecutable(const std::wstring& exePath)
    {
        const wchar_t* fileName = PathFindFileNameW(exePath.c_str());
        return fileName && _wcsnicmp(fileName, L"SplitViewerVerify_", 18) == 0;
    }



    void RegisterSvFileType()
    {
        wchar_t modulePath[MAX_PATH] = { 0 };
        if (GetModuleFileNameW(NULL, modulePath, ARRAYSIZE(modulePath)) == 0)
        {
            return;
        }

        const std::wstring exePath = modulePath;
        if (IsVerifyExecutable(exePath))
        {
            return;
        }

        const std::wstring extKey = L"Software\\Classes\\.sv";
        SetRegStringValue(HKEY_CURRENT_USER, extKey, NULL, L"SplitViewer.sv");
        SetRegStringValue(HKEY_CURRENT_USER, extKey, L"Content Type", L"image/png");
        SetRegStringValue(HKEY_CURRENT_USER, extKey, L"PerceivedType", L"image");
        SetRegStringValue(HKEY_CURRENT_USER, extKey + L"\\PersistentHandler", NULL, L"{098f2470-bae0-11cd-b579-08002b30bfeb}");
        SetRegStringValue(HKEY_CURRENT_USER,
                          extKey + L"\\ShellEx\\{8895b1c6-b41f-4c1c-a562-0d564250836f}",
                          NULL,
                          L"{d7ac9243-9d84-4498-8192-d7ad263b63c8}");
        SetRegStringValue(HKEY_CURRENT_USER,
                          extKey + L"\\ShellEx\\{e357fccd-a995-4576-b01f-234630154e96}",
                          NULL,
                          L"{C7657C4A-9F68-40fa-A4DF-96BC08EB3551}");

        SetRegStringValue(HKEY_CURRENT_USER, L"Software\\Classes\\SplitViewer.sv", NULL, L"SplitViewer Configuration");
        SetRegStringValue(HKEY_CURRENT_USER, L"Software\\Classes\\SplitViewer.sv\\DefaultIcon", NULL, exePath + L",0");
        SetRegStringValue(HKEY_CURRENT_USER,
                          L"Software\\Classes\\SplitViewer.sv\\ShellEx\\{8895b1c6-b41f-4c1c-a562-0d564250836f}",
                          NULL,
                          L"{d7ac9243-9d84-4498-8192-d7ad263b63c8}");
        SetRegStringValue(HKEY_CURRENT_USER,
                          L"Software\\Classes\\SplitViewer.sv\\ShellEx\\{e357fccd-a995-4576-b01f-234630154e96}",
                          NULL,
                          L"{C7657C4A-9F68-40fa-A4DF-96BC08EB3551}");
        SetRegStringValue(HKEY_CURRENT_USER,
                          L"Software\\Classes\\SplitViewer.sv\\shell\\open\\command",
                          NULL,
                          L"\"" + exePath + L"\" \"%1\"");

        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    }
}