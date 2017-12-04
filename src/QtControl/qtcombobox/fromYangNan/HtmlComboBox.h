#pragma once
#include "ComboBox.h"

class HtmlComboBox : public ComboBox
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	HtmlComboBox(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~HtmlComboBox();

private:
	void paintEvent(QPaintEvent* eve);
};