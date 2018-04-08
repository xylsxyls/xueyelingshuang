#pragma once
#include "ControlShow.h"
#include "ControlSelf.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include <QtWidgets/QLineEdit>

class LineEdit :
	public ControlShow < QLineEdit >,
	public ControlSelf < QLineEdit >,
	public ControlFont < QLineEdit >,
	public ControlBorderForNormalHoverDisabled < QLineEdit >,
	public ControlBackgroundForNormalHoverDisabled < QLineEdit >
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	LineEdit(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~LineEdit();

private slots:
	void currentTextChanged(const QString& str);

private:
	/** ���ó�ʼ��
	*/
	void setDefault();
};