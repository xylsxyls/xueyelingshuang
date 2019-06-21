#pragma once

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

#include "SingletonBase.inl"