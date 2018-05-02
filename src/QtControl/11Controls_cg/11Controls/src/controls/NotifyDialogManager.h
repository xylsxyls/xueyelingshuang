#pragma once
#include <QObject>
#include "DialogType.h"
#include "../core/ManagerBase.h"

/** ֪ͨ��Ĺ�����
*/
class NotifyDialogManager :
    public QObject,
    public ManagerBase < NotifyDialogManager >
{
    Q_OBJECT
public:
    /** ��ʾ����
    @param [in] type ��������
    @param [in] param ��������
    */
    void showDialog(DialogType type, ParamBase* param);

Q_SIGNALS:
    /** ��ģ̬���رյ�ʱ�����ź�
    @param [in] param ��ģ̬���رշ����źŵĲ����ṹ��
    */
    void notifyDialogDone(const DialogDoneSignalParam& param);

private slots:
    void onClosedSignal(int result);
};