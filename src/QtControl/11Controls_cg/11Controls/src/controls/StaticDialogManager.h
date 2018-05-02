#pragma once
#include <QObject>
#include "DialogType.h"
#include "../core/ManagerBase.h"
#include "ControlsMacro.h"

class AccountManagerDialog;
/** 静态窗口管理类
*/
class ControlsAPI StaticDialogManager :
    public QObject,
    public ManagerBase < StaticDialogManager >
{
    Q_OBJECT
public:
    /** 弹出静态框
    @param [in] type 静态框类型
    @param [in] param 参数
    */
    void popStaticDialog(DialogType type, ParamBase* param);

    /** 删除静态框
    @param [in] type 静态框类型
    */
    void deleteStaticDialog(DialogType type);

public:
    /** 构造函数
    */
    StaticDialogManager();

Q_SIGNALS:
    /** 静态窗关闭的时候发送信号
    @param [in] param 弹框关闭发送信号的参数结构体
    */
    void staticDialogDone(const DialogDoneSignalParam& param);

private slots:
    void onClosedSignal(int result);

public:
    int32_t m_accountManagerDialogId;
};