#pragma once
#include <QObject>
#include "DialogType.h"
#include "core/coreAPI.h"

class DownloadOperateDialog;
/** �����Ĺ�����
*/
class PopDialogManager :
    public QObject,
    public ManagerBase < PopDialogManager >
{
    Q_OBJECT
public:
    /** ��������
    @param [in] type ��������
    @param [in] param ��������
    */
	void popDialog(DialogParam& param);

public:
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
    void onChangeToBack();
    void onDownloadAgain();
    void onCancelDownload();
    void onUseOtherDownload();
    void onCopyDownloadAddr(const QString& addr);
    void onCopyPath(const QString& path);
	void onAlreadyShown();

private:
    quint64 userId();
	DownloadOperateDialog* downloadOperateDialogPtrByUserId(quint64 userId);
};