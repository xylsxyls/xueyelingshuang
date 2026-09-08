#pragma once
#include <string>
#include <QtWidgets/QScrollBar>
#include "ControlShow.h"
#include "ControlSelf.h"
#include "ControlItem.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

/** 滚动条控件，封装滑块、翻页区域、两端按钮和方向箭头样式
*/
class QtControlsAPI ScrollBar :
	public ControlShow < QScrollBar >,
	public ControlSelf < QScrollBar >,
	public ControlItem < QScrollBar >,
	public ControlBorderForNormalHoverDisabled < QScrollBar >,
	public ControlBackgroundForNormalHoverDisabled < QScrollBar >,
	public ControlItemBorderForNormalHoverPressedDisabled < QScrollBar >,
	public ControlItemBackgroundForNormalHoverPressedDisabled < QScrollBar >
{
public:
	/** 构造函数，默认创建垂直滚动条
	@param [in] parent 父窗口指针
	*/
	ScrollBar(QWidget* parent = nullptr);

	/** 构造函数
	@param [in] orientation 滚动条方向
	@param [in] parent 父窗口指针
	*/
	ScrollBar(Qt::Orientation orientation, QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~ScrollBar();

public:
	/** 设置控件默认样式参数
	*/
	void setDefault();

	/** 设置滑块最小尺寸
	@param [in] size 最小宽度和最小高度
	@param [in] rePaint 是否立即重画
	*/
	void setHandleMinSize(qint32 size, bool rePaint = false);

	/** 设置两端按钮尺寸
	@param [in] size 按钮宽高
	@param [in] rePaint 是否立即重画
	*/
	void setLineSize(qint32 size, bool rePaint = false);

	/** 设置两端按钮是否显示
	@param [in] visible true表示显示，false表示隐藏
	@param [in] rePaint 是否立即重画
	*/
	void setLineVisible(bool visible, bool rePaint = false);

	/** 设置滑槽已减小区域背景颜色
	@param [in] color 背景颜色
	@param [in] rePaint 是否立即重画
	*/
	void setSubPageBackgroundColor(const QColor& color, bool rePaint = false);

	/** 设置滑槽已增加区域背景颜色
	@param [in] color 背景颜色
	@param [in] rePaint 是否立即重画
	*/
	void setAddPageBackgroundColor(const QColor& color, bool rePaint = false);

	/** 设置两端按钮背景颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] pressedColor 按下颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setLineBackgroundColor(const QColor& normalColor,
								const QColor& hoverColor = QColor(0, 0, 0, 0),
								const QColor& pressedColor = QColor(0, 0, 0, 0),
								const QColor& disabledColor = QColor(0, 0, 0, 0),
								bool rePaint = false);

	/** 设置方向箭头尺寸
	@param [in] width 箭头宽度
	@param [in] height 箭头高度，传入-1时使用width
	@param [in] rePaint 是否立即重画
	*/
	void setArrowSize(qint32 width, qint32 height = -1, bool rePaint = false);

	/** 设置向上箭头图片
	@param [in] imagePath 图片路径，图片会按状态数量纵向切割
	@param [in] stateCount 图片纵向状态数量
	@param [in] normal 常态图片序号
	@param [in] hover 悬停图片序号
	@param [in] pressed 按下图片序号
	@param [in] disabled 禁用图片序号
	@param [in] rePaint 是否立即重画
	*/
	void setUpArrowImage(const QString& imagePath, qint32 stateCount = 4, qint32 normal = 1, qint32 hover = 2, qint32 pressed = 3, qint32 disabled = 4, bool rePaint = false);

	/** 设置向下箭头图片
	@param [in] imagePath 图片路径，图片会按状态数量纵向切割
	@param [in] stateCount 图片纵向状态数量
	@param [in] normal 常态图片序号
	@param [in] hover 悬停图片序号
	@param [in] pressed 按下图片序号
	@param [in] disabled 禁用图片序号
	@param [in] rePaint 是否立即重画
	*/
	void setDownArrowImage(const QString& imagePath, qint32 stateCount = 4, qint32 normal = 1, qint32 hover = 2, qint32 pressed = 3, qint32 disabled = 4, bool rePaint = false);

	/** 设置向左箭头图片
	@param [in] imagePath 图片路径，图片会按状态数量纵向切割
	@param [in] stateCount 图片纵向状态数量
	@param [in] normal 常态图片序号
	@param [in] hover 悬停图片序号
	@param [in] pressed 按下图片序号
	@param [in] disabled 禁用图片序号
	@param [in] rePaint 是否立即重画
	*/
	void setLeftArrowImage(const QString& imagePath, qint32 stateCount = 4, qint32 normal = 1, qint32 hover = 2, qint32 pressed = 3, qint32 disabled = 4, bool rePaint = false);

	/** 设置向右箭头图片
	@param [in] imagePath 图片路径，图片会按状态数量纵向切割
	@param [in] stateCount 图片纵向状态数量
	@param [in] normal 常态图片序号
	@param [in] hover 悬停图片序号
	@param [in] pressed 按下图片序号
	@param [in] disabled 禁用图片序号
	@param [in] rePaint 是否立即重画
	*/
	void setRightArrowImage(const QString& imagePath, qint32 stateCount = 4, qint32 normal = 1, qint32 hover = 2, qint32 pressed = 3, qint32 disabled = 4, bool rePaint = false);

private:
	/** 初始化滚动条通用状态
	*/
	void init();

	/** 设置指定箭头图片
	@param [in] subControl 箭头子控件名称
	@param [in] imagePath 图片路径
	@param [in] stateCount 图片纵向状态数量
	@param [in] normal 常态图片序号
	@param [in] hover 悬停图片序号
	@param [in] pressed 按下图片序号
	@param [in] disabled 禁用图片序号
	@param [in] rePaint 是否立即重画
	*/
	void setArrowImage(const std::wstring& subControl, const QString& imagePath, qint32 stateCount, qint32 normal, qint32 hover, qint32 pressed, qint32 disabled, bool rePaint);
};