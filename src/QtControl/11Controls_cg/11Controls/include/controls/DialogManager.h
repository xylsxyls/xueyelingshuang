#pragma once
#include <map>
#include <stdint.h>
#include <QString>
#include <QObject>
#include <QMutex>
#include "ControlsMacro.h"
#include "DialogType.h"
#include "../core/ManagerBase.h"

/** 单一实例，用于统一管理窗口创建关闭，该类为对外接口
*/
class ControlsAPI DialogManager :
    public QObject,
    public ManagerBase < DialogManager >
{
	Q_OBJECT
public:
    /** 构造函数
    */
    DialogManager();

public:
    /** 创建窗口
    @param [in] type 窗口类型
    @param [in,out] param 窗口参数结构体指针，结构体中可能有传出参数
    */
    void createDialog(DialogType type, ParamBase* param = nullptr);

	/** 根据ID号关闭窗口（无动画效果）
	@param [in] dialogId 窗口ID号
	*/
	void destroyDialog(int32_t dialogId);

	/** 关闭最后一个弹出的窗口（无动画效果）
	*/
	void destroyLastDialog();

	/** 销毁所有窗口（无动画效果）
	*/
	void destroyAll();

	/** 当前弹框数量
	@return 返回弹框数量
	*/
	int32_t dialogCount();

Q_SIGNALS:
    /** 转到后台下载
    @param [in] taskId 用户自定义ID
    */
    void changeToBack(qint32 taskId);

    /** 重试
    @param [in] taskId 用户自定义ID
    */
    void downloadAgain(qint32 taskId);

    /** 取消下载
    @param [in] taskId 用户自定义ID
    */
    void cancelDownload(qint32 taskId);

    /** 使用其他下载渠道
    @param [in] taskId 用户自定义ID
    */
    void useOtherDownload(qint32 taskId);

    /** 下载地址的复制按钮
    @param [in] taskId 用户自定义ID
    @param [in] addr 下载地址
    */
    void copyDownloadAddr(qint32 taskId, const QString& addr);

    /** 本地路径的复制按钮
    @param [in] taskId 用户自定义ID
    @param [in] path 本地路径
    */
    void copyPath(qint32 taskId, const QString& path);

    /** 弹窗关闭的时候发送信号
    @param [in] dialogId 窗口ID
    @param [in] userId 用户自定义ID
    @param [in] type 窗口类型
    @param [in] result 窗口返回值
    @param [in] userParam 用户自定义参数
    */
    void popDialogDone(int32_t dialogId, int32_t userId, DialogType type, int32_t result, int32_t userParam);

    /** 通知框关闭的时候发送信号
    @param [in] dialogId 窗口ID
    @param [in] userId 用户自定义ID
    @param [in] type 窗口类型
    @param [in] result 窗口返回值
    @param [in] userParam 用户自定义参数
    */
    void notifyDialogDone(int32_t dialogId, int32_t userId, DialogType type, int32_t result, int32_t userParam);

    /** 静态窗关闭的时候发送信号
    @param [in] dialogId 窗口ID
    @param [in] userId 用户自定义ID
    @param [in] type 窗口类型
    @param [in] result 窗口返回值
    @param [in] userParam 用户自定义参数
    */
    void staticDialogDone(int32_t dialogId, int32_t userId, DialogType type, int32_t result, int32_t userParam);

private:
    void init();
    
private:
    bool m_init;
};