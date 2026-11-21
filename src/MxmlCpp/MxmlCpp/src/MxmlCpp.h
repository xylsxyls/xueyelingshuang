#pragma once
#include "MxmlCppMacro.h"
#include <string>

struct _mxml_node_s;
typedef struct _mxml_node_s mxml_node_t;

/** 拥有一个 Mini-XML 文档和一个元素游标。一个对象由一个线程使用。 */
class MxmlCppAPI MxmlCpp
{
public:
    /** 建立空对象。 */
    MxmlCpp();

    /** 释放拥有的整棵树。 */
    ~MxmlCpp();

    /** 文档唯一拥有，禁用复制构造。
    @param [in] other 其他文档
    */
    MxmlCpp(const MxmlCpp& other) = delete;

    /** 文档唯一拥有，禁用复制赋值。
    @param [in] other 其他文档
    @return 当前对象
    */
    MxmlCpp& operator=(const MxmlCpp& other) = delete;

    /** 读取 UTF-8 文档，成功后替换整棵树。带 XML 声明时停在文档层，否则停在根元素。
    @param [in] xml 完整 XML 字节，允许 UTF-8 BOM
    @return 成功返回 true，失败保留原树和游标
    */
    bool load(const std::string& xml);

    /** 创建带 XML 1.0 声明的空文档，游标停在文档层。
    @return 成功返回 true，失败保留现状
    */
    bool create();

    /** 向当前元素追加子元素，文档层只允许一个根元素。游标位置保持。
    @param [in] name UTF-8 XML 名称
    @return 成功返回 true
    */
    bool addChild(const std::string& name);

    /** 追加带直接文本的子元素，自动转义文本，游标位置保持。
    @param [in] name UTF-8 XML 名称
    @param [in] text UTF-8 文本，允许空串
    @return 成功返回 true
    */
    bool addChild(const std::string& name, const std::string& text);

    /** 写入或替换当前元素的属性。
    @param [in] name 属性的 XML 名称
    @param [in] attr UTF-8 属性值，允许空串
    @return 成功返回 true
    */
    bool addAttr(const std::string& name, const std::string& attr);

    /** 按文档顺序进入首个匹配的直接子元素。
    @param [in] element 元素名，空表示任意名称
    @param [in] attr 属性名，空表示不过滤属性
    @param [in] value 非空时要求属性值匹配，空时只要求属性存在
    @return 有匹配返回 true，失败保持游标
    */
    bool firstChild(const std::string& element = "", const std::string& attr = "", const std::string& value = "");

    /** 进入上一个同级元素。
    @return 成功返回 true，失败保持游标
    */
    bool pre();

    /** 进入下一个同级元素。
    @return 成功返回 true，失败保持游标
    */
    bool next();

    /** 进入父元素，根元素的上一级为文档层。
    @return 成功返回 true，失败保持游标
    */
    bool parent();

    /** 读取当前元素名称。
    @return 名称副本，文档层或空对象返回空串
    */
    std::string element();

    /** 读取当前元素的属性。
    @param [in] name 属性名称
    @return 属性值副本，缺失时返回空串
    */
    std::string attr(const std::string& name);

    /** 连接当前元素的直接文本及 CDATA，保持空白，不递归读取子元素。
    @return 文本副本，无内容返回空串
    */
    std::string text();

    /** 序列化完整文档，游标保持。
    @return UTF-8 XML，空对象、缺少根或保存失败返回空串
    */
    std::string toString();

private:
    // 文档容器，唯一拥有声明、根元素及其他顶层节点
    mxml_node_t* m_xml;
    // 借用容器或其中一个元素；替换文档时更新
    mxml_node_t* m_xmlVisit;
};