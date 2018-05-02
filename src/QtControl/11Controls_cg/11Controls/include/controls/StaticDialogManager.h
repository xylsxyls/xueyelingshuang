#pragma once
#include <QObject>
#include "DialogType.h"
#include "../core/ManagerBase.h"
#include "ControlsMacro.h"

class AccountManagerDialog;
/** ��̬���ڹ�����
*/
class ControlsAPI StaticDialogManager :
    public QObject,
    public ManagerBase < StaticDialogManager >
{
    Q_OBJECT
public:
    /** ������̬��
    @param [in] type ��̬������
    @param [in] param ����
    */
    void popStaticDialog(DialogType type, ParamBase* param);

    /** ɾ����̬��
    @param [in] type ��̬������
    */
    void deleteStaticDialog(DialogType type);

public:
    /** ���캯��
    */
    StaticDialogManager();

Q_SIGNALS:
    /** ��̬���رյ�ʱ�����ź�
    @param [in] param ����رշ����źŵĲ����ṹ��
    */
    void staticDialogDone(const DialogDoneSignalParam& param);

private slots:
    void onClosedSignal(int result);

public:
    int32_t m_accountManagerDialogId;
};