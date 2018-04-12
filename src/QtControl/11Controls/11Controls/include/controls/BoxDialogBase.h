#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"

class BoxDialogBase : public DialogShow
{
public:
	/** 构造函数
	@param [in] dialogWidth 窗口宽度
	@param [in] dialogHeight 窗口高度
	@param [in] parent 父窗口句柄
	*/
	BoxDialogBase(int32_t dialogWidth, int32_t dialogHeight, QWindow* parent = nullptr);

	/** 设置窗口标题
	@param [in] title 标题内容
	*/
	void setDialogTitle(const QString& title);


private:
	void paintEvent(QPaintEvent* eve);


};