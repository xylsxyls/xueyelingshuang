#pragma once
#include "QtControls/DialogShow.h"

class StockDrawWidget : public DialogShow
{
public:
	/** ���캯��
	*/
	StockDrawWidget();

protected:
	/** ��ʼ��
	*/
	void init();

public:
	/** ����avg����
	*/
	void setAvgParam();

protected:
	void resizeEvent(QResizeEvent* eve);
};