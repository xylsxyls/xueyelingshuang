#pragma once
#include <map>
#include <stdint.h>
#include <QString>
#include <QObject>
#include <QMutex>
#include "ControlsMacro.h"
#include "DialogType.h"
#include "ManagerBase.h"

/** 单一实例，用于统一管理窗口创建关闭，该类的对外接口，销毁所有窗口
DialogManager::instance().removeAll();
*/
class ControlsAPI DialogManager : public ManagerBase < DialogManager >
{
	Q_OBJECT
public:
    /** 创建窗口
    @param [in] type 窗口类型
    @param [in,out] param 窗口参数结构体指针，结构体中可能有传出参数
    */
    void createDialog(DialogType type, void* param = nullptr);

	/** 根据ID号关闭窗口（有动画效果）
	@param [in] dialogId 窗口ID号
	*/
	void closeDialog(int32_t dialogId);

	/** 关闭最后一个弹出的窗口（有动画效果）
	*/
	void closeLastDialog();

	/** 销毁所有窗口（无动画效果）
	*/
	void destroyAll();

	/** 当前弹框数量
	@return 返回弹框数量
	*/
	int32_t dialogCount();

public:
	DialogManager();
};