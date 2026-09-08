#pragma once
#include <QObject>
#include "DialogType.h"
#include "ManagerBase/ManagerBaseAPI.h"

class DownloadOperateDialog;
/** 弹窗的管理类
*/
class PopDialogManager :
    public QObject,
    public ManagerBase<PopDialogManager>
{
    Q_OBJECT
public:
    /** 弹出弹框
    @param [in] param 弹窗参数
    */
	void popDialog(DialogParam& param);

public:
	/** 操作窗口
	@param [in] param 操作参数
	*/
	void operateDialog(OperateParam& param);

Q_SIGNALS:
	/** 窗口发出信号
	@param [in] param 信号参数
	*/
	void dialogSignal(const DialogSignalPtr& param);

private slots:
    /** 处理弹框关闭信号
    @param [in] result 弹框关闭结果
    */
    void onClosedSignal(DialogResult* result);

    /** 处理下载窗口切换到后台下载
    */
    void onChangeToBack();

    /** 处理下载窗口重试下载
    */
    void onDownloadAgain();

    /** 处理下载窗口取消下载
    */
    void onCancelDownload();

    /** 处理下载窗口选择其他下载渠道
    */
    void onUseOtherDownload();

    /** 处理复制下载地址
    @param [in] addr 下载地址
    */
    void onCopyDownloadAddr(const QString& addr);

    /** 处理复制本地路径
    @param [in] path 本地路径
    */
    void onCopyPath(const QString& path);

	/** 处理同类弹框已经显示的情况
	*/
	void onAlreadyShown();

private:
    /** 获取当前发送信号窗口的业务复用键，信号回传时用它让上层定位业务对象
    @return 返回当前窗口注册到AllocManager时保存的业务复用键
    */
    DialogUserKey userKey();

	/** 根据业务复用键查找下载操作弹框，避免不同业务模块的下载任务ID互相误伤
	@param [in] userKey 业务复用键
	@return 返回下载操作弹框指针，未找到时返回nullptr
	*/
	DownloadOperateDialog* downloadOperateDialogPtrByUserKey(const DialogUserKey& userKey);
};
