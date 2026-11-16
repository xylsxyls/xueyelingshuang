#include "NetGatewayWindowsProbe.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#ifdef _WIN32
#include <processsnapshot.h>

bool NetGatewayWindowsProbe::warmThreadWait()
{
    const auto wait = reinterpret_cast<decltype(&WaitOnAddress)>(GetProcAddress(GetModuleHandleW(L"kernelbase.dll"), "WaitOnAddress"));
    if (!wait)
    {
        return false;
    }
    DWORD before = 0;
    DWORD after = 0;
    uint32_t value = 0;
    uint32_t expected = 0;
    if (!GetProcessHandleCount(GetCurrentProcess(), &before))
    {
        return false;
    }
    const BOOL result = wait(&value, &expected, sizeof(value), 1);
    const DWORD error = GetLastError();
    if (!GetProcessHandleCount(GetCurrentProcess(), &after))
    {
        return false;
    }
    std::printf("RESOURCE_WAIT_WARM handles=%u/%u result=%u error=%u\n", before, after, result, error);
    return result == FALSE && error == ERROR_TIMEOUT;
}

std::string NetGatewayWindowsProbe::processHandles()
{
    const HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    const auto capture = reinterpret_cast<decltype(&PssCaptureSnapshot)>(GetProcAddress(kernel, "PssCaptureSnapshot"));
    const auto release = reinterpret_cast<decltype(&PssFreeSnapshot)>(GetProcAddress(kernel, "PssFreeSnapshot"));
    const auto walk = reinterpret_cast<decltype(&PssWalkSnapshot)>(GetProcAddress(kernel, "PssWalkSnapshot"));
    const auto createMarker = reinterpret_cast<decltype(&PssWalkMarkerCreate)>(GetProcAddress(kernel, "PssWalkMarkerCreate"));
    const auto freeMarker = reinterpret_cast<decltype(&PssWalkMarkerFree)>(GetProcAddress(kernel, "PssWalkMarkerFree"));
    if (!capture || !release || !walk || !createMarker || !freeMarker)
    {
        return "Handle snapshot unavailable\n";
    }
    HPSS snapshot = nullptr;
    HPSSWALK marker = nullptr;
    DWORD error = capture(GetCurrentProcess(), static_cast<PSS_CAPTURE_FLAGS>(PSS_CAPTURE_HANDLES |
        PSS_CAPTURE_HANDLE_NAME_INFORMATION | PSS_CAPTURE_HANDLE_BASIC_INFORMATION), 0, &snapshot);
    if (error == ERROR_SUCCESS)
    {
        error = createMarker(nullptr, &marker);
    }
    std::string result;
    try
    {
        PSS_HANDLE_ENTRY entry = {};
        while (error == ERROR_SUCCESS && (error = walk(snapshot, PSS_WALK_HANDLES, marker, &entry, sizeof(entry))) == ERROR_SUCCESS)
        {
            result += std::to_string(reinterpret_cast<uintptr_t>(entry.Handle)) + " " +
                CStringManager::UnicodeToUtf8(std::wstring(entry.TypeName ? entry.TypeName : L"",
                    entry.TypeName ? entry.TypeNameLength / sizeof(wchar_t) : 0)) + " " +
                CStringManager::UnicodeToUtf8(std::wstring(entry.ObjectName ? entry.ObjectName : L"",
                    entry.ObjectName ? entry.ObjectNameLength / sizeof(wchar_t) : 0)) + "\n";
        }
    }
    catch (...)
    {
        if (marker) { freeMarker(marker); }
        if (snapshot) { release(GetCurrentProcess(), snapshot); }
        throw;
    }
    if (marker) { freeMarker(marker); }
    if (snapshot) { release(GetCurrentProcess(), snapshot); }
    if (error != ERROR_NO_MORE_ITEMS)
    {
        result += "Handle snapshot error=" + std::to_string(error) + "\n";
    }
    // Windows返回的字符串长度可能包括终止符；诊断输出不能被首个NUL截断。
    result.erase(std::remove(result.begin(), result.end(), '\0'), result.end());
    return result;
}

NetGatewayWindowsProbe::NetGatewayWindowsProbe() :
m_anchor(nullptr), m_pid(0), m_window(nullptr), m_count(0), m_invalid(false)
{
    m_anchor = CreateWindowExW(0, L"STATIC", L"Gateway owned test observer", 0,
        0, 0, 1, 1, nullptr, nullptr, nullptr, nullptr);
    if (m_anchor == nullptr || IsWindowVisible(m_anchor))
    {
        if (m_anchor != nullptr) { DestroyWindow(m_anchor); }
        throw std::runtime_error("Cannot create the invisible test observer");
    }
}

NetGatewayWindowsProbe::~NetGatewayWindowsProbe()
{
    if (m_anchor != nullptr) { DestroyWindow(m_anchor); }
}

bool NetGatewayWindowsProbe::guiImage(const std::string& executable)
{
    std::vector<uint8_t> bytes;
    if (!CSystem::readBinaryFile(CStringManager::Utf8ToUnicode(executable), 64 * 1024 * 1024, bytes) ||
        bytes.size() < 64 || bytes[0] != 'M' || bytes[1] != 'Z') { return false; }
    uint32_t offset = 0;
    std::memcpy(&offset, &bytes[60], sizeof(offset));
    if (offset > bytes.size() - 64 || bytes.size() - offset < 94 ||
        std::memcmp(&bytes[offset], "PE\0\0", 4) != 0) { return false; }
    const uint16_t optionalSize = bytes[offset + 20] | (uint16_t(bytes[offset + 21]) << 8);
    const uint16_t magic = bytes[offset + 24] | (uint16_t(bytes[offset + 25]) << 8);
    return optionalSize >= 70 && optionalSize <= bytes.size() - offset - 24 &&
        (magic == 0x10b || magic == 0x20b) && bytes[offset + 92] == IMAGE_SUBSYSTEM_WINDOWS_GUI && bytes[offset + 93] == 0;
}

BOOL CALLBACK NetGatewayWindowsProbe::Enumerate(HWND window, LPARAM context)
{
    NetGatewayWindowsProbe* probe = reinterpret_cast<NetGatewayWindowsProbe*>(context);
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    if (pid == probe->m_pid)
    {
        if (IsWindowVisible(window)) { probe->m_invalid = true; }
        wchar_t name[128] = {};
        if (GetClassNameW(window, name, 128) == 0) { probe->m_invalid = true; }
        else if (std::wcscmp(name, L"XylsNetClientManagerLifetime") == 0)
        {
            ++probe->m_count;
            probe->m_window = window;
        }
    }
    return TRUE;
}

HWND NetGatewayWindowsProbe::window(const NetTestProcess& owner, const std::string& executable, uint32_t pid)
{
    std::vector<uint32_t> members;
    if (!owner.ownedProcesses(executable, members) || std::find(members.begin(), members.end(), pid) == members.end())
    {
        throw std::runtime_error("Window PID is not a live owned candidate");
    }
    m_pid = pid;
    m_count = 0;
    m_window = nullptr;
    m_invalid = false;
    if (!EnumWindows(&Enumerate, reinterpret_cast<LPARAM>(this)) || m_invalid || m_count != 1)
    {
        throw std::runtime_error("Gateway must own exactly one hidden lifecycle window and no visible window");
    }
    return m_window;
}

uintptr_t NetGatewayWindowsProbe::message(HWND window, uint32_t pid, uint32_t value, uintptr_t parameter)
{
    DWORD owner = 0;
    GetWindowThreadProcessId(window, &owner);
    DWORD_PTR returned = 0;
    if (owner != pid || !SendMessageTimeoutW(window, value, parameter, 0, SMTO_ABORTIFHUNG, 5000, &returned))
    {
        throw std::runtime_error("Owned lifecycle window changed or did not handle the message");
    }
    return static_cast<uintptr_t>(returned);
}

uint32_t NetGatewayWindowsProbe::receiver(const std::string& name)
{
    if (name.empty() || name.size() >= 1024 || name.find('\0') != std::string::npos)
    {
        throw std::runtime_error("Invalid IPC receiver probe name");
    }
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t index = 0; index < name.size(); ++index) { hash = (hash ^ uint8_t(name[index])) * UINT64_C(1099511628211); }
    const std::wstring key = CStringManager::Utf8ToUnicode("P3N" + std::to_string(hash));
    HANDLE mapping = OpenFileMappingW(FILE_MAP_READ, FALSE, key.c_str());
    if (mapping == nullptr)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) { return 0; }
        throw std::runtime_error("Cannot inspect the IPC name directory");
    }
    const ProcessWorkNameDirectory* view = static_cast<const ProcessWorkNameDirectory*>(
        MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, sizeof(ProcessWorkNameDirectory)));
    if (view == nullptr)
    {
        CloseHandle(mapping);
        throw std::runtime_error("Cannot map the IPC name directory");
    }
    // 在生产进程发布ready之后观察；仅允许同一正PID的稳定完整快照。
    const int32_t before = *reinterpret_cast<const volatile int32_t*>(&view->m_pid);
    MemoryBarrier();
    ProcessWorkNameDirectory snapshot;
    std::memcpy(&snapshot, view, sizeof(snapshot));
    MemoryBarrier();
    const int32_t after = *reinterpret_cast<const volatile int32_t*>(&view->m_pid);
    UnmapViewOfFile(view);
    CloseHandle(mapping);
    if (before <= 0 || before != after || snapshot.m_pid != before || snapshot.m_identity == 0 ||
        snapshot.m_length != static_cast<int32_t>(name.size()) ||
        std::memcmp(snapshot.m_name, name.data(), name.size()) != 0 || snapshot.m_name[name.size()] != '\0')
    {
        throw std::runtime_error("IPC name directory is inconsistent with the ready process");
    }
    return static_cast<uint32_t>(snapshot.m_pid);
}

NetTestStartupEntry::NetTestStartupEntry(const std::string& backupFile) :
m_key(nullptr), m_present(false), m_type(0), m_restore(false)
{
    const LONG opened = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, nullptr, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, nullptr, &m_key, nullptr);
    if (opened != ERROR_SUCCESS)
    {
        throw std::runtime_error("Cannot prepare the current-user Run key: " + std::to_string(opened));
    }
    try
    {
        DWORD size = 0;
        LONG status = RegQueryValueExW(m_key, L"NetClientManager", nullptr, &m_type, nullptr, &size);
        if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND)
        {
            throw std::runtime_error("Cannot back up the gateway startup value");
        }
        m_present = status == ERROR_SUCCESS;
        if (m_present)
        {
            if (size > 1024 * 1024) { throw std::runtime_error("Startup backup exceeds test budget"); }
            m_bytes.resize(size);
            if (RegQueryValueExW(m_key, L"NetClientManager", nullptr, &m_type,
                m_bytes.empty() ? nullptr : m_bytes.data(), &size) != ERROR_SUCCESS || size != m_bytes.size())
            {
                throw std::runtime_error("Startup value changed during backup");
            }
        }
        // durable备份先于生产安装命令；测试宿主异常终止时仍能按原类型恢复。
        std::vector<uint8_t> backup(12 + m_bytes.size(), 0);
        std::memcpy(backup.data(), "NTS1", 4);
        backup[4] = m_present ? 1 : 0;
        std::memcpy(&backup[8], &m_type, sizeof(m_type));
        if (!m_bytes.empty()) { std::memcpy(&backup[12], m_bytes.data(), m_bytes.size()); }
        const std::wstring target = CStringManager::Utf8ToUnicode(backupFile);
        HANDLE file = CreateFileW(target.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) { throw std::runtime_error("Cannot create original startup evidence"); }
        DWORD written = 0;
        const bool saved = WriteFile(file, backup.data(), static_cast<DWORD>(backup.size()), &written, nullptr) &&
            written == backup.size() && FlushFileBuffers(file);
        const bool closed = CloseHandle(file) != FALSE;
        if (!saved || !closed) { throw std::runtime_error("Cannot persist original startup evidence"); }
        m_restore = true;
    }
    catch (...)
    {
        RegCloseKey(m_key);
        m_key = nullptr;
        throw;
    }
}

NetTestStartupEntry::~NetTestStartupEntry()
{
    if (!restore()) { std::fputs("Failed to restore the original gateway startup value\n", stderr); }
    if (m_key != nullptr) { RegCloseKey(m_key); }
}

bool NetTestStartupEntry::matches(const std::wstring& value) const
{
    DWORD type = 0;
    DWORD size = 0;
    if (RegQueryValueExW(m_key, L"NetClientManager", nullptr, &type, nullptr, &size) != ERROR_SUCCESS ||
        type != REG_SZ || size != (value.size() + 1) * sizeof(wchar_t)) { return false; }
    std::vector<uint8_t> bytes(size);
    return RegQueryValueExW(m_key, L"NetClientManager", nullptr, &type, bytes.data(), &size) == ERROR_SUCCESS &&
        type == REG_SZ && size == bytes.size() && std::memcmp(bytes.data(), value.c_str(), size) == 0;
}

bool NetTestStartupEntry::absent() const
{
    return RegQueryValueExW(m_key, L"NetClientManager", nullptr, nullptr, nullptr, nullptr) == ERROR_FILE_NOT_FOUND;
}

bool NetTestStartupEntry::restore()
{
    if (!m_restore) { return true; }
    const LONG status = m_present ? RegSetValueExW(m_key, L"NetClientManager", 0, m_type,
        m_bytes.empty() ? nullptr : m_bytes.data(), static_cast<DWORD>(m_bytes.size())) : RegDeleteValueW(m_key, L"NetClientManager");
    if (status != ERROR_SUCCESS && (m_present || status != ERROR_FILE_NOT_FOUND)) { return false; }
    m_restore = false;
    return true;
}
#endif