#ifndef SINGLETON_BASE_H__
#define SINGLETON_BASE_H__

#include "SingletonBase.h"

template<typename Singleton>
Singleton* SingletonBase<Singleton>::s_singleton = nullptr;

template<typename Singleton>
void SingletonBase<Singleton>::releaseInstance()
{
    if (s_singleton != nullptr)
    {
        delete s_singleton;
        s_singleton = nullptr;
    }
}

template<typename Singleton>
bool SingletonBase<Singleton>::hasInstance()
{
    return s_singleton != nullptr;
}

template<typename Singleton>
Singleton& SingletonBase<Singleton>::instance()
{
    if (s_singleton == nullptr)
    {
        s_singleton = new Singleton;
        if (s_singleton == nullptr)
        {
            abort();
        }
    }
    return *s_singleton;
}

#endif