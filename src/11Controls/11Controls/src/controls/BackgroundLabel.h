#pragma once
#include "Label.h"
#include "ControlsMacro.h"

/** ͸�����ڱ����ؼ�
*/
class ControlsAPI BackgroundLabel : public Label
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	BackgroundLabel(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~BackgroundLabel();
};