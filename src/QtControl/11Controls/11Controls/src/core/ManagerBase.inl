#ifndef MANAGER_BASE_H__
#define MANAGER_BASE_H__

#include "ManagerBase.h"

template<typename Manager>
bool ManagerBase<Manager>::s_isUsed = false;

template<typename Manager>
void ManagerBase<Manager>::releaseInstance()
{
    s_isUsed = true;
    SingletonBase<Manager>::releaseInstance();
}

template<typename Manager>
Manager& ManagerBase<Manager>::instance()
{
    if (s_isUsed)
    {
        abort();
    }
    return SingletonBase<Manager>::instance();
}

#endif