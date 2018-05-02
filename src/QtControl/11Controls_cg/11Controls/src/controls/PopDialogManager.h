#pragma once
#include <QObject>
#include <stdint.h>
#include "DialogType.h"
#include "../core/ManagerBase.h"

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
    void popDialog(DialogType type, ParamBase* param);

public:
    /** 设置速度（支持多线程）
    @param [in] userId 用户自定义ID
    @param [in] speed 速度
    */
    void setDownloadSpeed(int32_t userId, const QString& speed);

    /** 设置已下载量（支持多线程）
    @param [in] userId 用户自定义ID
    @param [in] downloaded 已下载量
    */
    void setDownloaded(int32_t userId, const QString& download);

    /** 设置时间（支持多线程）
    @param [in] userId 用户自定义ID
    @param [in] time 时间
    */
    void setDownloadTime(int32_t userId, const QString& time);

    /** 设置比例（支持多线程）
    @param [in] userId 用户自定义ID
    @param [in] persent 百分比
    */
    void setRate(int32_t userId, int32_t persent);

    /** 设置编辑框内的下载地址（支持多线程）
    @param [in] userId 用户自定义ID
    @param [in] addr 下载地址
    */
    void setEditDownloadAddr(int32_t userId, const QString& addr);

    /** 设置编辑框内的本地路径（支持多线程）
    @param [in] userId 用户自定义ID
    @param [in] path 本地路径
    */
    void setEditPath(int32_t userId, const QString& path);

    /** 设置转到后台下载按钮是否可用（支持多线程）
    @param [in] userId 用户自定义ID
    @param [in] enable 是否可用
    */
    void setBackEnable(int32_t userId, bool enable);

    /** 当下载出错时显示下载框的出错状态（支持多线程）
    @param [in] userId 用户自定义ID
    */
    void error(int32_t userId);

    /** 从error切换到常态（支持多线程）
    @param [in] userId 用户自定义ID
    */
    void normal(int32_t userId);

Q_SIGNALS:
    //以下6个新号给新版下载框用
    //转到后台下载
    void changeToBack(qint32 taskId);
    //重试
    void downloadAgain(qint32 taskId);
    //取消下载
    void cancelDownload(qint32 taskId);
    //使用其他下载渠道
    void useOtherDownload(qint32 taskId);
    //下载地址的复制按钮
    void copyDownloadAddr(qint32 taskId, const QString& addr);
    //本地路径的复制按钮
    void copyPath(qint32 taskId, const QString& path);
    //弹窗关闭的时候发送信号
    void popDialogDone(const DialogDoneSignalParam& param);

private slots:
    void onClosedSignal(int result);
    void onChangeToBack();
    void onDownloadAgain();
    void onCancelDownload();
    void onUseOtherDownload();
    void onCopyDownloadAddr(const QString& addr);
    void onCopyPath(const QString& path);

private:
    int32_t userId();
};