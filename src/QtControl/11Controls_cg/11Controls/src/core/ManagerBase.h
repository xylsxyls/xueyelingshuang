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

    /** 释放实例，释放之后不再可以创建
    */
    static void releaseInstance();

protected:
    static bool s_isUsed;
};

#include "ManagerBase.inl"