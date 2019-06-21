#pragma once
#include "SingletonBase.h"

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

#include "ManagerBase.inl"