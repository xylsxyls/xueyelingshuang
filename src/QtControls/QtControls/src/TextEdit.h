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
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	TextEdit(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~TextEdit();

private:
	/** 常用初始化
	*/
	void setDefault();
};