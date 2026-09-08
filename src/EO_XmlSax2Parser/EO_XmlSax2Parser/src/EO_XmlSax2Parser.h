#ifndef EO_XMLSAX2PARSER_H
#define EO_XMLSAX2PARSER_H

#include "EO_XmlSax2Handler.h"

class QXmlStreamReader;

/** XML流式解析封装，按节点事件逐步通知调用方，不在库内构建完整XML树。
*/
class EO_XmlSax2ParserAPI EO_XmlSax2Parser
{
public:
    /** 构造XML解析器。
    */
    explicit EO_XmlSax2Parser();

    /** 析构XML解析器。
    */
    ~EO_XmlSax2Parser();

public:
    /** 按流式方式解析XML文件。
    @param [in] filename XML文件路径
    @param [in,out] handler 流式事件处理对象，不能为nullptr
    @return 返回true表示解析成功
    */
    bool parseFile(const QString& filename, EO_XmlSax2Handler* handler);

    /** 按流式方式解析XML文本。
    @param [in] data XML文本
    @param [in,out] handler 流式事件处理对象，不能为nullptr
    @return 返回true表示解析成功
    */
    bool parseData(const QString& data, EO_XmlSax2Handler* handler);

    /** 获取最近一次解析失败原因。
    @return 返回错误文本
    */
    QString lastError() const;

private:
    /** 解析Qt XML流并转发节点事件。
    @param [in,out] reader Qt XML流读取器
    @param [in,out] handler 流式事件处理对象，不能为nullptr
    @return 返回true表示解析成功
    */
    bool parseReader(QXmlStreamReader* reader, EO_XmlSax2Handler* handler);

    /** 从当前开始节点读取属性列表。
    @param [in] reader 已定位到开始节点的Qt XML流读取器
    @return 返回当前节点的属性列表
    */
    XMLAttributes readAttributes(const QXmlStreamReader& reader) const;

    /** 禁止拷贝构造，避免解析器状态被误复制。
    @param [in] other 被拷贝对象
    */
    EO_XmlSax2Parser(const EO_XmlSax2Parser& other);

    /** 禁止赋值，避免解析器状态被误复制。
    @param [in] other 被赋值对象
    @return 返回当前对象引用
    */
    EO_XmlSax2Parser& operator=(const EO_XmlSax2Parser& other);

private:
    // 最近一次解析失败原因
    QString m_lastError;
};

#endif // EO_XMLSAX2PARSER_H