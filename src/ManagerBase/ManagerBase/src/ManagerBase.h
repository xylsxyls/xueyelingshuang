#pragma once
#include "SingletonBase.h"

template<typename Manager>
class ManagerBase : public SingletonBase < Manager >
{
public:
    /** 单一实例
    @return 返回单一实例
    */
    static Manager& instance();

    /** 释放当前实例；后续再次调用instance会重新创建
    */
    static void releaseInstance();

};

#include "ManagerBase.inl"