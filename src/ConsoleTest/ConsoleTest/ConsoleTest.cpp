#include "ConsoleTest.h"
#include <stdint.h>
#include <stdio.h>
#include <windows.h>

template<typename Singleton>
class SingletonBase
{
public:
    /** ��һʵ��
    @return ���ص�һʵ��
    */
    static Singleton& instance();

    /** ʵ���Ƿ����
    @return �����Ƿ����
    */
    static bool hasInstance();

    /** �ͷ�ʵ�����ͷ�֮���ٿ��Դ���
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
    /** ��һʵ��
    @return ���ص�һʵ��
    */
    static Manager& instance();

    /** �ͷ�ʵ�����ͷ�֮���ٿ��Դ���
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