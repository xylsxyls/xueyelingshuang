#pragma once
#include "DialogManagerMacro.h"
#include "DialogType.h"

class COriginalDialog;

/** 内置窗口模板创建入口，集中维护通用弹窗类型和默认展示模式
*/
class DialogManagerAPI DialogTemplate
{
public:
	/** 创建内置窗口对象
	@param [in] dialogType 内置窗口类型ID
	@return 返回新建窗口指针，调用方负责交给AllocManager托管；失败返回nullptr
	*/
	static COriginalDialog* createDialog(DialogType dialogType);

	/** 查询内置窗口默认展示模式
	@param [in] dialogType 内置窗口类型ID
	@return 返回展示模式，不支持的历史类型返回ERROR_DIALOG_SHOW_MODE
	*/
	static DialogShowMode showMode(DialogType dialogType);

	/** 判断是否是内置窗口模板ID
	@param [in] dialogType 窗口类型ID
	@return 返回true表示属于DialogTemplate负责的通用窗口模板范围
	*/
	static bool isBuiltInDialogType(DialogType dialogType);

private:
	/** 禁止构造纯静态工具类
	*/
	DialogTemplate();
};
