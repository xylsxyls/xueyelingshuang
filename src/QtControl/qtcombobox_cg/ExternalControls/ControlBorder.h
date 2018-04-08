#pragma once
#include "ControlBase.h"

/**************该文件的类主要管理背景颜色相关部分**************/

template <class QBase>
class ControlBorderForNormal : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlBorderForNormal();

public:
	/** 设置边框颜色
	@param [in] color 常态
	@param [in] rePaint 是否立即重画
	*/
	void setBorderColor(const QColor& color, bool rePaint = false);

	/** 设置边框图片
	@param [in] borderImgPath 图片路径
	@param [in] rePaint 是否立即重画
	*/
	void setBorderImage(const QString& borderImgPath, bool rePaint = false);

	/** 设置文字颜色
	@param [in] textColor 常态颜色
	@param [in] rePaint 是否立即重画
	*/
	void setTextColor(const QColor& textColor, bool rePaint = false);
};

template <class QBase>
class ControlBorderForNormalHoverDisabled : virtual public ControlBase < QBase >
{

public:
	/** 析构函数
	*/
	virtual ~ControlBorderForNormalHoverDisabled();

public:
	/** 设置背景颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setBorderColor(const QColor& normalColor,
						const QColor& hoverColor = QColor(0, 0, 0, 0),
						const QColor& disabledColor = QColor(0, 0, 0, 0),
						bool rePaint = false);

	/** 设置节点背景图片，和边框颜色不用存
	@param [in] borderImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] borderImgStateCount 上下平分几份
	@param [in] borderImgNormal 常态图片，如果填1表示选最上面的一份
	@param [in] borderImgHover 悬停图片
	@param [in] borderImgDisabled 禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setBorderImage(const QString& borderImgPath,
						int32_t borderImgStateCount = 4,
						int32_t borderImgNormal = 1,
						int32_t borderImgHover = 2,
						int32_t borderImgDisabled = 4,
						bool rePaint = false);

	/** 设置文字颜色
	@param [in] textNormalColor 常态颜色
	@param [in] textHoverColor 悬停颜色
	@param [in] textDisabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setTextColor(const QColor& textNormalColor,
					  const QColor& textHoverColor = QColor(0, 0, 0, 0),
					  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
					  bool rePaint = false);
};

template <class QBase>
class ControlBorderForNormalSelectedDisabled : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlBorderForNormalSelectedDisabled();

public:
	/** 设置背景颜色
	@param [in] normalColor 常态颜色
	@param [in] selectedColor 悬停颜色（用于list）
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setBorderColor(const QColor& normalColor,
						const QColor& selectedColor = QColor(0, 0, 0, 0),
						const QColor& disabledColor = QColor(0, 0, 0, 0),
						bool rePaint = false);

	/** 设置节点背景图片，和边框颜色不用存
	@param [in] borderImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] borderImgStateCount 上下平分几份
	@param [in] borderImgNormal 常态图片，如果填1表示选最上面的一份
	@param [in] borderImgSelected 悬停图片（用于list）
	@param [in] borderImgDisabled 禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setBorderImage(const QString& borderImgPath,
						int32_t borderImgStateCount = 4,
						int32_t borderImgNormal = 1,
						int32_t borderImgSelected = 2,
						int32_t borderImgDisabled = 4,
						bool rePaint = false);

	/** 设置文字颜色
	@param [in] textNormalColor 常态颜色
	@param [in] textSelectedColor 悬停颜色（用于list）
	@param [in] textDisabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setTextColor(const QColor& textNormalColor,
					  const QColor& textSelectedColor = QColor(0, 0, 0, 0),
					  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
					  bool rePaint = false);
};

template <class QBase>
class ControlBorderForNormalHoverPressedDisabled : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlBorderForNormalHoverPressedDisabled();

public:
	/** 设置背景颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] pressedColor 按下颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setBorderColor(const QColor& normalColor,
						const QColor& hoverColor = QColor(0, 0, 0, 0),
						const QColor& pressedColor = QColor(0, 0, 0, 0),
						const QColor& disabledColor = QColor(0, 0, 0, 0),
						bool rePaint = false);

	/** 设置节点背景图片，和边框颜色不用存
	@param [in] borderImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] borderImgStateCount 上下平分几份
	@param [in] borderImgNormal 常态图片，如果填1表示选最上面的一份
	@param [in] borderImgHover 悬停图片
	@param [in] borderImgPressed 按下图片
	@param [in] borderImgDisabled 禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setBorderImage(const QString& borderImgPath,
						int32_t borderImgStateCount = 4,
						int32_t borderImgNormal = 1,
						int32_t borderImgHover = 2,
						int32_t borderImgPressed = 3,
						int32_t borderImgDisabled = 4,
						bool rePaint = false);

	/** 设置文字颜色
	@param [in] textNormalColor 常态颜色
	@param [in] textHoverColor 悬停颜色
	@param [in] textPressedColor 按下颜色
	@param [in] textDisabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setTextColor(const QColor& textNormalColor,
					  const QColor& textHoverColor = QColor(0, 0, 0, 0),
					  const QColor& textPressedColor = QColor(0, 0, 0, 0),
					  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
					  bool rePaint = false);
};

template <class QBase>
class ControlBorderForNormalHoverPressedDisabledAndCheck : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlBorderForNormalHoverPressedDisabledAndCheck();

public:
	/** 设置背景颜色
	@param [in] normalColor 非选中常态颜色
	@param [in] hoverColor 非选中悬停颜色
	@param [in] pressedColor 非选中按下颜色
	@param [in] disabledColor 非选中禁用颜色
	@param [in] normalCkColor 选中常态颜色
	@param [in] hoverCkColor 选中悬停颜色
	@param [in] pressedCkColor 选中按下颜色
	@param [in] disabledCkColor 选中禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setBorderColor(const QColor& normalColor,
						const QColor& hoverColor = QColor(0, 0, 0, 0),
						const QColor& pressedColor = QColor(0, 0, 0, 0),
						const QColor& disabledColor = QColor(0, 0, 0, 0),
						const QColor& normalCkColor = QColor(0, 0, 0, 0),
						const QColor& hoverCkColor = QColor(0, 0, 0, 0),
						const QColor& pressedCkColor = QColor(0, 0, 0, 0),
						const QColor& disabledCkColor = QColor(0, 0, 0, 0),
						bool rePaint = false);

	/** 设置背景图片
	@param [in] borderImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] borderImgStateCount 图片纵向切割个数
	@param [in] borderImgNormal 非选中常态图片，如果填1表示将图片纵向分割4份或8份，选最上面的第一份
	@param [in] borderImgHover 非选中悬停图片
	@param [in] borderImgPressed 非选中按下图片
	@param [in] borderImgDisabled 非选中禁用图片
	@param [in] borderImgCkNormal 选中常态图片
	@param [in] borderImgCkHover 选中悬停图片
	@param [in] borderImgCkPressed 选中按下图片
	@param [in] borderImgCkDisabled 选中禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setBorderImage(const QString& borderImgPath,
						int32_t borderImgStateCount = 8,
						int32_t borderImgNormal = 1,
						int32_t borderImgHover = 2,
						int32_t borderImgPressed = 3,
						int32_t borderImgDisabled = 4,
						int32_t borderImgCkNormal = 5,
						int32_t borderImgCkHover = 6,
						int32_t borderImgCkPressed = 7,
						int32_t borderImgCkDisabled = 8,
						bool rePaint = false);

	/** 设置字体颜色
	@param [in] textNormalColor 非选中常态颜色
	@param [in] textHoverColor 非选中悬停颜色
	@param [in] textPressedColor 非选中按下颜色
	@param [in] textDisabledColor 非选中禁用颜色
	@param [in] textCkNormalColor 选中常态颜色
	@param [in] textCkHoverColor 选中悬停颜色
	@param [in] textCkPressedColor 选中按下颜色
	@param [in] textCkDisabledColor 选中禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setTextColor(const QColor& textNormalColor,
	 	 		 	  const QColor& textHoverColor = QColor(0, 0, 0, 0),
					  const QColor& textPressedColor = QColor(0, 0, 0, 0),
					  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
					  const QColor& textCkNormalColor = QColor(0, 0, 0, 0),
					  const QColor& textCkHoverColor = QColor(0, 0, 0, 0),
					  const QColor& textCkPressedColor = QColor(0, 0, 0, 0),
					  const QColor& textCkDisabledColor = QColor(0, 0, 0, 0),
					  bool rePaint = false);
};

template <class QBase>
class ControlItemBorderForNormal : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlItemBorderForNormal();

public:
	/** 设置边框颜色
	@param [in] color 常态
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderColor(const QColor& color, bool rePaint = false);

	/** 设置边框图片
	@param [in] borderImgPath 图片路径
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderImage(const QString& borderImgPath, bool rePaint = false);

	/** 设置文字颜色
	@param [in] textColor 常态颜色
	@param [in] rePaint 是否立即重画
	*/
	void setItemTextColor(const QColor& textColor, bool rePaint = false);
};

template <class QBase>
class ControlItemBorderForNormalHoverDisabled : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlItemBorderForNormalHoverDisabled();

public:
	/** 设置背景颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderColor(const QColor& normalColor,
							const QColor& hoverColor = QColor(0, 0, 0, 0),
							const QColor& disabledColor = QColor(0, 0, 0, 0),
							bool rePaint = false);

	/** 设置节点背景图片，和边框颜色不用存
	@param [in] borderImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] borderImgStateCount 上下平分几份
	@param [in] borderImgNormal 常态图片，如果填1表示选最上面的一份
	@param [in] borderImgHover 悬停图片
	@param [in] borderImgDisabled 禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderImage(const QString& borderImgPath,
							int32_t borderImgStateCount = 4,
							int32_t borderImgNormal = 1,
							int32_t borderImgHover = 2,
							int32_t borderImgDisabled = 4,
							bool rePaint = false);

	/** 设置文字颜色
	@param [in] textNormalColor 常态颜色
	@param [in] textHoverColor 悬停颜色
	@param [in] textDisabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setItemTextColor(const QColor& textNormalColor,
						  const QColor& textHoverColor = QColor(0, 0, 0, 0),
						  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
						  bool rePaint = false);
};

template <class QBase>
class ControlItemBorderForNormalSelectedDisabled : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlItemBorderForNormalSelectedDisabled();

public:
	/** 设置背景颜色
	@param [in] normalColor 常态颜色
	@param [in] selectedColor 悬停颜色（用于list）
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderColor(const QColor& normalColor,
							const QColor& selectedColor = QColor(0, 0, 0, 0),
							const QColor& disabledColor = QColor(0, 0, 0, 0),
							bool rePaint = false);

	/** 设置节点背景图片，和边框颜色不用存
	@param [in] borderImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] borderImgStateCount 上下平分几份
	@param [in] borderImgNormal 常态图片，如果填1表示选最上面的一份
	@param [in] borderImgSelected 悬停图片（用于list）
	@param [in] borderImgDisabled 禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderImage(const QString& borderImgPath,
							int32_t borderImgStateCount = 4,
							int32_t borderImgNormal = 1,
							int32_t borderImgSelected = 2,
							int32_t borderImgDisabled = 4,
							bool rePaint = false);

	/** 设置文字颜色
	@param [in] textNormalColor 常态颜色
	@param [in] textSelectedColor 悬停颜色（用于list）
	@param [in] textDisabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setItemTextColor(const QColor& textNormalColor,
						  const QColor& textSelectedColor = QColor(0, 0, 0, 0),
						  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
						  bool rePaint = false);
};

template <class QBase>
class ControlItemBorderForNormalHoverPressedDisabled : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlItemBorderForNormalHoverPressedDisabled();

public:
	/** 设置背景颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] pressedColor 按下颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderColor(const QColor& normalColor,
							const QColor& hoverColor = QColor(0, 0, 0, 0),
							const QColor& pressedColor = QColor(0, 0, 0, 0),
							const QColor& disabledColor = QColor(0, 0, 0, 0),
							bool rePaint = false);

	/** 设置节点背景图片，和边框颜色不用存
	@param [in] borderImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] borderImgStateCount 上下平分几份
	@param [in] borderImgNormal 常态图片，如果填1表示选最上面的一份
	@param [in] borderImgHover 悬停图片
	@param [in] borderImgPressed 按下图片
	@param [in] borderImgDisabled 禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderImage(const QString& borderImgPath,
							int32_t borderImgStateCount = 4,
							int32_t borderImgNormal = 1,
							int32_t borderImgHover = 2,
							int32_t borderImgPressed = 3,
							int32_t borderImgDisabled = 4,
							bool rePaint = false);

	/** 设置文字颜色
	@param [in] textNormalColor 常态颜色
	@param [in] textHoverColor 悬停颜色
	@param [in] textPressedColor 按下颜色
	@param [in] textDisabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setItemTextColor(const QColor& textNormalColor,
						  const QColor& textHoverColor = QColor(0, 0, 0, 0),
						  const QColor& textPressedColor = QColor(0, 0, 0, 0),
						  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
						  bool rePaint = false);
};

template <class QBase>
class ControlItemBorderForNormalHoverPressedDisabledAndCheck : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlItemBorderForNormalHoverPressedDisabledAndCheck();

public:
	/** 设置背景颜色
	@param [in] normalColor 非选中常态颜色
	@param [in] hoverColor 非选中悬停颜色
	@param [in] pressedColor 非选中按下颜色
	@param [in] disabledColor 非选中禁用颜色
	@param [in] normalCkColor 选中常态颜色
	@param [in] hoverCkColor 选中悬停颜色
	@param [in] pressedCkColor 选中按下颜色
	@param [in] disabledCkColor 选中禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderColor(const QColor& normalColor,
							const QColor& hoverColor = QColor(0, 0, 0, 0),
							const QColor& pressedColor = QColor(0, 0, 0, 0),
							const QColor& disabledColor = QColor(0, 0, 0, 0),
							const QColor& normalCkColor = QColor(0, 0, 0, 0),
							const QColor& hoverCkColor = QColor(0, 0, 0, 0),
							const QColor& pressedCkColor = QColor(0, 0, 0, 0),
							const QColor& disabledCkColor = QColor(0, 0, 0, 0),
							bool rePaint = false);

	/** 设置背景图片
	@param [in] borderImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] borderImgStateCount 图片纵向切割个数
	@param [in] borderImgNormal 非选中常态图片，如果填1表示将图片纵向分割4份或8份，选最上面的第一份
	@param [in] borderImgHover 非选中悬停图片
	@param [in] borderImgPressed 非选中按下图片
	@param [in] borderImgDisabled 非选中禁用图片
	@param [in] borderImgCkNormal 选中常态图片
	@param [in] borderImgCkHover 选中悬停图片
	@param [in] borderImgCkPressed 选中按下图片
	@param [in] borderImgCkDisabled 选中禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderImage(const QString& borderImgPath,
							int32_t borderImgStateCount = 8,
							int32_t borderImgNormal = 1,
							int32_t borderImgHover = 2,
							int32_t borderImgPressed = 3,
							int32_t borderImgDisabled = 4,
							int32_t borderImgCkNormal = 5,
							int32_t borderImgCkHover = 6,
							int32_t borderImgCkPressed = 7,
							int32_t borderImgCkDisabled = 8,
							bool rePaint = false);

	/** 设置字体颜色
	@param [in] textNormalColor 非选中常态颜色
	@param [in] textHoverColor 非选中悬停颜色
	@param [in] textPressedColor 非选中按下颜色
	@param [in] textDisabledColor 非选中禁用颜色
	@param [in] textCkNormalColor 选中常态颜色
	@param [in] textCkHoverColor 选中悬停颜色
	@param [in] textCkPressedColor 选中按下颜色
	@param [in] textCkDisabledColor 选中禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setItemTextColor(const QColor& textNormalColor,
	 	 		 		  const QColor& textHoverColor = QColor(0, 0, 0, 0),
						  const QColor& textPressedColor = QColor(0, 0, 0, 0),
						  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
						  const QColor& textCkNormalColor = QColor(0, 0, 0, 0),
						  const QColor& textCkHoverColor = QColor(0, 0, 0, 0),
						  const QColor& textCkPressedColor = QColor(0, 0, 0, 0),
						  const QColor& textCkDisabledColor = QColor(0, 0, 0, 0),
						  bool rePaint = false);
};

#include "ControlBorder.inl"
