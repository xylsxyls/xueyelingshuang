#pragma once
#include <QtWidgets/QTreeWidget>
#include <stdint.h>
#include "ControlFont.h"
#include "ControlItem.h"
#include "ControlSelf.h"
#include "ControlShow.h"
#include "ControlBorder.h"
#include "ControlBackground.h"

class TreeWidget :
	public ControlShow < QTreeWidget >,
	public ControlFont < QTreeWidget >,
	public ControlItem < QTreeWidget >,
	public ControlSelf < QTreeWidget >,
	public ControlBorderForNormal < QTreeWidget >,
	public ControlBackgroundForNormal < QTreeWidget >,
	public ControlItemBorderForNormalHoverDisabled < QTreeWidget >,
	public ControlItemBackgroundForNormalHoverDisabled < QTreeWidget >
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	TreeWidget(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~TreeWidget();

public:
	/** 设置最大高度
	@param [in] maxHeight 最大高度
	@param [in] rePaint 是否立即重画
	*/
	void setMaxHeight(int32_t maxHeight, bool rePaint = false);

	void addWidget(int32_t itemHeight, QWidget* widget, QWidget* parent = nullptr, int32_t column = 0);

private:
	std::map<QWidget*, QTreeWidgetItem*> m_itemData;
};