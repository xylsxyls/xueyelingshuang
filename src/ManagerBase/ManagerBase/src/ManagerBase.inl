#ifndef MANAGER_BASE_H__
#define MANAGER_BASE_H__

#include "ManagerBase.h"

template<typename Manager>
void ManagerBase<Manager>::releaseInstance()
{
    SingletonBase<Manager>::releaseInstance();
}

template<typename Manager>
Manager& ManagerBase<Manager>::instance()
{
    return SingletonBase<Manager>::instance();
}

#endif