#pragma once
#include "DialogManagerMacro.h"
#include "DialogType.h"

class CustomViewBase;

/** 自定义内容区工厂，只负责创建CustomDialog内部承载的内容区
*/
class DialogManagerAPI CustomViewFactory
{
public:
	/** 析构函数
	*/
	virtual ~CustomViewFactory();

public:
	/** 创建自定义内容区
	@param [in] param 窗口创建参数
	@return 返回自定义内容区指针，创建失败返回nullptr
	*/
	virtual CustomViewBase* createView(const DialogParam& param) = 0;
};

/** 自定义内容区工厂销毁函数类型，未传入时由DialogManager使用delete释放
*/
typedef void (*CustomViewFactoryDestroy)(CustomViewFactory* factory);