#pragma once
#include <map>
#include <QtWidgets/QAbstractSpinBox>
#include "ControlBase.h"
#include "QtControlsMacro.h"

/** 数值编辑类控件的上下按钮样式模板，适用于QSpinBox、QDoubleSpinBox和日期时间编辑控件
*/
template <class QBase>
class ControlSpinBox : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlSpinBox();

public:
	/** 设置上下按钮尺寸
	@param [in] width 按钮宽度
	@param [in] height 按钮高度，传入-1时使用width
	@param [in] rePaint 是否立即重画
	*/
	void setSpinButtonSize(qint32 width, qint32 height = -1, bool rePaint = false);

	/** 设置上下按钮宽度
	@param [in] width 按钮宽度
	@param [in] rePaint 是否立即重画
	*/
	void setSpinButtonWidth(qint32 width, bool rePaint = false);

	/** 设置上下按钮高度
	@param [in] height 按钮高度
	@param [in] rePaint 是否立即重画
	*/
	void setSpinButtonHeight(qint32 height, bool rePaint = false);

	/** 设置上下按钮边框宽度
	@param [in] width 边框宽度
	@param [in] rePaint 是否立即重画
	*/
	void setSpinButtonBorderWidth(qint32 width, bool rePaint = false);

	/** 设置上下按钮背景颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] pressedColor 按下颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setSpinButtonBackgroundColor(const QColor& normalColor,
									  const QColor& hoverColor = QColor(0, 0, 0, 0),
									  const QColor& pressedColor = QColor(0, 0, 0, 0),
									  const QColor& disabledColor = QColor(0, 0, 0, 0),
									  bool rePaint = false);

	/** 设置上下按钮边框颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] pressedColor 按下颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setSpinButtonBorderColor(const QColor& normalColor,
								  const QColor& hoverColor = QColor(0, 0, 0, 0),
								  const QColor& pressedColor = QColor(0, 0, 0, 0),
								  const QColor& disabledColor = QColor(0, 0, 0, 0),
								  bool rePaint = false);

	/** 设置上下按钮背景图片
	@param [in] imagePath 图片路径，图片会按状态数量纵向切割
	@param [in] stateCount 图片纵向状态数量
	@param [in] normal 常态图片序号
	@param [in] hover 悬停图片序号
	@param [in] pressed 按下图片序号
	@param [in] disabled 禁用图片序号
	@param [in] rePaint 是否立即重画
	*/
	void setSpinButtonBackgroundImage(const QString& imagePath,
									  qint32 stateCount = 4,
									  qint32 normal = 1,
									  qint32 hover = 2,
									  qint32 pressed = 3,
									  qint32 disabled = 4,
									  bool rePaint = false);

	/** 设置上下箭头尺寸
	@param [in] width 箭头宽度
	@param [in] height 箭头高度，传入-1时使用width
	@param [in] rePaint 是否立即重画
	*/
	void setSpinArrowSize(qint32 width, qint32 height = -1, bool rePaint = false);

	/** 设置向上箭头图片
	@param [in] imagePath 图片路径，图片会按状态数量纵向切割
	@param [in] stateCount 图片纵向状态数量
	@param [in] normal 常态图片序号
	@param [in] hover 悬停图片序号
	@param [in] pressed 按下图片序号
	@param [in] disabled 禁用图片序号
	@param [in] rePaint 是否立即重画
	*/
	void setSpinUpArrowImage(const QString& imagePath,
							 qint32 stateCount = 4,
							 qint32 normal = 1,
							 qint32 hover = 2,
							 qint32 pressed = 3,
							 qint32 disabled = 4,
							 bool rePaint = false);

	/** 设置向下箭头图片
	@param [in] imagePath 图片路径，图片会按状态数量纵向切割
	@param [in] stateCount 图片纵向状态数量
	@param [in] normal 常态图片序号
	@param [in] hover 悬停图片序号
	@param [in] pressed 按下图片序号
	@param [in] disabled 禁用图片序号
	@param [in] rePaint 是否立即重画
	*/
	void setSpinDownArrowImage(const QString& imagePath,
							   qint32 stateCount = 4,
							   qint32 normal = 1,
							   qint32 hover = 2,
							   qint32 pressed = 3,
							   qint32 disabled = 4,
							   bool rePaint = false);

	/** 设置上下按钮是否显示
	@param [in] visible true表示显示Qt原生上下按钮，false表示隐藏
	@param [in] rePaint 是否立即重画
	*/
	void setSpinButtonVisible(bool visible, bool rePaint = false);

protected:
	/** 获取最终显示控件指针
	@return 返回最终控件指针，继承结构不符合控件库约定时返回空指针
	*/
	ControlShow<QBase>* spinControlShow();

	/** 根据四态参数生成状态表
	@param [in] normal 常态值
	@param [in] hover 悬停值
	@param [in] pressed 按下值
	@param [in] disabled 禁用值
	@return 返回可写入QSS工具的状态表
	*/
	template <typename TValue>
	std::map<qint32, std::map<qint32, TValue>> buildSpinStateMap(const TValue& normal,
																 const TValue& hover,
																 const TValue& pressed,
																 const TValue& disabled) const;

	/** 按需刷新最终控件样式
	@param [in] rePaint true表示立即刷新
	*/
	void repaintSpinControl(bool rePaint);
};

#include "ControlSpinBox.inl"