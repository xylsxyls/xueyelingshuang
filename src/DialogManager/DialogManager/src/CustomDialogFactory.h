#pragma once
#include "DialogManagerMacro.h"
#include "DialogType.h"

class CustomDialog;
class CustomViewBase;

/** 业务自定义窗口工厂，负责创建10000以上类型ID对应的窗口外壳或内容区
*/
class DialogManagerAPI CustomDialogFactory
{
public:
	/** 析构函数
	*/
	virtual ~CustomDialogFactory();

public:
	/** 创建自定义窗口外壳
	@param [in] param 窗口创建参数
	@return 返回自定义窗口指针，创建失败返回nullptr
	*/
	virtual CustomDialog* createDialog(const DialogParam& param);

	/** 创建自定义窗口内容区
	@param [in] param 窗口创建参数
	@return 返回自定义内容区指针，默认返回nullptr
	*/
	virtual CustomViewBase* createView(const DialogParam& param);

	/** 获取自定义窗口展示模式
	@param [in] param 窗口创建参数
	@return 返回展示模式，自定义窗口只支持模态和普通非模态两种模式
	*/
	virtual DialogShowMode showMode(const DialogParam& param) const;
};

/** 自定义窗口工厂销毁函数类型，未传入时由DialogManager使用delete释放
*/
typedef void (*CustomDialogFactoryDestroy)(CustomDialogFactory* factory);