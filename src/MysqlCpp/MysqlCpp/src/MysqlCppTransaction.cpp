#include "MysqlCppTransaction.h"
#include "MysqlCpp.h"

MysqlCppTransaction::MysqlCppTransaction(MysqlCpp& mysql) :
m_mysql(&mysql),
m_active(mysql.beginTransaction())
{

}

MysqlCppTransaction::~MysqlCppTransaction()
{
    if (m_active && m_mysql != nullptr)
    {
        m_mysql->rollback();
    }
}

bool MysqlCppTransaction::isActive() const
{
    return m_active;
}

bool MysqlCppTransaction::commit()
{
    if (!m_active || m_mysql == nullptr)
    {
        return false;
    }
    bool ok = m_mysql->commit();
    if (ok)
    {
        m_active = false;
    }
    return ok;
}

void MysqlCppTransaction::rollback()
{
    if (m_active && m_mysql != nullptr)
    {
        m_mysql->rollback();
        m_active = false;
    }
}