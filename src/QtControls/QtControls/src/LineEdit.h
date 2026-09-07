#pragma once
#include "ControlShow.h"
#include "ControlSelf.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include <QtWidgets/QLineEdit>
#include "QtControlsMacro.h"

/** 单行输入框控件，封装字体、边框、背景和常用输入样式
*/
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
	/** 处理文本变化，用于维护输入框默认提示和样式状态
	@param [in] str 当前输入框文字
	*/
	void currentTextChanged(const QString& str);

private:
	/** 设置输入框默认属性和信号连接
	*/
	void setDefault();
};