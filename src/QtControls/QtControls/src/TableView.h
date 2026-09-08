#pragma once
#include <QtWidgets/QTableView>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlItem.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "ControlTableStyle.h"
#include "QtControlsMacro.h"

/** 表格视图控件，封装QTableView本体、单元格、表头和网格线样式
*/
class QtControlsAPI TableView :
	public ControlShow < QTableView >,
	public ControlFont < QTableView >,
	public ControlSelf < QTableView >,
	public ControlItem < QTableView >,
	public ControlBorderForNormal < QTableView >,
	public ControlBackgroundForNormal < QTableView >,
	public ControlItemBorderForNormalSelectedDisabled < QTableView >,
	public ControlItemBackgroundForNormalSelectedDisabled < QTableView >,
	public ControlTableStyle < QTableView >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	TableView(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~TableView();

public:
	/** 设置控件默认样式参数
	*/
	void setDefault();
};