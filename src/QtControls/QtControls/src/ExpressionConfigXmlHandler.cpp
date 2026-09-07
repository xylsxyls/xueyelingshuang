#include "ExpressionConfigXmlHandler.h"

ExpressionConfigXmlHandler::ExpressionConfigXmlHandler(ExpressionConfig* config, const QString& emotionRootPath) :
m_config(config),
m_emotionRootPath(emotionRootPath),
m_depth(0),
m_hasConfigRoot(false),
m_recordType(kRecordNone),
m_currentFieldName(),
m_currentText(),
m_currentGroup(),
m_currentExpression(),
m_lastError()
{

}

ExpressionConfigXmlHandler::~ExpressionConfigXmlHandler()
{

}

bool ExpressionConfigXmlHandler::startElement(const QString& elementName, const XMLAttributes& attributes)
{
    Q_UNUSED(attributes);

    if (m_config == nullptr)
    {
        m_lastError = QStringLiteral("expression config output is null");
        return false;
    }

    if (m_depth == 0)
    {
        if (elementName != QStringLiteral("config"))
        {
            m_lastError = QStringLiteral("expression XML root mismatch, expected config, actual: %1").arg(elementName);
            return false;
        }
        m_hasConfigRoot = true;
    }
    else if (m_depth == 1)
    {
        if (elementName == QStringLiteral("group"))
        {
            m_recordType = kRecordGroup;
            m_currentGroup = ExpressionGroupInfo();
        }
        else if (elementName == QStringLiteral("emotion"))
        {
            m_recordType = kRecordEmotion;
            m_currentExpression = ExpressionInfo();
        }
    }
    else if (m_depth == 2 && m_recordType != kRecordNone)
    {
        m_currentFieldName = elementName;
        m_currentText.clear();
    }

    ++m_depth;
    return true;
}

bool ExpressionConfigXmlHandler::endElement(const QString& elementName)
{
    if (m_depth <= 0)
    {
        m_lastError = QStringLiteral("expression XML end element depth is invalid: %1").arg(elementName);
        return false;
    }

    if (m_depth == 3 && m_recordType != kRecordNone && elementName == m_currentFieldName)
    {
        assignField(m_currentFieldName, m_currentText.trimmed());
        m_currentFieldName.clear();
        m_currentText.clear();
    }
    else if (m_depth == 2)
    {
        if (elementName == QStringLiteral("group") && m_recordType == kRecordGroup)
        {
            finishGroup();
            m_recordType = kRecordNone;
        }
        else if (elementName == QStringLiteral("emotion") && m_recordType == kRecordEmotion)
        {
            finishExpression();
            m_recordType = kRecordNone;
        }
    }

    --m_depth;
    return true;
}

bool ExpressionConfigXmlHandler::characters(const QString& text)
{
    if (m_recordType != kRecordNone && !m_currentFieldName.isEmpty())
    {
        m_currentText += text;
    }
    return true;
}

bool ExpressionConfigXmlHandler::error(const QString& message, int32_t line, int32_t column)
{
    m_lastError = QStringLiteral("%1, line: %2, column: %3").arg(message).arg(line).arg(column);
    return false;
}

QString ExpressionConfigXmlHandler::lastError() const
{
    return m_lastError;
}

bool ExpressionConfigXmlHandler::hasConfigRoot() const
{
    return m_hasConfigRoot;
}

void ExpressionConfigXmlHandler::finishGroup()
{
    if (m_config == nullptr || m_currentGroup.m_groupId.isEmpty())
    {
        return;
    }

    fillDefaultGroupImage(&m_currentGroup);
    if (m_currentGroup.m_tooltip.isEmpty())
    {
        m_currentGroup.m_tooltip = m_currentGroup.m_desc;
    }
    m_config->m_groups << m_currentGroup;
}

void ExpressionConfigXmlHandler::finishExpression()
{
    if (m_config == nullptr || !m_currentExpression.isValid())
    {
        return;
    }
    m_config->m_expressions << m_currentExpression;
}

void ExpressionConfigXmlHandler::assignField(const QString& fieldName, const QString& fieldValue)
{
    if (m_recordType == kRecordGroup)
    {
        if (fieldName == QStringLiteral("id"))
        {
            m_currentGroup.m_groupId = fieldValue;
        }
        else if (fieldName == QStringLiteral("desc"))
        {
            m_currentGroup.m_desc = fieldValue;
        }
        else if (fieldName == QStringLiteral("file"))
        {
            m_currentGroup.m_fileName = buildResourcePath(fieldValue);
        }
        else if (fieldName == QStringLiteral("tooltip"))
        {
            m_currentGroup.m_tooltip = fieldValue;
        }
    }
    else if (m_recordType == kRecordEmotion)
    {
        if (fieldName == QStringLiteral("groupid"))
        {
            m_currentExpression.m_groupId = fieldValue;
        }
        else if (fieldName == QStringLiteral("id"))
        {
            m_currentExpression.m_id = fieldValue;
        }
        else if (fieldName == QStringLiteral("desc"))
        {
            m_currentExpression.m_desc = fieldValue;
        }
        else if (fieldName == QStringLiteral("file"))
        {
            m_currentExpression.m_fileName = buildResourcePath(fieldValue);
        }
        else if (fieldName == QStringLiteral("shortcut"))
        {
            m_currentExpression.m_shortcut = fieldValue;
        }
        else if (fieldName == QStringLiteral("tooltip"))
        {
            m_currentExpression.m_tooltip = fieldValue;
        }
    }
}

void ExpressionConfigXmlHandler::fillDefaultGroupImage(ExpressionGroupInfo* group)
{
    if (group == nullptr || !group->m_fileName.isEmpty())
    {
        return;
    }

    if (group->m_groupId == QStringLiteral("0"))
    {
        group->m_fileName = buildResourcePath(QStringLiteral("Emotions/def/def_jingkong.gif"));
    }
    else if (group->m_groupId == QStringLiteral("1"))
    {
        group->m_fileName = buildResourcePath(QStringLiteral("Emotions/7f/71_daxiao.gif"));
    }
    else if (group->m_groupId == QStringLiteral("2"))
    {
        group->m_fileName = buildResourcePath(QStringLiteral("Emotions/Smileys/em4.bmp"));
    }
}

QString ExpressionConfigXmlHandler::buildResourcePath(const QString& relativePath) const
{
    if (relativePath.isEmpty())
    {
        return QString();
    }

    QString rootPath = m_emotionRootPath;
    rootPath.replace("\\", "/");
    while (rootPath.endsWith("/"))
    {
        rootPath.chop(1);
    }

    QString path = relativePath;
    path.replace("\\", "/");
    while (path.startsWith("/"))
    {
        path.remove(0, 1);
    }

    if (rootPath.isEmpty())
    {
        return path;
    }
    return rootPath + "/" + path;
}

ExpressionConfigXmlHandler::ExpressionConfigXmlHandler(const ExpressionConfigXmlHandler& other) :
m_config(nullptr),
m_emotionRootPath(),
m_depth(0),
m_hasConfigRoot(false),
m_recordType(kRecordNone),
m_currentFieldName(),
m_currentText(),
m_currentGroup(),
m_currentExpression(),
m_lastError()
{
    Q_UNUSED(other);
}

ExpressionConfigXmlHandler& ExpressionConfigXmlHandler::operator=(const ExpressionConfigXmlHandler& other)
{
    Q_UNUSED(other);
    return *this;
}
