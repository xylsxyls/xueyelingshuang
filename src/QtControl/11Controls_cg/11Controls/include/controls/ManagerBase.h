#pragma once
#include <QObject>

template<typename Manager>
class ManagerBase : public QObject
{
public:
    /** ��һʵ��
    @return ���ص�һʵ��
    */
    static Manager& instance();

    /** ʵ���Ƿ����
    @return �����Ƿ����
    */
    static bool hasInstance();

    /** �ͷ�ʵ�����ͷ�֮���ٿ��Դ���
    */
    static void releaseInstance();

protected:
    static Manager* m_manager;
};

#include "ManagerBase.inl"