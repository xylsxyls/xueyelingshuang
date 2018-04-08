#pragma once
#include "Label.h"

class UserWidget : public Label
{
public:
	/** ���캯��
	@param [in] hover ��ͣʱ�Ļ滭��
	@param [in] parent ������ָ��
	*/
	UserWidget(Label* hover, QWidget* parent = nullptr);

private:
	void enterEvent(QEvent* eve);
	void leaveEvent(QEvent* eve);

private:
	Label* m_hover;
};