#include "CTaskCallbackScope.h"

#ifdef _MSC_VER
__declspec(thread) CTaskCallbackScope* CTaskCallbackScope::m_current = nullptr;
#else
thread_local CTaskCallbackScope* CTaskCallbackScope::m_current = nullptr;
#endif

bool CTaskCallbackScope::contains(const void* owner)
{
    for (CTaskCallbackScope* scope = m_current; scope != nullptr; scope = scope->m_previous)
    {
        if (scope->m_owner == owner)
        {
            return true;
        }
    }
    return false;
}

bool CTaskCallbackScope::reading(const void* owner)
{
    for (CTaskCallbackScope* scope = m_current; scope != nullptr; scope = scope->m_previous)
    {
        if (scope->m_owner == owner && scope->m_reader)
        {
            return true;
        }
    }
    return false;
}

bool CTaskCallbackScope::stopping(const void* task)
{
    for (CTaskCallbackScope* scope = m_current; scope != nullptr; scope = scope->m_previous)
    {
        if (scope->m_stopped == task)
        {
            return true;
        }
    }
    return false;
}

bool CTaskCallbackScope::active()
{
    return m_current != nullptr;
}