#include "EO_XmlSax2Handler.h"

EO_XmlSax2Handler::~EO_XmlSax2Handler()
{

}

bool EO_XmlSax2Handler::startElement(const QString& elementName, const XMLAttributes& attributes)
{
    Q_UNUSED(elementName);
    Q_UNUSED(attributes);
    return true;
}

bool EO_XmlSax2Handler::endElement(const QString& elementName)
{
    Q_UNUSED(elementName);
    return true;
}

bool EO_XmlSax2Handler::characters(const QString& text)
{
    Q_UNUSED(text);
    return true;
}

bool EO_XmlSax2Handler::error(const QString& message, int32_t line, int32_t column)
{
    Q_UNUSED(message);
    Q_UNUSED(line);
    Q_UNUSED(column);
    return false;
}
