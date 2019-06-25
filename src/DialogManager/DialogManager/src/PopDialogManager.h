#pragma once
#include <QObject>
#include "DialogType.h"
#include "core/coreAPI.h"

class DownloadOperateDialog;
/** 弹窗的管理类
*/
class PopDialogManager :
    public QObject,
    public ManagerBase < PopDialogManager >
{
    Q_OBJECT
public:
    /** 弹出弹框
    @param [in] type 弹窗类型
    @param [in] param 弹窗参数
    */
	void popDialog(DialogParam& param);

public:
	/** 操作窗口
	@param [in] type 操作类型
	@param [in] param 操作参数
	*/
	void operateDialog(OperateParam& param);

Q_SIGNALS:
	/** 窗口发出信号
	@param [in] type 信号类型
	@param [in] param 信号参数
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