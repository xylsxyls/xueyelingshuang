#ifndef MANAGER_BASE_H__
#define MANAGER_BASE_H__

#include "ManagerBase.h"

template<typename Manager>
Manager* ManagerBase<Manager>::m_manager = nullptr;

template<typename Manager>
void ManagerBase<Manager>::releaseInstance()
{
    delete m_manager;
    m_manager = (Manager*)-1;
}

template<typename Manager>
bool ManagerBase<Manager>::hasInstance()
{
    return m_manager != nullptr && m_manager != (Manager*)-1;
}

template<typename Manager>
Manager& ManagerBase<Manager>::instance()
{
    if (m_manager == nullptr)
    {
        m_manager = new Manager;
        if (m_manager == nullptr)
        {
            abort();
        }
    }
    else if (m_manager == (Manager*)-1)
    {
        abort();
    }
    return *m_manager;
}

#endif