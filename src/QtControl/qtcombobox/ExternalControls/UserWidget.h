#pragma once
#include "Label.h"

class UserWidget : public Label
{
public:
	/** 构造函数
	@param [in] hover 悬停时的绘画层
	@param [in] parent 父窗口指针
	*/
	UserWidget(Label* hover, QWidget* parent = nullptr);

private:
	void enterEvent(QEvent* eve);
	void leaveEvent(QEvent* eve);

private:
	Label* m_hover;
};