#pragma once
#include <mutex>

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

    /** 释放当前实例；后续再次调用instance会重新创建
    */
    static void releaseInstance();

protected:
    static Singleton* s_singleton;
    static std::mutex s_mutex;
};

#include "SingletonBase.inl"