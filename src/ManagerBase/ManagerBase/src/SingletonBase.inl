#ifndef SINGLETON_BASE_H__
#define SINGLETON_BASE_H__

#include "SingletonBase.h"

template<typename Singleton>
Singleton* SingletonBase<Singleton>::s_singleton = nullptr;

template<typename Singleton>
std::mutex SingletonBase<Singleton>::s_mutex;

template<typename Singleton>
void SingletonBase<Singleton>::releaseInstance()
{
    Singleton* oldSingleton = nullptr;
    {
        std::lock_guard<std::mutex> locker(s_mutex);
        oldSingleton = s_singleton;
        s_singleton = nullptr;
    }
    delete oldSingleton;
}

template<typename Singleton>
bool SingletonBase<Singleton>::hasInstance()
{
    std::lock_guard<std::mutex> locker(s_mutex);
    return s_singleton != nullptr;
}

template<typename Singleton>
Singleton& SingletonBase<Singleton>::instance()
{
    std::lock_guard<std::mutex> locker(s_mutex);
    if (s_singleton == nullptr)
    {
        s_singleton = new Singleton;
    }
    return *s_singleton;
}

#endif
