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
	/** ���ڷ����ź�
	@param [in] type �ź�����
	@param [in] param �źŲ���
	*/
	void dialogSignal(SignalType type, SignalParam* param);

private slots:
    void onClosedSignal(DialogResult* result);
};