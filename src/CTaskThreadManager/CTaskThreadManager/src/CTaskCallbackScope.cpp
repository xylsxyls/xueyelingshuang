#include "CTaskCallbackScope.h"

#ifdef _MSC_VER
__declspec(thread) static CTaskCallbackScope* s_taskCallbackScope = nullptr;
#else
static thread_local CTaskCallbackScope* s_taskCallbackScope = nullptr;
#endif

CTaskCallbackScope::CTaskCallbackScope(const void* owner, const void* stopped) :
m_owner(owner),
m_stopped(stopped),
m_previous(s_taskCallbackScope)
{
    s_taskCallbackScope = this;
}

CTaskCallbackScope::~CTaskCallbackScope()
{
    s_taskCallbackScope = m_previous;
}

bool CTaskCallbackScope::contains(const void* owner)
{
    for (CTaskCallbackScope* scope = s_taskCallbackScope; scope != nullptr; scope = scope->m_previous)
    {
        if (scope->m_owner == owner)
        {
            return true;
        }
    }
    return false;
}

bool CTaskCallbackScope::stopping(const void* task)
{
    for (CTaskCallbackScope* scope = s_taskCallbackScope; scope != nullptr; scope = scope->m_previous)
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
    return s_taskCallbackScope != nullptr;
}