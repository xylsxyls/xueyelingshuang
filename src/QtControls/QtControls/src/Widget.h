#pragma once
#include <QWidget>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

/** ���ڿؼ����ÿؼ���setBorder��غ���û��Ч����setBackground����ֻ��������ɫ
*/
class QtControlsAPI Widget :
	public ControlShow < QWidget >,
	public ControlFont < QWidget >,
	public ControlSelf < QWidget >,
	public ControlBackgroundForNormal < QWidget >
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	Widget(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~Widget();
};