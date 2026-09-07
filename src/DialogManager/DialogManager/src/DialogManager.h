#pragma once
#include <QObject>
#include "DialogManagerMacro.h"
#include "DialogType.h"
#include "ManagerBase/ManagerBaseAPI.h"

/** 弹框统一入口单例，负责把创建和操作请求分发到对应弹框管理器
*/
class DialogManagerAPI DialogManager :
    public QObject,
    public ManagerBase<DialogManager>
{
	Q_OBJECT
public:
    /** 构造函数
    */
    DialogManager();

	/** 析构函数
	*/
	~DialogManager();

public:
    /** 创建窗口
    @param [in,out] param 窗口参数，结构体中可能包含传出参数
    */
    void makeDialog(DialogParam& param);

public:
	/** 操作窗口
	@param [in,out] param 操作参数
	*/
	void operateDialog(OperateParam& param);

Q_SIGNALS:
	/** 窗口发出信号
	@param [in] param 信号参数
	*/
	void dialogSignal(const SignalParam& param);
};