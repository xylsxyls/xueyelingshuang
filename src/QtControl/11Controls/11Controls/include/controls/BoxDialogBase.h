#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"

class BoxDialogBase : public DialogShow
{
public:
	/** ���캯��
	@param [in] dialogWidth ���ڿ��
	@param [in] dialogHeight ���ڸ߶�
	@param [in] parent �����ھ��
	*/
	BoxDialogBase(int32_t dialogWidth, int32_t dialogHeight, QWindow* parent = nullptr);

	/** ���ô��ڱ���
	@param [in] title ��������
	*/
	void setDialogTitle(const QString& title);


private:
	void paintEvent(QPaintEvent* eve);


};