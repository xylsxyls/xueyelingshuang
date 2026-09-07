#ifndef EO_XMLSAX2HANDLER_H
#define EO_XMLSAX2HANDLER_H

#include "EO_XmlSax2ParserMacro.h"
#include "XMLAttribute.h"
#include <QString>
#include <stdint.h>

/** XML流式解析事件处理接口，调用方按需消费节点事件而不必构建完整XML树。
*/
class EO_XmlSax2ParserAPI EO_XmlSax2Handler
{
public:
    /** 析构函数。
    */
    virtual ~EO_XmlSax2Handler();

public:
    /** 处理开始节点事件。
    @param [in] elementName 节点名称
    @param [in] attributes 当前节点属性列表
    @return 返回true表示继续解析，返回false表示停止解析
    */
    virtual bool startElement(const QString& elementName, const XMLAttributes& attributes);

    /** 处理结束节点事件。
    @param [in] elementName 节点名称
    @return 返回true表示继续解析，返回false表示停止解析
    */
    virtual bool endElement(const QString& elementName);

    /** 处理节点文本事件，同一节点可能收到多次文本回调。
    @param [in] text 本次解析出的文本片段
    @return 返回true表示继续解析，返回false表示停止解析
    */
    virtual bool characters(const QString& text);

    /** 处理解析错误。
    @param [in] message 错误文本
    @param [in] line 错误所在行号
    @param [in] column 错误所在列号
    @return 返回true表示调用方已处理错误，返回false表示解析失败
    */
    virtual bool error(const QString& message, int32_t line, int32_t column);
};

#endif // EO_XMLSAX2HANDLER_H
