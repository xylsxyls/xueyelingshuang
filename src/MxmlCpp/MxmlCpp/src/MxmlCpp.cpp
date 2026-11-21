#include "MxmlCpp.h"
#include "mxml/mxml.h"
#include <cstring>
#include <cstdlib>
#include <memory>
#include <set>
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

/** 校验 UTF-8 区间，可同时转写 UTF-16LE 供 Mini-XML 保留文内 U+FEFF。
@param [in] input 区间起点
@param [in] size 区间字节数
@param [in] name 是否按 XML 名称检查
@param [out] utf16 可选输出，逐个追加 UTF-16LE 码元
@return 合法返回 true
*/
static bool ValidXmlRange(const char* input, size_t size, bool name, std::string* utf16 = nullptr)
{
    for (size_t pos = 0; pos < size; )
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
            if (size - pos < count)
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
        if (utf16)
        {
            if (c >= 0x10000)
            {
                c -= 0x10000;
                const unsigned high = 0xd800 + (c >> 10);
                utf16->push_back(static_cast<char>(high));
                utf16->push_back(static_cast<char>(high >> 8));
                c = 0xdc00 + (c & 0x3ff);
            }
            utf16->push_back(static_cast<char>(c));
            utf16->push_back(static_cast<char>(c >> 8));
        }
    }
    return !name || size != 0;
}

/** 检查完整字符串，可用于名称、属性值和直接文本。
@param [in] input 完整 UTF-8 字节串
@param [in] name 是否检查名称
@return 合法返回 true
*/
static bool ValidXmlString(const std::string& input, bool name)
{
    return ValidXmlRange(input.data(), input.size(), name);
}

/** 对待交给 Mini-XML 的字节做局部替换，首次实际替换时才分配副本。 */
struct XmlInput
{
    const char* remaining; // 尚未复制的原输入起点
    std::string bytes; // 按原输入顺序拼接的修正片段
    bool restoreNames; // 是否需要还原有非 ASCII 首字符的属性名或空注释

    /** 建立对原输入的借用。
    @param [in] input 完整输入起点
    */
    explicit XmlInput(const char* input) : remaining(input), restoreNames(false)
    {
    }

    /** 拼入待替换区间之前的原文以及替换文本。
    @param [in] start 替换起点
    @param [in] end 替换结束位置，可等于起点
    @param [in] replacement 替换字节
    */
    void replace(const char* start, const char* end, const char* replacement)
    {
        bytes.append(remaining, start);
        bytes.append(replacement);
        remaining = end;
    }
};

/** 校验数值字符引用并读取完整分号，避免 Mini-XML 接受部分数字或非法码点。
@param [in,out] pos 输入为井号之后的位置，成功时移至分号之后
@param [in,out] prepared 必要时缩短冗长的前导零，以适配上游的实体缓冲区
@return 完整的 XML 1.0 数值引用返回 true
*/
static bool ReadCharacterReference(const char*& pos, XmlInput& prepared)
{
    const unsigned base = *pos == 'x' ? 16 : 10;
    if (base == 16)
    {
        ++pos;
    }
    const char* start = pos;
    uint32_t c = 0;
    for (; *pos != ';'; ++pos)
    {
        unsigned digit;
        if (*pos >= '0' && *pos <= '9')
        {
            digit = *pos - '0';
        }
        else if (*pos >= 'a' && *pos <= 'f')
        {
            digit = *pos - 'a' + 10;
        }
        else if (*pos >= 'A' && *pos <= 'F')
        {
            digit = *pos - 'A' + 10;
        }
        else
        {
            return false;
        }
        if (digit >= base)
        {
            return false;
        }
        c = c * base + digit;
        if (c > 0x10ffff)
        {
            return false;
        }
    }
    if (pos == start)
    {
        return false;
    }
    // 上游仅缓冲 63 个实体字符。合法非零码点至多七个十进制数字。
    if (pos - start > 60)
    {
        prepared.replace(start, start + std::strspn(start, "0"), "");
    }
    ++pos;
    return c == 9 || c == 10 || c == 13 || (c >= 0x20 && c <= 0xd7ff) ||
        (c >= 0xe000 && c <= 0xfffd) || c >= 0x10000;
}

/** 读取单个完整属性，校验名称、引号和数值引用，归一字面空白。
@param [in,out] pos 属性名称起点，成功移到结束引号之后
@param [in,out] prepared 修正后的解析输入
@param [out] nameLength 属性名称的 UTF-8 字节数
@return 属性有效返回 true
*/
static bool ReadXmlAttribute(const char*& pos, XmlInput& prepared, size_t& nameLength)
{
    const char* name = pos;
    pos += std::strcspn(pos, "= \t\n/>");
    nameLength = pos - name;
    if (!ValidXmlRange(name, nameLength, true))
    {
        return false;
    }
    if (static_cast<unsigned char>(*name) >= 128)
    {
        // 名称首字符不能是数字；前置 0 防止上游截断非 ASCII 首码点。
        prepared.replace(name, name, "0");
        prepared.restoreNames = true;
    }
    pos += std::strspn(pos, " \t\n");
    if (*pos++ != '=')
    {
        return false;
    }
    pos += std::strspn(pos, " \t\n");
    const char quote = *pos++;
    if (quote != '\'' && quote != '"')
    {
        return false;
    }
    while (*pos != quote)
    {
        if (!*pos || *pos == '<')
        {
            return false;
        }
        if (*pos == '&' && pos[1] == '#')
        {
            pos += 2;
            if (!ReadCharacterReference(pos, prepared))
            {
                return false;
            }
        }
        else
        {
            if (*pos == '\t' || *pos == '\n')
            {
                prepared.replace(pos, pos + 1, " ");
            }
            ++pos;
        }
    }
    ++pos;
    return true;
}

/** 读取普通元素的开始、自闭合或结束标记。
@param [in,out] pos 左尖括号之后的位置，成功移至标记之后
@param [in,out] prepared 修正后的解析输入
@return 标记语法正确返回 true
*/
static bool ReadXmlTag(const char*& pos, XmlInput& prepared)
{
    const bool close = *pos == '/';
    const char* name = close ? ++pos : pos;
    pos += std::strcspn(pos, " \t\n/>");
    if (!ValidXmlRange(name, pos - name, true))
    {
        return false;
    }
    const char* firstAttribute = nullptr;
    size_t firstLength = 0;
    std::unique_ptr<std::set<std::string>> attributes;
    while (*pos)
    {
        const char* gap = pos;
        pos += std::strspn(pos, " \t\n");
        if (*pos == '>' || (!close && *pos == '/' && pos[1] == '>'))
        {
            break;
        }
        const char* attribute = pos;
        size_t length = 0;
        if (close || gap == pos || !ReadXmlAttribute(pos, prepared, length))
        {
            return false;
        }
        if (!firstAttribute)
        {
            firstAttribute = attribute;
            firstLength = length;
        }
        else
        {
            // 单属性元素无需另行分配；多属性元素先拒绝重名，避开上游损坏的重复属性诊断。
            if (!attributes)
            {
                attributes.reset(new std::set<std::string>());
                attributes->insert(std::string(firstAttribute, firstLength));
            }
            if (!attributes->insert(std::string(attribute, length)).second)
            {
                return false;
            }
        }
    }
    if (!*pos)
    {
        return false;
    }
    pos += *pos == '/' ? 2 : 1;
    return true;
}

/** 读取 UTF-8 XML 1.0 声明，字段按 version、encoding、standalone 的顺序出现。
@param [in,out] pos xml 之后的位置，成功移至结束问号
@return 字段及值受支持时返回 true
*/
static bool ReadXmlDeclaration(const char*& pos)
{
    const char* const fields[] = {"version", "encoding", "standalone"};
    for (int i = 0; i < 3; ++i)
    {
        const char* field = pos + std::strspn(pos, " \t\n");
        const size_t length = std::strlen(fields[i]);
        if (pos == field || std::strncmp(field, fields[i], length))
        {
            if (i == 0)
            {
                return false;
            }
            continue;
        }
        pos = field + length;
        pos += std::strspn(pos, " \t\n");
        if (*pos++ != '=')
        {
            return false;
        }
        pos += std::strspn(pos, " \t\n");
        const char quote = *pos++;
        if (quote != '\'' && quote != '"')
        {
            return false;
        }
        const char* value = pos;
        pos = std::strchr(pos, quote);
        if (!pos)
        {
            return false;
        }
        const size_t n = pos++ - value;
        if ((i == 0 && (n != 3 || std::strncmp(value, "1.0", 3))) ||
            (i == 1 && (n != 5 || (value[0] | 32) != 'u' || (value[1] | 32) != 't' ||
                       (value[2] | 32) != 'f' || value[3] != '-' || value[4] != '8')) ||
            (i == 2 && !((n == 3 && !std::strncmp(value, "yes", 3)) || (n == 2 && !std::strncmp(value, "no", 2)))))
        {
            return false;
        }
    }
    pos += std::strspn(pos, " \t\n");
    return *pos == '?' && pos[1] == '>';
}

/** 跳过单个注释、CDATA、处理指令或文档类型声明。
@param [in,out] pos 左尖括号之后的位置，成功移至标记之后
@param [in] source 已移除文首 BOM 的输入起点
@param [in,out] prepared 修正后的解析输入
@param [in,out] declaration 是否检出文首 XML 声明
@return 标记完整且可交给 Mini-XML 时返回 true
*/
static bool ReadXmlDirective(const char*& pos, const char* source, XmlInput& prepared, bool& declaration)
{
    const char* tag = pos - 1;
    if (!std::strncmp(pos, "!--", 3))
    {
        pos = std::strstr(pos + 3, "--");
        if (!pos || pos[2] != '>')
        {
            return false;
        }
        pos += 3;
        if (pos - tag == 7)
        {
            // 合法输入没有名为 0 的元素，借此保留 Mini-XML 3.1 读不进的空注释。
            prepared.replace(tag, pos, "<0/>");
            prepared.restoreNames = true;
        }
    }
    else if (!std::strncmp(pos, "![CDATA[", 8))
    {
        pos = std::strstr(pos + 8, "]]>");
        if (!pos)
        {
            return false;
        }
        pos += 3;
    }
    else if (*pos == '?')
    {
        const char* target = ++pos;
        pos += std::strcspn(pos, " \t\n?>");
        if (!ValidXmlRange(target, pos - target, true))
        {
            return false;
        }
        if (*pos == '>' || (pos[0] == '?' && pos[1] != '>'))
        {
            return false;
        }
        if (pos - target == 3 && (target[0] | 32) == 'x' && (target[1] | 32) == 'm' && (target[2] | 32) == 'l')
        {
            if (tag != source || std::strncmp(target, "xml", 3) || !ReadXmlDeclaration(pos))
            {
                return false;
            }
            declaration = true;
        }
        pos = std::strstr(pos, "?>");
        if (!pos)
        {
            return false;
        }
        pos += 2;
    }
    else
    {
        // Mini-XML 可保存简单的 DOCTYPE；它不处理 DTD 内部子集。
        if (std::strncmp(pos, "!DOCTYPE", 8) || !pos[8] || !std::strchr(" \t\n", pos[8]))
        {
            return false;
        }
        pos = std::strchr(pos + 8, '>');
        if (!pos)
        {
            return false;
        }
        ++pos;
    }

    return true;
}

/** 检查标记边界和名称，将 XML 属性中的字面制表与换行归一，建树和标签配对由 Mini-XML 完成。
@param [in] source 已按 XML 1.0 归一行尾并移除文首 BOM 的完整字符串
@param [in,out] prepared 按需保存上游解析所需的局部改写
@param [out] declaration 是否以 XML 声明开始
@return 输入能交给 Mini-XML 时返回 true
*/
static bool PrepareXml(const char* source, XmlInput& prepared, bool& declaration)
{
    size_t depth = 0;
    bool hasRoot = false;
    bool hasDoctype = false;
    const char* pos = source;
    while (*pos)
    {
        if (*pos == '<')
        {
            ++pos;
            if (*pos == '!' || *pos == '?')
            {
                if (!std::strncmp(pos, "!DOCTYPE", 8))
                {
                    if (hasRoot || hasDoctype)
                    {
                        return false;
                    }
                    hasDoctype = true;
                }
                if ((depth == 0 && !std::strncmp(pos, "![CDATA[", 8)) ||
                    !ReadXmlDirective(pos, source, prepared, declaration))
                {
                    return false;
                }
            }
            else
            {
                const bool close = *pos == '/';
                if (!ReadXmlTag(pos, prepared))
                {
                    return false;
                }
                if (close)
                {
                    if (!depth)
                    {
                        return false;
                    }
                    --depth;
                }
                else
                {
                    if (!depth && hasRoot)
                    {
                        return false;
                    }
                    hasRoot = true;
                    depth += pos[-2] != '/' ? 1 : 0;
                }
            }
        }
        else if (!depth)
        {
            // Mini-XML 会忽略未提交的尾部文本，文档层只允许字面 XML 空白。
            const size_t space = std::strspn(pos, " \t\n");
            if (!space)
            {
                return false;
            }
            pos += space;
        }
        else if (*pos == '&' && pos[1] == '#')
        {
            pos += 2;
            if (!ReadCharacterReference(pos, prepared))
            {
                return false;
            }
        }
        else if (*pos == ']' && pos[1] == ']' && pos[2] == '>')
        {
            return false;
        }
        else
        {
            ++pos;
        }
    }
    if (!prepared.bytes.empty())
    {
        prepared.bytes.append(prepared.remaining, pos);
    }
    return hasRoot && depth == 0;
}

/** 还原输入中临时添加的数字标记；普通 XML 名称不可能以数字开头。
@param [in,out] doc 尚未交付的临时文档
@return 恢复成功返回 true
*/
static bool RestoreXmlNames(mxml_node_t* doc)
{
    for (mxml_node_t* node = mxmlGetFirstChild(doc); node; node = mxmlWalkNext(node, doc, MXML_DESCEND))
    {
        const char* name = mxmlGetElement(node);
        if (name && !std::strcmp(name, "0") && mxmlSetElement(node, "!----"))
        {
            return false;
        }
        for (int i = mxmlElementGetAttrCount(node); i-- > 0; )
        {
            const char* value = mxmlElementGetAttrByIndex(node, i, &name);
            if (name && name[0] == '0')
            {
                mxmlElementSetAttr(node, name + 1, value);
                if (!mxmlElementGetAttr(node, name + 1))
                {
                    return false;
                }
                mxmlElementDeleteAttr(node, name);
            }
        }
    }
    return true;
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
    if (!ValidXmlString(xml, false))
    {
        return false;
    }
    const size_t bom = xml.compare(0, 3, "\xef\xbb\xbf") == 0 ? 3 : 0;
    const char* source = xml.c_str() + bom;
    std::string lines;
    // XML 的 CR 和 CRLF 在解析实体前统一为 LF，适用于文本、CDATA 和属性。
    if (std::strchr(source, '\r'))
    {
        lines.reserve(xml.size() - bom);
        for (; *source; ++source)
        {
            if (*source == '\r')
            {
                lines.push_back('\n');
                source += source[1] == '\n' ? 1 : 0;
            }
            else
            {
                lines.push_back(*source);
            }
        }
        source = lines.c_str();
    }
    bool declaration = false;
    XmlInput prepared(source);
    if (!PrepareXml(source, prepared, declaration))
    {
        return false;
    }
    if (!prepared.bytes.empty())
    {
        source = prepared.bytes.c_str();
    }
    // Mini-XML 的 UTF-8 读取器会吞掉文中的 U+FEFF，UTF-16LE 读取器会按字符保留。
    std::string utf16;
    if (std::strstr(source, "\xef\xbb\xbf"))
    {
        utf16.assign("\xff\xfe");
        ValidXmlRange(source, std::strlen(source), false, &utf16);
        utf16.push_back('\0');
        source = utf16.c_str();
    }
    // 解析到不可能由输入闭合的容器，Mini-XML 才会检查裸根元素也已闭合。
    std::unique_ptr<mxml_node_t, decltype(&mxmlDelete)> doc(CheckedElement(mxmlNewElement(nullptr, " ")), &mxmlDelete);
    if (!doc || mxmlLoadString(doc.get(), source, MXML_OPAQUE_CALLBACK) != doc.get() ||
        (prepared.restoreNames && !RestoreXmlNames(doc.get())))
    {
        return false;
    }
    mxml_node_t* root = FirstElement(doc.get());
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

/** 将 Mini-XML 的输出追加到文档，避免重读时按 XML 的空白规则改写属性值和文本回车。
@param [in,out] result 序列化结果
@param [in] xml Mini-XML 刚写出的、以 NUL 结尾的完整节点
*/
static void AppendSavedXml(std::string& result, const char* xml)
{
    const char* space = std::strpbrk(xml, "\r\n\t");
    if (!space || (*space == '\n' && !space[1]))
    {
        result.append(xml);
        return;
    }
    const char* start = xml;
    bool tag = false;
    bool quoted = false;
    for (const char* pos = xml; *pos; ++pos)
    {
        if (*pos == '<')
        {
            // 声明、注释、处理指令和 CDATA 的正文均按上游原样保存。
            if (pos[1] == '!' || pos[1] == '?')
            {
                const char* end = ">";
                if (pos[1] == '?')
                {
                    end = "?>";
                }
                else if (std::strncmp(pos, "<!--", 4) == 0)
                {
                    end = "-->";
                }
                else if (std::strncmp(pos, "<![CDATA[", 9) == 0)
                {
                    end = "]]>";
                }
                const char* close = std::strstr(pos + 2, end);
                if (!close)
                {
                    break;
                }
                pos = close + std::strlen(end) - 1;
                continue;
            }
            tag = true;
        }
        else if (tag && *pos == '"')
        {
            // Mini-XML 总是以双引号包住属性值，并将值内的双引号和尖括号转义。
            quoted = !quoted;
        }
        else if (tag && *pos == '>')
        {
            tag = false;
        }
        else if ((quoted && (*pos == '\t' || *pos == '\n')) || (*pos == '\r' && (quoted || !tag)))
        {
            result.append(start, static_cast<size_t>(pos - start));
            result.append(*pos == '\t' ? "&#9;" : (*pos == '\n' ? "&#10;" : "&#13;"));
            start = pos + 1;
        }
    }
    result.append(start);
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
                AppendSavedXml(result, data.get());
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