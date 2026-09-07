#include "ExpressionConfig.h"

ExpressionGroupInfo::ExpressionGroupInfo() :
m_groupId(),
m_desc(),
m_fileName(),
m_tooltip()
{

}

ExpressionInfo::ExpressionInfo() :
m_groupId(),
m_id(),
m_desc(),
m_fileName(),
m_shortcut(),
m_tooltip()
{

}

bool ExpressionInfo::isValid() const
{
    return !m_groupId.isEmpty() && !m_fileName.isEmpty() && !m_shortcut.isEmpty();
}

ExpressionConfig::ExpressionConfig() :
m_groups(),
m_expressions()
{

}

void ExpressionConfig::clear()
{
    m_groups.clear();
    m_expressions.clear();
}
