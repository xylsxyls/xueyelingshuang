#pragma once
#include <QtWidgets/QListWidget>
#include <stdint.h>
#include "ControlFont.h"
#include "ControlItem.h"
#include "ControlSelf.h"
#include "ControlShow.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "ControlsMacro.h"

class ControlsAPI ListWidget :
	public ControlShow < QListWidget >,
	public ControlFont < QListWidget >,
	public ControlItem < QListWidget >,
	public ControlSelf < QListWidget >,
	public ControlBorderForNormal < QListWidget >,
	public ControlBackgroundForNormal < QListWidget >,
	public ControlItemBorderForNormalHoverDisabled < QListWidget >,
	public ControlItemBackgroundForNormalHoverDisabled < QListWidget >
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	ListWidget(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~ListWidget();

public:
	/** 设置最大高度
	@param [in] maxHeight 最大高度
	@param [in] rePaint 是否立即重画
	*/
	void setMaxHeight(int32_t maxHeight, bool rePaint = false);

	/** 是否使点击失效
	@param [in] enable 是否有效
	*/
	void setClickEnable(bool enable);

Q_SIGNALS:
	void itemPressed(QListWidgetItem* item);

protected:
	void mousePressEvent(QMouseEvent* eve);

private:
	bool m_click;
};