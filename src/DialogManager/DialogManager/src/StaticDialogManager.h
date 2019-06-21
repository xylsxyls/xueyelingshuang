#pragma once
#include <QObject>
#include "DialogType.h"
#include "core/ManagerBase.h"
#include "DialogManagerMacro.h"

class SubAccountPanel;
class AccountDialog;
class ClosureDialog;
class AccountManagerDialog;
/** ��̬���ڹ�����
*/
class DialogManagerAPI StaticDialogManager :
    public QObject,
    public ManagerBase < StaticDialogManager >
{
    Q_OBJECT
public:
    /** ���캯��
    */
    StaticDialogManager();

	/** ��������
	*/
	~StaticDialogManager();

public:
    /** ������̬��
    @param [in] type ��̬������
    @param [in] param ����
    */
	void popStaticDialog(DialogParam& param);

	/** ��������
	@param [in] type ��������
	@param [in] param ��������
	*/
	void operateDialog(OperateParam& param);

Q_SIGNALS:
	/** ���ڷ����ź�
	@param [in] type �ź�����
	@param [in] param �źŲ���
	*/
	void dialogSignal(const SignalParam& param);

private slots:
    void onClosedSignal(DialogResult* result);
	void onAlreadyShown();

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