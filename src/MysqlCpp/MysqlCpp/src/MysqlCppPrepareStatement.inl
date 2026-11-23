template<class DriverHelper, class Binder>
void MysqlCppPrepareStatement::bindParameter(uint32_t parameterIndex, const Binder& bind)
{
    if (!isValid())
    {
        return;
    }
    const uint32_t index = DriverHelper::toDriverIndex(parameterIndex);
    if (index == 0)
    {
        m_session.reset();
        m_error = "mysql parameter index overflow";
        return;
    }
    try
    {
        bind(index);
    }
    catch (const typename DriverHelper::SqlException& error)
    {
        m_session.reset();
        m_error = DriverHelper::exceptionText(error);
    }
    catch (...)
    {
        m_session.reset();
        m_error = "mysql parameter binding failed";
    }
}