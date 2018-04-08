#pragma once
#include <QtWidgets/QLabel>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"

/** ������ͼƬ�ؼ����ÿؼ���setBackground��غ���û�зָ�Ч��������ʹ��setBorder��غ���
*/
class Label : 
	public ControlShow < QLabel >,
	public ControlFont < QLabel >,
	public ControlSelf < QLabel >,
	public ControlBorderForNormalHoverDisabled < QLabel >,
	public ControlBackgroundForNormalHoverDisabled < QLabel >
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	Label(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~Label();

public:
	
};