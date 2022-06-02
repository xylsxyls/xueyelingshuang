#pragma once
#include "QtControls/TextEdit.h"

class SearchEdit : public TextEdit
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	SearchEdit(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~SearchEdit();

public:
	int32_t rightClickRow();

public:
	virtual void mousePressEvent(QMouseEvent* eve);
	virtual void mouseDoubleClickEvent(QMouseEvent* eve);

private:
	int32_t m_rightClickRow;
};