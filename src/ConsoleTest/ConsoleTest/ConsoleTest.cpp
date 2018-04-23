#include "ConsoleTest.h"
#include <stdint.h>
#include <stdio.h>
#include <windows.h>

template<typename Singleton>
class SingletonBase
{
public:
    /** 单一实例
    @return 返回单一实例
    */
    static Singleton& instance();

    /** 实例是否存在
    @return 返回是否存在
    */
    static bool hasInstance();

    /** 释放实例，释放之后不再可以创建
    */
    static void releaseInstance();

protected:
    static Singleton* s_singleton;
};

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

template<typename Manager>
class ManagerBase : public SingletonBase < Manager >
{
public:
    /** 单一实例
    @return 返回单一实例
    */
    static Manager& instance();

    /** 释放实例，释放之后不再可以创建
    */
    static void releaseInstance();

protected:
    static bool s_isUsed;
};

template<typename Manager>
bool ManagerBase<Manager>::s_isUsed = false;

template<typename Manager>
void ManagerBase<Manager>::releaseInstance()
{
    SingletonBase<Manager>::releaseInstance();
    s_isUsed = true;
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

class A : public ManagerBase < A >
{

};

class B : public ManagerBase < B >
{

};

int32_t main()
{
    A::instance();
    B::instance();
    A::releaseInstance();
    B::releaseInstance();
    getchar();
	return 0;
}