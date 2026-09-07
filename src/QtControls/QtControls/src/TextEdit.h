#pragma once
#include "ControlShow.h"
#include "ControlSelf.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include <QTextEdit>
#include "QtControlsMacro.h"

/** 多行文本编辑控件，封装字体、边框、背景和默认输入样式
*/
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
	/** 设置文本编辑框默认属性
	*/
	void setDefault();
};