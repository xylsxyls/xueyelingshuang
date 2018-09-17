#pragma once
#include <QDialog>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "ControlsMacro.h"

/** ���ڿؼ����ÿؼ���setBackgroundImage�����޷�����hoverЧ����setBorderImage����
*/
class ControlsAPI Dialog :
	public ControlShow < QDialog >,
	public ControlFont < QDialog >,
	public ControlSelf < QDialog >,
	public ControlBorderForNormalHoverDisabled < QDialog >,
	public ControlBackgroundForNormalHoverDisabled < QDialog >
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	Dialog(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~Dialog();
};