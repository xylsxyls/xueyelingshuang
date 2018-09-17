#pragma once
#include <QDialog>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "ControlsMacro.h"

/** 窗口控件，该控件的setBackgroundImage函数无法做到hover效果，setBorderImage可以
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
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	Dialog(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~Dialog();
};