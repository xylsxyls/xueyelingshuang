#pragma once
#include <QtWidgets/QTreeWidget>
#include <stdint.h>
#include "ControlFont.h"
#include "ControlItem.h"
#include "ControlSelf.h"
#include "ControlShow.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "ControlsMacro.h"

class ControlsAPI TreeWidget :
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

	/** 初始化滚动条样式
	*/
	void initScrollBar();

	/** 添加一个节点，如果父节点为空则添加组
	@param [in] itemHeight 节点高度
	@param [in] widget 节点指针
	@param [in] parent 父节点指针
	@param [in] column 第几列
	*/
	void addWidget(int32_t itemHeight, QWidget* widget, QWidget* parent = nullptr, int32_t column = 0);

	/** 移除一个节点
	@param [in] widget 节点指针
	@param [in] column 第几列
	*/
	void removeWidget(QWidget* widget, int32_t column = 0);

private:
	void dropEvent(QDropEvent* eve);

private:
	std::map<QWidget*, QTreeWidgetItem*> m_itemData;
};