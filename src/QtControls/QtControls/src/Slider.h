#pragma once
#include <QtWidgets/QSlider>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlItem.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

/** ������ͼƬ�ؼ����ÿؼ���setBackground��غ���û�зָ�Ч��������ʹ��setBorder��غ���
*/
class QtControlsAPI Slider :
	public ControlShow < QSlider >,
	public ControlFont < QSlider >,
	public ControlSelf < QSlider >,
	public ControlItem < QSlider >,
	public ControlBorderForNormalHoverDisabled < QSlider >,
	public ControlBackgroundForNormalHoverDisabled < QSlider >,
	public ControlItemBorderForNormalHoverDisabled < QSlider >,
	public ControlItemBackgroundForNormalHoverDisabled < QSlider >
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	Slider(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~Slider();
};