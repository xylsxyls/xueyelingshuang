#include "Cini.h"
// UTF-8 名称仅对 ASCII 字母折叠大小写，各端使用同一比较规则
#define SI_NO_MBCS
#include "SimpleIni.h"
#include <memory>
#include <cerrno>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#include <algorithm>
#include <limits>
#endif

/** 检查名称能否原样写回单行 INI 文本
@param [in] name 键名或分区名；空分区名表示全局分区
@param [in] section 为 true 检查分区语法，为 false 检查键语法
@return 写入后能按同名重新解析时返回 true
*/
static bool IsWritableName(const std::string& name, bool section)
{
    if (name.empty())
    {
        return section;
    }
    if (name.front() == ' ' || name.front() == '\t' || name.back() == ' ' || name.back() == '\t')
    {
        return false;
    }
    if (!section && (name.front() == ';' || name.front() == '#' || name.front() == '['))
    {
        return false;
    }
    return name.find_first_of(section ? "]\r\n\0" : "=\r\n\0", 0, 4) == std::string::npos;
}

#ifdef _WIN32
/** 使用实际内容所需的缓冲区读取 Profile，达到调用方预算后遵循系统截断语义
@param [in] path 文件路径
@param [in] section 分区名，空指针请求分区列表
@param [in] key 键名，空指针请求名称列表
@param [in] limit 包含结尾零的字节预算
@return 系统写入的字节；列表保留项间的零字符
*/
static std::string ReadProfile(const std::string& path, const char* section, const char* key, uint64_t limit)
{
    const DWORD reserved = key ? 1 : 2;
    if (path.empty() || limit <= reserved)
    {
        return "";
    }
    limit = (std::min<uint64_t>)(limit, (std::numeric_limits<DWORD>::max)());
    DWORD capacity = static_cast<DWORD>((std::min<uint64_t>)(256, limit));
    std::string buffer;
    for (;;)
    {
        buffer.resize(capacity);
        DWORD count = section ? GetPrivateProfileStringA(section, key, "", &buffer[0], capacity, path.c_str()) :
            GetPrivateProfileSectionNamesA(&buffer[0], capacity, path.c_str());
        if (count < capacity - reserved || capacity == limit)
        {
            buffer.resize(count);
            return buffer;
        }
        capacity = static_cast<DWORD>((std::min<uint64_t>)(uint64_t(capacity) * 2, limit));
    }
}
#endif

Cini::Cini(const std::string& iniPath, bool useSimpleIni) :
m_iniPath(iniPath),
m_length(1024),
m_sectionNum(1024),
m_simpleIni(nullptr),
m_ready(false)
{
    if (m_iniPath.find('\0') != std::string::npos)
    {
        m_iniPath.clear();
        return;
    }
#ifdef _WIN32
    if (!useSimpleIni)
    {
        return;
    }
#else
    (void)useSimpleIni;
#endif
    std::unique_ptr<CSimpleIniA> ini(new CSimpleIniA(true));
    m_ready = iniPath.empty();
    if (!iniPath.empty())
    {
        std::unique_ptr<FILE, int (*)(FILE*)> file(std::fopen(iniPath.c_str(), "rb"), std::fclose);
        m_ready = file ? ini->LoadFile(file.get()) == SI_OK : errno == ENOENT;
    }
    m_simpleIni = ini.release();
}

Cini::~Cini()
{
    delete static_cast<CSimpleIniA*>(m_simpleIni);
}

bool Cini::loadData(const std::string& data)
{
    CSimpleIniA* ini = static_cast<CSimpleIniA*>(m_simpleIni);
    if (!ini)
    {
        return false;
    }
    m_ready = false;
    ini->Reset();
    m_ready = ini->LoadData(data.data(), data.size()) == SI_OK;
    return m_ready;
}

void Cini::setMaxLength(int32_t length)
{
    m_length = length;
}

void Cini::setMaxSection(int32_t sectionNum)
{
    m_sectionNum = sectionNum;
}

std::string Cini::readIni(const std::string& key, const std::string& section) const
{
    if (key.find('\0') != std::string::npos || section.find('\0') != std::string::npos)
    {
        return "";
    }
    if (m_ready)
    {
        return static_cast<CSimpleIniA*>(m_simpleIni)->GetValue(section.c_str(), key.c_str(), "");
    }
#ifdef _WIN32
    if (!m_simpleIni && m_length > 0)
    {
        return ReadProfile(m_iniPath, section.c_str(), key.c_str(), m_length);
    }
#endif
    return "";
}

void Cini::writeIni(const std::string& key, const std::string& value, const std::string& section) const
{
    if (!IsWritableName(key, false) || !IsWritableName(section, true) ||
        value.find_first_of("\r\n\0", 0, 3) != std::string::npos)
    {
        return;
    }
    if (m_ready)
    {
        CSimpleIniA* ini = static_cast<CSimpleIniA*>(m_simpleIni);
        if (ini->SetValue(section.c_str(), key.c_str(), value.c_str()) >= SI_OK && !m_iniPath.empty())
        {
            ini->SaveFile(m_iniPath.c_str(), false);
        }
    }
#ifdef _WIN32
    else if (!m_simpleIni && !m_iniPath.empty())
    {
        WritePrivateProfileStringA(section.c_str(), key.c_str(), value.c_str(), m_iniPath.c_str());
    }
#endif
}

std::vector<std::string> Cini::getAllSection() const
{
    std::vector<std::string> result;
    if (m_ready)
    {
        CSimpleIniA::TNamesDepend names;
        static_cast<CSimpleIniA*>(m_simpleIni)->GetAllSections(names);
        for (const auto& name : names)
        {
            result.push_back(name.pItem);
        }
    }
#ifdef _WIN32
    else if (!m_simpleIni && m_length > 0 && m_sectionNum > 0)
    {
        std::string names = ReadProfile(m_iniPath, nullptr, nullptr, uint64_t(m_length) * m_sectionNum);
        size_t start = 0;
        while (start < names.size() && result.size() < static_cast<size_t>(m_sectionNum))
        {
            size_t end = names.find('\0', start);
            if (end == std::string::npos || end == start)
            {
                break;
            }
            result.push_back(names.substr(start, end - start));
            start = end + 1;
        }
    }
#endif
    return result;
}

std::vector<std::string> Cini::getAllKey(const std::string& section) const
{
    if (section.find('\0') != std::string::npos)
    {
        return {};
    }
    std::vector<std::string> result;
    if (m_ready)
    {
        CSimpleIniA::TNamesDepend keys;
        static_cast<CSimpleIniA*>(m_simpleIni)->GetAllKeys(section.c_str(), keys);
        for (const auto& key : keys)
        {
            result.push_back(key.pItem);
        }
    }
    return result;
}

std::string Cini::getFirstKey(const std::string& section) const
{
    if (section.find('\0') != std::string::npos)
    {
        return "";
    }
    if (m_ready)
    {
        const CSimpleIniA::TKeyVal* keys = static_cast<CSimpleIniA*>(m_simpleIni)->GetSection(section.c_str());
        if (keys && !keys->empty())
        {
            return keys->begin()->first.pItem;
        }
    }
#ifdef _WIN32
    else if (!m_simpleIni && m_length > 0)
    {
        std::string keys = ReadProfile(m_iniPath, section.c_str(), nullptr, m_length);
        return keys.substr(0, keys.find('\0'));
    }
#endif
    return "";
}

void Cini::deleteKey(const std::string& key, const std::string& section) const
{
    if (key.find('\0') != std::string::npos || section.find('\0') != std::string::npos)
    {
        return;
    }
    if (m_ready)
    {
        CSimpleIniA* ini = static_cast<CSimpleIniA*>(m_simpleIni);
        if (ini->Delete(section.c_str(), key.c_str()) && !m_iniPath.empty())
        {
            ini->SaveFile(m_iniPath.c_str(), false);
        }
    }
#ifdef _WIN32
    else if (!m_simpleIni && !m_iniPath.empty())
    {
        WritePrivateProfileStringA(section.c_str(), key.c_str(), nullptr, m_iniPath.c_str());
    }
#endif
}

void Cini::deleteSection(const std::string& section) const
{
    if (section.find('\0') != std::string::npos)
    {
        return;
    }
    if (m_ready)
    {
        CSimpleIniA* ini = static_cast<CSimpleIniA*>(m_simpleIni);
        if (ini->Delete(section.c_str(), nullptr) && !m_iniPath.empty())
        {
            ini->SaveFile(m_iniPath.c_str(), false);
        }
    }
#ifdef _WIN32
    else if (!m_simpleIni && !m_iniPath.empty())
    {
        WritePrivateProfileSectionA(section.c_str(), nullptr, m_iniPath.c_str());
    }
#endif
}

// 内存配置用法示例，取消以下各行注释即可单独构建运行
// int main()
// {
//     Cini ini("", true);
//     // 这两个设置只限制 Windows Profile 的查询缓冲区
//     ini.setMaxLength(256);
//     ini.setMaxSection(16);
//     if (!ini.loadData("[Section]\nname=Cini\n[Extra]\nk=v\n"))
//     {
//         return 1;
//     }
//     ini.writeIni("level", "2");
//     if (ini.readIni("level") != "2" || ini.getAllSection().size() != 2 ||
//         ini.getAllKey().size() != 2 || ini.getFirstKey("Extra") != "k")
//     {
//         return 1;
//     }
//     ini.deleteKey("level");
//     ini.deleteSection("Extra");
//     return ini.readIni("level").empty() && ini.getAllSection().size() == 1 ? 0 : 1;
// }