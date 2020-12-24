#pragma once
#include <QtWidgets/QPushButton>
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

class QtControlsAPI PushButton :
	public ControlShow < QPushButton >,
	public ControlFont < QPushButton >,
	public ControlSelf < QPushButton >,
	public ControlBorderForNormalHoverPressedDisabled < QPushButton >,
	public ControlBackgroundForNormalHoverPressedDisabled < QPushButton >
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	PushButton(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~PushButton();
};