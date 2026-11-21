#include "MxmlCpp.h"
#include "mxml/mxml.h"
#include <cstring>
#include <cstdlib>
#include <memory>
#include <stdint.h>
#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "mxmlstatd.lib")
#else
#pragma comment(lib, "mxmlstat.lib")
#endif
#endif

/** 检查 XML 1.0 NameStartChar。
@param [in] c Unicode 码点
@return 可用作名称首字符时返回 true
*/
static bool NameStart(uint32_t c)
{
    return c == ':' || c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= 0xc0 && c <= 0xd6) || (c >= 0xd8 && c <= 0xf6) || (c >= 0xf8 && c <= 0x2ff) ||
        (c >= 0x370 && c <= 0x37d) || (c >= 0x37f && c <= 0x1fff) || (c >= 0x200c && c <= 0x200d) ||
        (c >= 0x2070 && c <= 0x218f) || (c >= 0x2c00 && c <= 0x2fef) || (c >= 0x3001 && c <= 0xd7ff) ||
        (c >= 0xf900 && c <= 0xfdcf) || (c >= 0xfdf0 && c <= 0xfffd) || (c >= 0x10000 && c <= 0xeffff);
}

/** 检查无截断的 UTF-8 和 XML 1.0 允许的字符；名称另按 Name 检查。
@param [in] input 完整字符串
@param [in] name 是否按 XML 名称检查
@return 合法返回 true
*/
static bool ValidXmlString(const std::string& input, bool name)
{
    for (size_t pos = 0; pos < input.size(); )
    {
        const bool first = pos == 0;
        uint32_t c = static_cast<unsigned char>(input[pos++]);
        if (c >= 0x80)
        {
            if (c < 0xc2 || c > 0xf4)
            {
                return false;
            }
            const unsigned count = c < 0xe0 ? 1 : (c < 0xf0 ? 2 : 3);
            c &= 0x7f >> (count + 1);
            if (input.size() - pos < count)
            {
                return false;
            }
            for (unsigned n = 0; n < count; ++n)
            {
                const unsigned next = static_cast<unsigned char>(input[pos++]);
                if ((next & 0xc0) != 0x80)
                {
                    return false;
                }
                c = (c << 6) | (next & 63);
            }
            if ((count == 2 && c < 0x800) || (count == 3 && c < 0x10000) || c > 0x10ffff ||
                (c >= 0xd800 && c <= 0xdfff) || c == 0xfffe || c == 0xffff)
            {
                return false;
            }
        }
        if (name)
        {
            if (!NameStart(c) && (first || !(c == '-' || c == '.' || (c >= '0' && c <= '9') ||
                c == 0xb7 || (c >= 0x300 && c <= 0x36f) || (c >= 0x203f && c <= 0x2040))))
            {
                return false;
            }
        }
        else if (c < 32 && c != 9 && c != 10 && c != 13)
        {
            return false;
        }
    }
    return !name || !input.empty();
}

/** 读取业务元素名，过滤声明、注释、CDATA 及文本。
@param [in] node 借用的节点，可为空
@return 借用的名称，其他类型返回空指针
*/
static const char* ElementName(mxml_node_t* node)
{
    const char* name = mxmlGetElement(node);
    return name && *name != '?' && *name != '!' ? name : nullptr;
}

/** Mini-XML 对名称另行分配，只有节点和名称均存在才交付。
@param [in] node 新建的元素或声明，允许为空；失败时释放
@return 完整节点，失败返回空指针
*/
static mxml_node_t* CheckedElement(mxml_node_t* node)
{
    if (!mxmlGetElement(node))
    {
        mxmlDelete(node);
        return nullptr;
    }
    return node;
}

/** 查找一层中的第一个普通元素。
@param [in] parent 借用的父节点
@return 首个元素，无则返回空指针
*/
static mxml_node_t* FirstElement(mxml_node_t* parent)
{
    mxml_node_t* child = mxmlGetFirstChild(parent);
    while (child && !ElementName(child))
    {
        child = mxmlGetNextSibling(child);
    }
    return child;
}

MxmlCpp::MxmlCpp() :
m_xml(nullptr),
m_xmlVisit(nullptr)
{

}

MxmlCpp::~MxmlCpp()
{
    mxmlDelete(m_xml);
}

bool MxmlCpp::load(const std::string& xml)
{
    const size_t end = xml.find_last_not_of(" \t\r\n");
    if (end == std::string::npos || xml[end] != '>' || !ValidXmlString(xml, false))
    {
        return false;
    }
    // 解析到不可能由输入闭合的容器，Mini-XML 才会检查裸根元素也已闭合。
    std::unique_ptr<mxml_node_t, decltype(&mxmlDelete)> doc(CheckedElement(mxmlNewElement(nullptr, " ")), &mxmlDelete);
    const size_t bom = xml.compare(0, 3, "\xef\xbb\xbf") == 0 ? 3 : 0;
    if (!doc || mxmlLoadString(doc.get(), xml.c_str() + bom, MXML_OPAQUE_CALLBACK) != doc.get())
    {
        return false;
    }
    mxml_node_t* root = nullptr;
    bool declaration = false;
    for (mxml_node_t* node = mxmlGetFirstChild(doc.get()); node; node = mxmlGetNextSibling(node))
    {
        const char* name = mxmlGetElement(node);
        if (ElementName(node))
        {
            if (root)
            {
                return false;
            }
            root = node;
        }
        else if (!name)
        {
            const char* data = mxmlGetOpaque(node);
            if (!data || data[std::strspn(data, " \t\r\n")] != '\0')
            {
                return false;
            }
        }
        else if (mxmlGetCDATA(node))
        {
            return false;
        }
        else if (std::strncmp(name, "?xml", 4) == 0 && (name[4] == ' ' || name[4] == '\t' || name[4] == '?'))
        {
            if (declaration || root)
            {
                return false;
            }
            declaration = true;
        }
    }
    if (!root)
    {
        return false;
    }
    mxmlDelete(m_xml);
    m_xml = doc.release();
    m_xmlVisit = declaration ? m_xml : root;
    return true;
}

bool MxmlCpp::create()
{
    std::unique_ptr<mxml_node_t, decltype(&mxmlDelete)> doc(CheckedElement(mxmlNewElement(nullptr, " ")), &mxmlDelete);
    mxml_node_t* declaration = doc ? CheckedElement(mxmlNewXML("1.0")) : nullptr;
    if (!declaration)
    {
        return false;
    }
    mxmlAdd(doc.get(), MXML_ADD_AFTER, nullptr, declaration);
    mxmlDelete(m_xml);
    m_xml = doc.release();
    m_xmlVisit = m_xml;
    return true;
}

bool MxmlCpp::addChild(const std::string& name)
{
    return m_xmlVisit && ValidXmlString(name, true) && (m_xmlVisit != m_xml || !FirstElement(m_xml)) &&
        CheckedElement(mxmlNewElement(m_xmlVisit, name.c_str())) != nullptr;
}

bool MxmlCpp::addChild(const std::string& name, const std::string& text)
{
    if (!ValidXmlString(text, false) || !addChild(name))
    {
        return false;
    }
    mxml_node_t* child = mxmlGetLastChild(m_xmlVisit);
    if (!mxmlGetOpaque(mxmlNewOpaque(child, text.c_str())))
    {
        mxmlDelete(child);
        return false;
    }
    return true;
}

bool MxmlCpp::addAttr(const std::string& name, const std::string& attr)
{
    if (!m_xmlVisit || m_xmlVisit == m_xml || !ValidXmlString(name, true) || !ValidXmlString(attr, false))
    {
        return false;
    }
    mxmlElementSetAttr(m_xmlVisit, name.c_str(), attr.c_str());
    const char* stored = mxmlElementGetAttr(m_xmlVisit, name.c_str());
    return stored && attr == stored;
}

bool MxmlCpp::firstChild(const std::string& element, const std::string& attr, const std::string& value)
{
    if (!m_xmlVisit || (attr.empty() && !value.empty()) || attr.find('\0') != std::string::npos)
    {
        return false;
    }
    for (mxml_node_t* node = mxmlGetFirstChild(m_xmlVisit); node; node = mxmlGetNextSibling(node))
    {
        const char* name = ElementName(node);
        if (name && (element.empty() || element == name))
        {
            const char* stored = attr.empty() ? nullptr : mxmlElementGetAttr(node, attr.c_str());
            if (attr.empty() || (stored && (value.empty() || value == stored)))
            {
                m_xmlVisit = node;
                return true;
            }
        }
    }
    return false;
}

bool MxmlCpp::pre()
{
    for (mxml_node_t* node = mxmlGetPrevSibling(m_xmlVisit); node; node = mxmlGetPrevSibling(node))
    {
        if (ElementName(node))
        {
            m_xmlVisit = node;
            return true;
        }
    }
    return false;
}

bool MxmlCpp::next()
{
    for (mxml_node_t* node = mxmlGetNextSibling(m_xmlVisit); node; node = mxmlGetNextSibling(node))
    {
        if (ElementName(node))
        {
            m_xmlVisit = node;
            return true;
        }
    }
    return false;
}

bool MxmlCpp::parent()
{
    if (!m_xmlVisit || m_xmlVisit == m_xml)
    {
        return false;
    }
    m_xmlVisit = mxmlGetParent(m_xmlVisit);
    return true;
}

std::string MxmlCpp::element()
{
    const char* name = m_xmlVisit == m_xml ? nullptr : ElementName(m_xmlVisit);
    return name ? name : "";
}

std::string MxmlCpp::attr(const std::string& name)
{
    const char* value = m_xmlVisit == m_xml || name.find('\0') != std::string::npos ? nullptr :
        mxmlElementGetAttr(m_xmlVisit, name.c_str());
    return value ? value : "";
}

std::string MxmlCpp::text()
{
    std::string result;
    if (m_xmlVisit != m_xml)
    {
        for (mxml_node_t* node = mxmlGetFirstChild(m_xmlVisit); node; node = mxmlGetNextSibling(node))
        {
            const char* value = mxmlGetType(node) == MXML_OPAQUE ? mxmlGetOpaque(node) : mxmlGetCDATA(node);
            if (value)
            {
                result.append(value);
            }
        }
    }
    return result;
}

std::string MxmlCpp::toString()
{
    std::string result;
    if (FirstElement(m_xml))
    {
        for (mxml_node_t* node = mxmlGetFirstChild(m_xml); node; node = mxmlGetNextSibling(node))
        {
            if (mxmlGetType(node) == MXML_ELEMENT)
            {
                std::unique_ptr<char, decltype(&std::free)> data(mxmlSaveAllocString(node, nullptr), &std::free);
                if (!data)
                {
                    return std::string();
                }
                result.append(data.get());
            }
        }
    }
    return result;
}

/*
int main()
{
    MxmlCpp writer;
    if (!writer.create() || !writer.addChild("users") || !writer.firstChild() ||
        !writer.addChild("user", "Ada") || !writer.addChild("user", "Lin") || !writer.firstChild() ||
        !writer.addAttr("id", "1") || !writer.next() || !writer.pre() || !writer.parent())
    {
        return 1;
    }
    MxmlCpp reader;
    if (!reader.load(writer.toString()) || !reader.firstChild("users") ||
        !reader.firstChild("user", "id", "1"))
    {
        return 1;
    }
    return reader.element() == "user" && reader.attr("id") == "1" && reader.text() == "Ada" ? 0 : 1;
}
*/