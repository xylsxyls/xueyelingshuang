#pragma once
#include <QtWidgets/QScrollArea>
#include "ControlShow.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

class ScrollBar;

/** 滚动区域控件，封装 QScrollArea 本体、视口背景和自定义滚动条接入。
*/
class QtControlsAPI ScrollArea :
	public ControlShow < QScrollArea >,
	public ControlSelf < QScrollArea >,
	public ControlBorderForNormalHoverDisabled < QScrollArea >,
	public ControlBackgroundForNormalHoverDisabled < QScrollArea >
{
public:
	/** 构造函数。
	@param [in] parent 父窗口指针。
	*/
	ScrollArea(QWidget* parent = nullptr);

	/** 析构函数。
	*/
	virtual ~ScrollArea();

public:
	/** 获取垂直滚动条封装控件。
	@return 返回当前垂直滚动条封装控件，外部替换为非 ScrollBar 时返回空指针。
	*/
	ScrollBar* verticalScrollBarControl() const;

	/** 获取水平滚动条封装控件。
	@return 返回当前水平滚动条封装控件，外部替换为非 ScrollBar 时返回空指针。
	*/
	ScrollBar* horizontalScrollBarControl() const;

	/** 设置垂直滚动条封装控件。
	@param [in] scrollBar 垂直滚动条，传入空指针时不处理，控件所有权交给 QScrollArea。
	*/
	void setVerticalScrollBarControl(ScrollBar* scrollBar);

	/** 设置水平滚动条封装控件。
	@param [in] scrollBar 水平滚动条，传入空指针时不处理，控件所有权交给 QScrollArea。
	*/
	void setHorizontalScrollBarControl(ScrollBar* scrollBar);

	/** 设置视口背景颜色。
	@param [in] color 背景颜色。
	@param [in] rePaint 是否立即重画。
	*/
	void setViewportBackgroundColor(const QColor& color, bool rePaint = false);

private:
	/** 设置控件默认滚动条和基础样式。
	*/
	void setDefault();
};