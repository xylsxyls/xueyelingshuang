#pragma once
#include "Label.h"
#include "QtControlsMacro.h"

/** ͸�����ڱ����ؼ�
*/
class QtControlsAPI BackgroundLabel : public Label
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