#include "MysqlCppTransaction.h"
#include "MysqlCpp.h"

MysqlCppTransaction::MysqlCppTransaction(MysqlCpp& mysql) :
m_mysql(&mysql),
m_session(mysql.m_session),
m_active(mysql.beginTransaction())
{

}

MysqlCppTransaction::~MysqlCppTransaction()
{
    if (isActive())
    {
        m_mysql->rollback();
    }
}

bool MysqlCppTransaction::isActive() const
{
    return m_active && !m_session.expired();
}

bool MysqlCppTransaction::commit()
{
    if (!isActive())
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
    if (isActive() && m_mysql->rollback())
    {
        m_active = false;
    }
}