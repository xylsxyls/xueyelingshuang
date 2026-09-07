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
	void dialogSignal(const SignalParam& param);

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
    /** 获取当前用户ID
    @return 返回当前用户ID
    */
    quint64 userId();

	/** 根据用户ID查找下载操作弹框
	@param [in] userId 用户ID
	@return 返回下载操作弹框指针，未找到时返回nullptr
	*/
	DownloadOperateDialog* downloadOperateDialogPtrByUserId(quint64 userId);
};