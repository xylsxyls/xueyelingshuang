#pragma once
#include <QtWidgets/QTableWidget>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlItem.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "ControlTableStyle.h"
#include "QtControlsMacro.h"

/** 表格控件，封装QTableWidget本体、单元格、表头和网格线样式
*/
class QtControlsAPI TableWidget :
	public ControlShow < QTableWidget >,
	public ControlFont < QTableWidget >,
	public ControlSelf < QTableWidget >,
	public ControlItem < QTableWidget >,
	public ControlBorderForNormal < QTableWidget >,
	public ControlBackgroundForNormal < QTableWidget >,
	public ControlItemBorderForNormalSelectedDisabled < QTableWidget >,
	public ControlItemBackgroundForNormalSelectedDisabled < QTableWidget >,
	public ControlTableStyle < QTableWidget >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	TableWidget(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~TableWidget();

public:
	/** 设置控件默认样式参数
	*/
	void setDefault();
};