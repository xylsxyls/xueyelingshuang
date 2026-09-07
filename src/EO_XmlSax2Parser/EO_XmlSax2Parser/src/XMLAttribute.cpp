#include "XMLAttribute.h"

XMLAttribute::XMLAttribute() :
m_name(),
m_value()
{

}

XMLAttribute::XMLAttribute(const QString& attributeName, const QString& attributeValue) :
m_name(attributeName),
m_value(attributeValue)
{

}
