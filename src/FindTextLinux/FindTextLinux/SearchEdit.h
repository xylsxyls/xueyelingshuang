#pragma once
#include "QtControls/TextEdit.h"

class SearchEdit : public TextEdit
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	SearchEdit(QWidget* parent = nullptr);

	/** ��������
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