#pragma once
#include <QtWidgets/QListWidget>
#include <stdint.h>
#include "ControlFont.h"
#include "ControlItem.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"

class ListWidget : 
	public ControlFont < QListWidget >,
	public ControlItem < QListWidget >,
	public ControlSelf < QListWidget >,
	public ControlBorderForNormal < QListWidget >,
	public ControlBackgroundForNormal < QListWidget >,
	public ControlItemBorderForNormalHoverDisabled < QListWidget >,
	public ControlItemBackgroundForNormalHoverDisabled < QListWidget >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	ListWidget(QWidget* parent = NULL);

	/** 析构函数
	*/
	virtual ~ListWidget();

public:
	/** 设置最大高度
	@param [in] maxHeight 最大高度
	@param [in] rePaint 是否立即重画
	*/
	void setMaxHeight(int32_t maxHeight, bool rePaint = false);
};