#pragma once
#include <QtWidgets/QTabBar>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlItem.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

/** 标签栏控件，封装QTabBar本体和tab节点的普通、悬停、按下、选中样式
*/
class QtControlsAPI TabBar :
	public ControlShow < QTabBar >,
	public ControlFont < QTabBar >,
	public ControlSelf < QTabBar >,
	public ControlItem < QTabBar >,
	public ControlBorderForNormalHoverDisabled < QTabBar >,
	public ControlBackgroundForNormalHoverDisabled < QTabBar >,
	public ControlItemBorderForNormalHoverPressedDisabled < QTabBar >,
	public ControlItemBackgroundForNormalHoverPressedDisabled < QTabBar >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	TabBar(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~TabBar();

public:
	/** 设置控件默认样式参数
	*/
	void setDefault();

	/** 设置tab节点尺寸
	@param [in] width 节点宽度
	@param [in] height 节点高度，传入-1时使用width
	@param [in] rePaint 是否立即重画
	*/
	void setTabSize(qint32 width, qint32 height = -1, bool rePaint = false);

	/** 设置选中tab背景颜色
	@param [in] color 背景颜色
	@param [in] rePaint 是否立即重画
	*/
	void setSelectedTabBackgroundColor(const QColor& color, bool rePaint = false);

	/** 设置选中tab边框颜色
	@param [in] color 边框颜色
	@param [in] rePaint 是否立即重画
	*/
	void setSelectedTabBorderColor(const QColor& color, bool rePaint = false);

	/** 设置选中tab文字颜色
	@param [in] color 文字颜色
	@param [in] rePaint 是否立即重画
	*/
	void setSelectedTabTextColor(const QColor& color, bool rePaint = false);
};