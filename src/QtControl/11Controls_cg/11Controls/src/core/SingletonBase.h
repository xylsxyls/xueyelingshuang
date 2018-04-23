#pragma once

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

#include "SingletonBase.inl"