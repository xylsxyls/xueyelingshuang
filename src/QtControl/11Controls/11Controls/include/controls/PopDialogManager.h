#pragma once
#include <QObject>
#include <stdint.h>
#include "DialogType.h"
#include "../core/ManagerBase.h"

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
    void popDialog(DialogType type, ParamBase* param);

public:
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
    void onChangeToBack();
    void onDownloadAgain();
    void onCancelDownload();
    void onUseOtherDownload();
    void onCopyDownloadAddr(const QString& addr);
    void onCopyPath(const QString& path);

private:
    quint64 userId();
	DownloadOperateDialog* downloadOperateDialogPtrByUserId(quint64 userId);
};