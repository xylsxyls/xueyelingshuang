#include "ExpressionConfigParser.h"
#include "ExpressionConfigXmlHandler.h"
#include "EO_XmlSax2Parser/EO_XmlSax2ParserAPI.h"

ExpressionConfigParser::ExpressionConfigParser() :
m_lastError()
{

}

ExpressionConfigParser::~ExpressionConfigParser()
{

}

bool ExpressionConfigParser::parse(const QString& emotionRootPath, ExpressionConfig* config)
{
    return parseResult(emotionRootPath, config) == ExpressionLoadSuccess;
}

ExpressionLoadResult ExpressionConfigParser::parseResult(const QString& emotionRootPath, ExpressionConfig* config)
{
    m_lastError.clear();
    if (config == nullptr)
    {
        m_lastError = QStringLiteral("expression config output is null");
        return ExpressionLoadInvalidOutput;
    }

    config->clear();

    EO_XmlSax2Parser parser;
    ExpressionConfigXmlHandler handler(config, emotionRootPath);
    if (!parser.parseFile(configFilePath(emotionRootPath), &handler))
    {
        m_lastError = handler.lastError();
        if (m_lastError.isEmpty())
        {
            m_lastError = parser.lastError();
        }
        return ExpressionLoadParseFailed;
    }

    if (!handler.hasConfigRoot())
    {
        m_lastError = QStringLiteral("expression XML missing config root element");
        return ExpressionLoadMissingRoot;
    }

    if (config->m_groups.isEmpty() && config->m_expressions.isEmpty())
    {
        m_lastError = QStringLiteral("expression config is empty");
        return ExpressionLoadEmptyConfig;
    }

    return ExpressionLoadSuccess;
}

QString ExpressionConfigParser::lastError() const
{
    return m_lastError;
}

QString ExpressionConfigParser::configFilePath(const QString& emotionRootPath) const
{
    QString rootPath = emotionRootPath;
    rootPath.replace("\\", "/");
    while (rootPath.endsWith("/"))
    {
        rootPath.chop(1);
    }
    return rootPath + QStringLiteral("/Emotions/emotion.xml");
}

ExpressionConfigParser::ExpressionConfigParser(const ExpressionConfigParser& other) :
m_lastError()
{
    Q_UNUSED(other);
}

ExpressionConfigParser& ExpressionConfigParser::operator=(const ExpressionConfigParser& other)
{
    Q_UNUSED(other);
    return *this;
}