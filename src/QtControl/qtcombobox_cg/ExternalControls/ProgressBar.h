#pragma once
#include <QtWidgets/QProgressBar>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlItem.h"
#include "ControlBorder.h"
#include "ControlBackground.h"

class ProgressBar : 
	public ControlShow < QProgressBar >,
	public ControlFont < QProgressBar >,
	public ControlSelf < QProgressBar >,
	public ControlItem < QProgressBar >,
	public ControlBorderForNormalHoverDisabled < QProgressBar >,
	public ControlBackgroundForNormalHoverDisabled < QProgressBar >,
	public ControlItemBorderForNormalHoverDisabled < QProgressBar >,
	public ControlItemBackgroundForNormalHoverDisabled < QProgressBar >
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	ProgressBar(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~ProgressBar();

public:
	
};