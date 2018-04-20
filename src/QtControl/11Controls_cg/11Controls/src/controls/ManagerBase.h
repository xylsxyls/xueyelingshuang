#pragma once
#include <QObject>

template<typename Manager>
class ManagerBase : public QObject
{
public:
    /** 单一实例
    @return 返回单一实例
    */
    static Manager& instance();

    /** 实例是否存在
    @return 返回是否存在
    */
    static bool hasInstance();

    /** 释放实例，释放之后不再可以创建
    */
    static void releaseInstance();

protected:
    static Manager* m_manager;
};

#include "ManagerBase.inl"