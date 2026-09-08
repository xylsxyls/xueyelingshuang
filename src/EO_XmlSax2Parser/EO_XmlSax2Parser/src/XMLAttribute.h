#ifndef XMLATTRIBUTE_H
#define XMLATTRIBUTE_H

#include "EO_XmlSax2ParserMacro.h"
#include <QString>
#include <QVector>

/** XML节点属性，保存流式解析时当前节点携带的属性名和属性值。
*/
struct EO_XmlSax2ParserAPI XMLAttribute
{
    /** 构造空属性对象。
    */
    XMLAttribute();

    /** 构造属性对象并设置名称和值。
    @param [in] attributeName 属性名称
    @param [in] attributeValue 属性值
    */
    XMLAttribute(const QString& attributeName, const QString& attributeValue);

    // 属性名称
    QString m_name;

    // 属性值
    QString m_value;
};

typedef QVector<XMLAttribute> XMLAttributes;

#endif // XMLATTRIBUTE_H