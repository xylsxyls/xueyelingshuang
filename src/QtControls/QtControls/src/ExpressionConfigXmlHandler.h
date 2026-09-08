#ifndef EXPRESSIONCONFIGXMLHANDLER_H
#define EXPRESSIONCONFIGXMLHANDLER_H

#include "ExpressionConfig.h"
#include "EO_XmlSax2Parser/EO_XmlSax2ParserAPI.h"
#include <QString>
#include <stdint.h>

/** 表情XML流式事件处理器，把emotion.xml中的group和emotion节点转换成表情配置结果。
*/
class ExpressionConfigXmlHandler : public EO_XmlSax2Handler
{
public:
    /** 构造表情XML流式处理器。
    @param [out] config 表情配置输出对象，不能为nullptr
    @param [in] emotionRootPath 表情资源根目录
    */
    ExpressionConfigXmlHandler(ExpressionConfig* config, const QString& emotionRootPath);

    /** 析构函数。
    */
    ~ExpressionConfigXmlHandler();

public:
    /** 处理XML开始节点事件。
    @param [in] elementName 节点名称
    @param [in] attributes 节点属性列表，emotion.xml当前不依赖属性
    @return 返回true表示继续解析
    */
    bool startElement(const QString& elementName, const XMLAttributes& attributes);

    /** 处理XML结束节点事件。
    @param [in] elementName 节点名称
    @return 返回true表示继续解析
    */
    bool endElement(const QString& elementName);

    /** 处理XML字符文本事件。
    @param [in] text 本次解析出的文本片段
    @return 返回true表示继续解析
    */
    bool characters(const QString& text);

    /** 处理XML解析错误事件。
    @param [in] message 错误信息
    @param [in] line 错误行号
    @param [in] column 错误列号
    @return 返回false表示解析失败
    */
    bool error(const QString& message, int32_t line, int32_t column);

    /** 获取最近一次失败原因。
    @return 返回错误文本
    */
    QString lastError() const;

    /** 判断是否已经读取到config根节点。
    @return 返回true表示已读取到config根节点
    */
    bool hasConfigRoot() const;

private:
    enum ParseRecordType
    {
        // 当前不在group或emotion记录内
        kRecordNone,
        // 当前正在解析group记录
        kRecordGroup,
        // 当前正在解析emotion记录
        kRecordEmotion
    };

private:
    /** 完成一个group节点的解析并写入输出结果。
    */
    void finishGroup();

    /** 完成一个emotion节点的解析并写入输出结果。
    */
    void finishExpression();

    /** 将当前字段文本写入临时记录。
    @param [in] fieldName 字段名称
    @param [in] fieldValue 字段值
    */
    void assignField(const QString& fieldName, const QString& fieldValue);

    /** 根据历史默认规则补充分组代表图。
    @param [in,out] group 表情分组配置
    */
    void fillDefaultGroupImage(ExpressionGroupInfo* group);

    /** 生成表情资源完整路径。
    @param [in] relativePath 表情资源相对路径
    @return 返回完整路径
    */
    QString buildResourcePath(const QString& relativePath) const;

private:
    /** 禁止拷贝构造，避免输出对象指针被误复制。
    @param [in] other 被拷贝对象
    */
    ExpressionConfigXmlHandler(const ExpressionConfigXmlHandler& other);

    /** 禁止赋值，避免输出对象指针被误复制。
    @param [in] other 被赋值对象
    @return 返回当前对象引用
    */
    ExpressionConfigXmlHandler& operator=(const ExpressionConfigXmlHandler& other);

private:
    // 表情配置输出对象，不负责释放
    ExpressionConfig* m_config;

    // 表情资源根目录
    QString m_emotionRootPath;

    // 当前正在解析的XML深度
    int32_t m_depth;

    // 是否已经读取到config根节点
    bool m_hasConfigRoot;

    // 当前正在解析的记录类型
    ParseRecordType m_recordType;

    // 当前正在解析的字段名称
    QString m_currentFieldName;

    // 当前字段累计文本，SAX文本可能分多次回调
    QString m_currentText;

    // 正在解析的分组临时对象
    ExpressionGroupInfo m_currentGroup;

    // 正在解析的表情临时对象
    ExpressionInfo m_currentExpression;

    // 最近一次失败原因
    QString m_lastError;
};

#endif // EXPRESSIONCONFIGXMLHANDLER_H