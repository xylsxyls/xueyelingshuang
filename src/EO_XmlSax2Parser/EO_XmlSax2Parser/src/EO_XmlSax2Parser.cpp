#include "EO_XmlSax2Parser.h"
#include <QFile>
#include <QIODevice>
#include <QXmlStreamAttribute>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <limits>
#include <stdint.h>

EO_XmlSax2Parser::EO_XmlSax2Parser() :
m_lastError()
{

}

EO_XmlSax2Parser::~EO_XmlSax2Parser()
{

}

bool EO_XmlSax2Parser::parseFile(const QString& filename, EO_XmlSax2Handler* handler)
{
    m_lastError.clear();
    if (handler == nullptr)
    {
        m_lastError = QStringLiteral("XML parse failed: handler is null");
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        m_lastError = QStringLiteral("XML file open failed: %1").arg(filename);
        handler->error(m_lastError, 0, 0);
        return false;
    }

    QXmlStreamReader reader(&file);
    bool success = parseReader(&reader, handler);
    file.close();
    return success;
}

bool EO_XmlSax2Parser::parseData(const QString& data, EO_XmlSax2Handler* handler)
{
    m_lastError.clear();
    if (handler == nullptr)
    {
        m_lastError = QStringLiteral("XML parse failed: handler is null");
        return false;
    }

    QXmlStreamReader reader(data);
    return parseReader(&reader, handler);
}

QString EO_XmlSax2Parser::lastError() const
{
    return m_lastError;
}

bool EO_XmlSax2Parser::parseReader(QXmlStreamReader* reader, EO_XmlSax2Handler* handler)
{
    if (reader == nullptr)
    {
        m_lastError = QStringLiteral("XML parse failed: reader is null");
        if (handler != nullptr)
        {
            handler->error(m_lastError, 0, 0);
        }
        return false;
    }

	if (handler == nullptr)
	{
		m_lastError = QStringLiteral("XML parse failed: handler is null");
		return false;
	}

	// clampXmlLocation入参：value是Qt返回的XML行号或列号。
	// clampXmlLocation出参：无。
	// clampXmlLocation返回值：返回限制到int32_t范围内的错误位置。
	auto clampXmlLocation = [](qint64 value) -> int32_t
	{
		const qint64 maxValue = static_cast<qint64>((std::numeric_limits<int32_t>::max)());
		if (value > maxValue)
		{
			return static_cast<int32_t>(maxValue);
		}
		if (value < 0)
		{
			return 0;
		}
		return static_cast<int32_t>(value);
	};

    while (!reader->atEnd())
    {
        QXmlStreamReader::TokenType tokenType = reader->readNext();
        if (tokenType == QXmlStreamReader::StartElement)
        {
            QString elementName = reader->name().toString();
            XMLAttributes attributes = readAttributes(*reader);
            if (!handler->startElement(elementName, attributes))
            {
                m_lastError = QStringLiteral("XML parse stopped by startElement handler, element: %1, line: %2, column: %3")
                    .arg(elementName)
                    .arg(reader->lineNumber())
                    .arg(reader->columnNumber());
                return false;
            }
        }
        else if (tokenType == QXmlStreamReader::Characters)
        {
            if (!handler->characters(reader->text().toString()))
            {
                m_lastError = QStringLiteral("XML parse stopped by characters handler, line: %1, column: %2")
                    .arg(reader->lineNumber())
                    .arg(reader->columnNumber());
                return false;
            }
        }
        else if (tokenType == QXmlStreamReader::EndElement)
        {
            QString elementName = reader->name().toString();
            if (!handler->endElement(elementName))
            {
                m_lastError = QStringLiteral("XML parse stopped by endElement handler, element: %1, line: %2, column: %3")
                    .arg(elementName)
                    .arg(reader->lineNumber())
                    .arg(reader->columnNumber());
                return false;
            }
        }
    }

    if (reader->hasError())
    {
        m_lastError = QStringLiteral("XML parse failed: %1, line: %2, column: %3")
            .arg(reader->errorString())
            .arg(reader->lineNumber())
            .arg(reader->columnNumber());
        handler->error(m_lastError, clampXmlLocation(reader->lineNumber()), clampXmlLocation(reader->columnNumber()));
        return false;
    }

    return true;
}

XMLAttributes EO_XmlSax2Parser::readAttributes(const QXmlStreamReader& reader) const
{
    XMLAttributes attributes;
    QXmlStreamAttributes streamAttributes = reader.attributes();
    for (int32_t i = 0; i < streamAttributes.count(); ++i)
    {
        const QXmlStreamAttribute& attribute = streamAttributes[i];
        attributes << XMLAttribute(attribute.name().toString(), attribute.value().toString());
    }
    return attributes;
}

EO_XmlSax2Parser::EO_XmlSax2Parser(const EO_XmlSax2Parser& other) :
m_lastError()
{
    Q_UNUSED(other);
}

EO_XmlSax2Parser& EO_XmlSax2Parser::operator=(const EO_XmlSax2Parser& other)
{
    Q_UNUSED(other);
    return *this;
}