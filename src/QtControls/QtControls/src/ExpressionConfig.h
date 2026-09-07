#ifndef EXPRESSIONCONFIG_H
#define EXPRESSIONCONFIG_H

#include <QString>
#include <QVector>
#include "QtControlsMacro.h"

/** 表情分组配置，保存一组表情在界面上需要展示的基础信息。
*/
struct QtControlsAPI ExpressionGroupInfo
{
    /** 构造表情分组配置，所有字段默认为空。
    */
    ExpressionGroupInfo();

    // 分组ID，对应emotion.xml中的group/id
    QString m_groupId;

    // 分组描述文本
    QString m_desc;

    // 分组代表图片完整路径
    QString m_fileName;

    // 分组悬停提示文本
    QString m_tooltip;
};

typedef QVector<ExpressionGroupInfo> ExpressionGroupInfoList;

/** 表情项配置，保存单个表情文件、快捷码和分组关系。
*/
struct QtControlsAPI ExpressionInfo
{
    /** 构造表情项配置，所有字段默认为空。
    */
    ExpressionInfo();

    /** 判断表情项是否具备最小可用字段。
    @return 返回true表示包含分组ID、文件路径和快捷码
    */
    bool isValid() const;

    // 所属分组ID，对应emotion.xml中的emotion/groupid
    QString m_groupId;

    // 表情ID
    QString m_id;

    // 表情描述文本
    QString m_desc;

    // 表情文件完整路径
    QString m_fileName;

    // 表情快捷码
    QString m_shortcut;

    // 表情悬停提示文本
    QString m_tooltip;
};

typedef QVector<ExpressionInfo> ExpressionInfoList;

/** 表情配置解析结果，集中保存分组和表情列表。
*/
struct QtControlsAPI ExpressionConfig
{
    /** 构造表情配置结果，默认列表为空。
    */
    ExpressionConfig();

    /** 清空表情分组和表情列表。
    */
    void clear();

    // 表情分组列表
    ExpressionGroupInfoList m_groups;

    // 表情项列表
    ExpressionInfoList m_expressions;
};

#endif // EXPRESSIONCONFIG_H
