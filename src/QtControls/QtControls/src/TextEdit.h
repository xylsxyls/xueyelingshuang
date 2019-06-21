#pragma once
#include "ControlShow.h"
#include "ControlSelf.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include <QTextEdit>
#include "QtControlsMacro.h"

class QtControlsAPI TextEdit :
	public ControlShow < QTextEdit >,
	public ControlSelf < QTextEdit >,
	public ControlFont < QTextEdit >,
	public ControlBorderForNormalHoverDisabled < QTextEdit >,
	public ControlBackgroundForNormalHoverDisabled < QTextEdit >
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	TextEdit(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~TextEdit();

private:
	/** ���ó�ʼ��
	*/
	void setDefault();
};