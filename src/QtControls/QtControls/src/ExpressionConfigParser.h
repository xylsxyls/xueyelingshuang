#ifndef EXPRESSIONCONFIGPARSER_H
#define EXPRESSIONCONFIGPARSER_H

#include "ExpressionConfig.h"

/** 表情配置解析器，隐藏emotion.xml路径规则和XML流式解析细节。
*/
class QtControlsAPI ExpressionConfigParser
{
public:
    /** 构造表情配置解析器。
    */
    ExpressionConfigParser();

    /** 析构函数。
    */
    ~ExpressionConfigParser();

public:
    /** 解析表情资源目录下的Emotions/emotion.xml。
    @param [in] emotionRootPath 表情资源根目录
    @param [out] config 解析结果输出对象，不能为nullptr
    @return 返回true表示解析成功
    */
    bool parse(const QString& emotionRootPath, ExpressionConfig* config);

    /** 获取最近一次失败原因。
    @return 返回错误文本
    */
    QString lastError() const;

private:
    /** 生成emotion.xml完整路径。
    @param [in] emotionRootPath 表情资源根目录
    @return 返回XML文件路径
    */
    QString configFilePath(const QString& emotionRootPath) const;

private:
    /** 禁止拷贝构造，避免错误状态被误复制。
    @param [in] other 被拷贝对象
    */
    ExpressionConfigParser(const ExpressionConfigParser& other);

    /** 禁止赋值，避免错误状态被误复制。
    @param [in] other 被赋值对象
    @return 返回当前对象引用
    */
    ExpressionConfigParser& operator=(const ExpressionConfigParser& other);

private:
    // 最近一次失败原因
    QString m_lastError;
};

#endif // EXPRESSIONCONFIGPARSER_H