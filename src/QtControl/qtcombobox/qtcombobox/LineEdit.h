#pragma once
#include "ControlSelf.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include <QtWidgets/QLineEdit>

class LineEdit :
	public ControlSelf < QLineEdit >,
	public ControlFont < QLineEdit >,
	public ControlBorderForNormalHoverDisabled < QLineEdit >
{
public:
	virtual ~LineEdit(){};
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	LineEdit(QWidget* parent = NULL);

	/** 设置背景颜色（没有按下效果）
	@param [in] backgroundNormalColor 常态颜色
	@param [in] backgroundHoverColor 悬停颜色
	@param [in] backgroundDisabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundColor(const QColor& backgroundNormalColor,
	 	 		 		    const QColor& backgroundHoverColor,
						    const QColor& backgroundDisabledColor,
							bool rePaint = false);

private:
	/** 常用初始化
	*/
	void init();
};