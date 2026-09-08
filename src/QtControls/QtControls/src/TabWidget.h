#pragma once
#include <QtWidgets/QTabWidget>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

class TabBar;

/** 标签页容器控件，封装 QTabWidget 本体、pane 区域和内置 TabBar。
*/
class QtControlsAPI TabWidget :
	public ControlShow < QTabWidget >,
	public ControlFont < QTabWidget >,
	public ControlSelf < QTabWidget >,
	public ControlBorderForNormalHoverDisabled < QTabWidget >,
	public ControlBackgroundForNormalHoverDisabled < QTabWidget >
{
public:
	/** 构造函数。
	@param [in] parent 父窗口指针。
	*/
	TabWidget(QWidget* parent = nullptr);

	/** 析构函数。
	*/
	virtual ~TabWidget();

public:
	/** 获取标签栏封装控件。
	@return 返回当前标签栏封装控件，外部替换为非 TabBar 时返回空指针。
	*/
	TabBar* tabBarControl() const;

	/** 设置标签栏封装控件。
	@param [in] tabBar 标签栏控件，传入空指针时不处理，控件所有权交给 QTabWidget。
	*/
	void setTabBarControl(TabBar* tabBar);

	/** 设置 pane 边框宽度。
	@param [in] width 边框宽度。
	@param [in] rePaint 是否立即重画。
	*/
	void setPaneBorderWidth(qint32 width, bool rePaint = false);

	/** 设置 pane 边框颜色。
	@param [in] color 边框颜色。
	@param [in] rePaint 是否立即重画。
	*/
	void setPaneBorderColor(const QColor& color, bool rePaint = false);

	/** 设置 pane 背景颜色。
	@param [in] color 背景颜色。
	@param [in] rePaint 是否立即重画。
	*/
	void setPaneBackgroundColor(const QColor& color, bool rePaint = false);

private:
	/** 设置控件默认标签栏和基础样式。
	*/
	void setDefault();
};