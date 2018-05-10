#pragma once
#include <QObject>
#include "DialogType.h"
#include "../core/ManagerBase.h"
#include "ControlsMacro.h"

class SubAccountPanel;
class AccountDialog;
class ClosureDialog;
class AccountManagerDialog;
/** ��̬���ڹ�����
*/
class ControlsAPI StaticDialogManager :
    public QObject,
    public ManagerBase < StaticDialogManager >
{
    Q_OBJECT
public:
    /** ���캯��
    */
    StaticDialogManager();

public:
    /** ������̬��
    @param [in] type ��̬������
    @param [in] param ����
    */
    void popStaticDialog(DialogType type, ParamBase* param);

	/** ��������
	@param [in] type ��������
	@param [in] param ��������
	*/
	void operateDialog(OperateType type, OperateParam* param);

Q_SIGNALS:
	/** ���ڷ����ź�
	@param [in] type �ź�����
	@param [in] param �źŲ���
	*/
	void dialogSignal(SignalType type, SignalParam* param);

private slots:
    void onClosedSignal(DialogResult* result);

private:
	/** �رվ�̬��
	@param [in] type ��̬������
	*/
	void closeStaticDialog(DialogType type);

	/** ��ȡ��̬��ID
	@param [in] type ��̬������
	@return ���ؾ�̬��ID
	*/
	quint64 staticDialogDialogId(DialogType type);

private:
	AccountManagerDialog* accountManagerDialogPtr();

public:
    quint64 m_accountManagerDialogId;
};