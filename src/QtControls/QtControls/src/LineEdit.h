#pragma once
#include "ControlShow.h"
#include "ControlSelf.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include <QtWidgets/QLineEdit>
#include "QtControlsMacro.h"

class QtControlsAPI LineEdit :
	public ControlShow < QLineEdit >,
	public ControlSelf < QLineEdit >,
	public ControlFont < QLineEdit >,
	public ControlBorderForNormalHoverDisabled < QLineEdit >,
	public ControlBackgroundForNormalHoverDisabled < QLineEdit >
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	LineEdit(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~LineEdit();

private slots:
	void currentTextChanged(const QString& str);

private:
	/** 常用初始化
	*/
	void setDefault();
};