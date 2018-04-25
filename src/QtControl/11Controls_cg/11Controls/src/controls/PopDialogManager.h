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
    void popDialog(DialogType type, ParamBase* param);

public:
    /** 设置比例（老版）
    @param [in] dialogId 窗口ID值
    @param [in] persent 百分比
    */
    void setDownloadRate(int32_t dialogId, int32_t persent);

    /** 下载完成（老版）
    @param [in] dialogId 窗口ID值
    */
    void downloadComplete(int32_t dialogId);

    /** 将当前下载框销毁并弹窗下载失败框（老版）
    @param [out] dialogId 窗口ID值
    */
    void downloadError(int32_t dialogId);

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
    void popDialogDone(int32_t dialogId, int32_t userId, DialogType type, int32_t result, int32_t userParam);

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