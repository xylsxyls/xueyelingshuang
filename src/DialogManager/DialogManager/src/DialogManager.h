#pragma once
#include <QObject>
#include "DialogManagerMacro.h"
#include "DialogType.h"
#include "ManagerBase/ManagerBaseAPI.h"

/** 单一实例，用于统一管理窗口创建关闭，该类为对外接口
*/
class DialogManagerAPI DialogManager :
    public QObject,
    public ManagerBase < DialogManager >
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
    @param [in] type 窗口类型
    @param [in,out] param 窗口参数结构体指针，结构体中可能有传出参数
    */
    void makeDialog(DialogParam& param);

public:
	/** 操作窗口
	@param [in,out] param 操作参数
	*/
	void operateDialog(OperateParam& param);

Q_SIGNALS:
	/** 窗口发出信号
	@param [in] type 信号类型
	@param [in] param 信号参数
	*/
	void dialogSignal(const SignalParam& param);
};