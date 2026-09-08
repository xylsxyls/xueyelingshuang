#pragma once
#include <QtWidgets/QToolButton>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

/** 工具按钮控件，封装图标按钮、弹出菜单指示器和四态按钮样式
*/
class QtControlsAPI ToolButton :
	public ControlShow < QToolButton >,
	public ControlFont < QToolButton >,
	public ControlSelf < QToolButton >,
	public ControlBorderForNormalHoverPressedDisabled < QToolButton >,
	public ControlBackgroundForNormalHoverPressedDisabled < QToolButton >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	ToolButton(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~ToolButton();

public:
	/** 设置控件默认样式参数
	*/
	void setDefault();

	/** 设置图标尺寸
	@param [in] width 图标宽度
	@param [in] height 图标高度，传入-1时使用width
	*/
	void setIconSizeValue(qint32 width, qint32 height = -1);

	/** 设置菜单指示器是否显示
	@param [in] visible true表示显示，false表示隐藏
	@param [in] rePaint 是否立即重画
	*/
	void setMenuIndicatorVisible(bool visible, bool rePaint = false);

	/** 设置菜单指示器尺寸
	@param [in] width 指示器宽度
	@param [in] height 指示器高度，传入-1时使用width
	@param [in] rePaint 是否立即重画
	*/
	void setMenuIndicatorSize(qint32 width, qint32 height = -1, bool rePaint = false);

	/** 设置菜单指示器图片
	@param [in] imagePath 图片路径，图片会按状态数量纵向切割
	@param [in] stateCount 图片纵向状态数量
	@param [in] normal 常态图片序号
	@param [in] hover 悬停图片序号
	@param [in] pressed 按下图片序号
	@param [in] disabled 禁用图片序号
	@param [in] rePaint 是否立即重画
	*/
	void setMenuIndicatorImage(const QString& imagePath,
							   qint32 stateCount = 4,
							   qint32 normal = 1,
							   qint32 hover = 2,
							   qint32 pressed = 3,
							   qint32 disabled = 4,
							   bool rePaint = false);
};