#pragma once
#include "ControlBase.h"
#include "ControlsMacro.h"

/**************该文件的类主要管理背景颜色相关部分**************/

template <class QBase>
class ControlBackgroundForNormal : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlBackgroundForNormal();

public:
	/** 设置边框颜色
	@param [in] color 常态
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundColor(const QColor& color, bool rePaint = false);

	/** 设置边框图片
	@param [in] backgroundImgPath 图片路径
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundImage(const QString& backgroundImgPath, bool rePaint = false);
};

template <class QBase>
class ControlBackgroundForNormalHoverDisabled : virtual public ControlBase < QBase >
{

public:
	/** 析构函数
	*/
	virtual ~ControlBackgroundForNormalHoverDisabled();

public:
	/** 设置背景颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundColor(const QColor& normalColor,
							const QColor& hoverColor = QColor(0, 0, 0, 0),
							const QColor& disabledColor = QColor(0, 0, 0, 0),
							bool rePaint = false);

	/** 设置节点背景图片，和边框颜色不用存
	@param [in] backgroundImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] backgroundImgStateCount 上下平分几份
	@param [in] backgroundImgNormal 常态图片，如果填1表示选最上面的一份
	@param [in] backgroundImgHover 悬停图片
	@param [in] backgroundImgDisabled 禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundImage(const QString& backgroundImgPath,
							qint32 backgroundImgStateCount = 4,
							qint32 backgroundImgNormal = 1,
							qint32 backgroundImgHover = 2,
							qint32 backgroundImgDisabled = 4,
							bool rePaint = false);
};

template <class QBase>
class ControlBackgroundForNormalSelectedDisabled : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlBackgroundForNormalSelectedDisabled();

public:
	/** 设置背景颜色
	@param [in] normalColor 常态颜色
	@param [in] selectedColor 悬停颜色（用于list）
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundColor(const QColor& normalColor,
							const QColor& selectedColor = QColor(0, 0, 0, 0),
							const QColor& disabledColor = QColor(0, 0, 0, 0),
							bool rePaint = false);

	/** 设置节点背景图片，和边框颜色不用存
	@param [in] backgroundImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] backgroundImgStateCount 上下平分几份
	@param [in] backgroundImgNormal 常态图片，如果填1表示选最上面的一份
	@param [in] backgroundImgSelected 悬停图片（用于list）
	@param [in] backgroundImgDisabled 禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundImage(const QString& backgroundImgPath,
							qint32 backgroundImgStateCount = 4,
							qint32 backgroundImgNormal = 1,
							qint32 backgroundImgSelected = 2,
							qint32 backgroundImgDisabled = 4,
							bool rePaint = false);
};

template <class QBase>
class ControlBackgroundForNormalHoverPressedDisabled : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlBackgroundForNormalHoverPressedDisabled();

public:
	/** 设置背景颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] pressedColor 按下颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundColor(const QColor& normalColor,
							const QColor& hoverColor = QColor(0, 0, 0, 0),
							const QColor& pressedColor = QColor(0, 0, 0, 0),
							const QColor& disabledColor = QColor(0, 0, 0, 0),
							bool rePaint = false);

	/** 设置节点背景图片，和边框颜色不用存
	@param [in] backgroundImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] backgroundImgStateCount 上下平分几份
	@param [in] backgroundImgNormal 常态图片，如果填1表示选最上面的一份
	@param [in] backgroundImgHover 悬停图片
	@param [in] backgroundImgPressed 按下图片
	@param [in] backgroundImgDisabled 禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundImage(const QString& backgroundImgPath,
							qint32 backgroundImgStateCount = 4,
							qint32 backgroundImgNormal = 1,
							qint32 backgroundImgHover = 2,
							qint32 backgroundImgPressed = 3,
							qint32 backgroundImgDisabled = 4,
							bool rePaint = false);
};

template <class QBase>
class ControlBackgroundForNormalHoverPressedDisabledAndCheck : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlBackgroundForNormalHoverPressedDisabledAndCheck();

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
	void setBackgroundColor(const QColor& normalColor,
							const QColor& hoverColor = QColor(0, 0, 0, 0),
							const QColor& pressedColor = QColor(0, 0, 0, 0),
							const QColor& disabledColor = QColor(0, 0, 0, 0),
							const QColor& normalCkColor = QColor(0, 0, 0, 0),
							const QColor& hoverCkColor = QColor(0, 0, 0, 0),
							const QColor& pressedCkColor = QColor(0, 0, 0, 0),
							const QColor& disabledCkColor = QColor(0, 0, 0, 0),
							bool rePaint = false);

	/** 设置背景图片
	@param [in] backgroundImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] backgroundImgStateCount 图片纵向切割个数
	@param [in] backgroundImgNormal 非选中常态图片，如果填1表示将图片纵向分割4份或8份，选最上面的第一份
	@param [in] backgroundImgHover 非选中悬停图片
	@param [in] backgroundImgPressed 非选中按下图片
	@param [in] backgroundImgDisabled 非选中禁用图片
	@param [in] backgroundImgCkNormal 选中常态图片
	@param [in] backgroundImgCkHover 选中悬停图片
	@param [in] backgroundImgCkPressed 选中按下图片
	@param [in] backgroundImgCkDisabled 选中禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundImage(const QString& backgroundImgPath,
							qint32 backgroundImgStateCount = 8,
							qint32 backgroundImgNormal = 1,
							qint32 backgroundImgHover = 2,
							qint32 backgroundImgPressed = 3,
							qint32 backgroundImgDisabled = 4,
							qint32 backgroundImgCkNormal = 5,
							qint32 backgroundImgCkHover = 6,
							qint32 backgroundImgCkPressed = 7,
							qint32 backgroundImgCkDisabled = 8,
							bool rePaint = false);
};

template <class QBase>
class ControlItemBackgroundForNormal : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlItemBackgroundForNormal();

public:
	/** 设置边框颜色
	@param [in] color 常态
	@param [in] rePaint 是否立即重画
	*/
	void setItemBackgroundColor(const QColor& color, bool rePaint = false);

	/** 设置边框图片
	@param [in] backgroundImgPath 图片路径
	@param [in] rePaint 是否立即重画
	*/
	void setItemBackgroundImage(const QString& backgroundImgPath, bool rePaint = false);
};

template <class QBase>
class ControlItemBackgroundForNormalHoverDisabled : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlItemBackgroundForNormalHoverDisabled();

public:
	/** 设置背景颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setItemBackgroundColor(const QColor& normalColor,
								const QColor& hoverColor = QColor(0, 0, 0, 0),
								const QColor& disabledColor = QColor(0, 0, 0, 0),
								bool rePaint = false);

	/** 设置节点背景图片，和边框颜色不用存
	@param [in] backgroundImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] backgroundImgStateCount 上下平分几份
	@param [in] backgroundImgNormal 常态图片，如果填1表示选最上面的一份
	@param [in] backgroundImgHover 悬停图片
	@param [in] backgroundImgDisabled 禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setItemBackgroundImage(const QString& backgroundImgPath,
								qint32 backgroundImgStateCount = 4,
								qint32 backgroundImgNormal = 1,
								qint32 backgroundImgHover = 2,
								qint32 backgroundImgDisabled = 4,
								bool rePaint = false);
};

template <class QBase>
class ControlItemBackgroundForNormalSelectedDisabled : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlItemBackgroundForNormalSelectedDisabled();

public:
	/** 设置背景颜色
	@param [in] normalColor 常态颜色
	@param [in] selectedColor 悬停颜色（用于list）
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setItemBackgroundColor(const QColor& normalColor,
								const QColor& selectedColor = QColor(0, 0, 0, 0),
								const QColor& disabledColor = QColor(0, 0, 0, 0),
								bool rePaint = false);

	/** 设置节点背景图片，和边框颜色不用存
	@param [in] backgroundImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] backgroundImgStateCount 上下平分几份
	@param [in] backgroundImgNormal 常态图片，如果填1表示选最上面的一份
	@param [in] backgroundImgSelected 悬停图片（用于list）
	@param [in] backgroundImgDisabled 禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setItemBackgroundImage(const QString& backgroundImgPath,
								qint32 backgroundImgStateCount = 4,
								qint32 backgroundImgNormal = 1,
								qint32 backgroundImgSelected = 2,
								qint32 backgroundImgDisabled = 4,
								bool rePaint = false);
};

template <class QBase>
class ControlItemBackgroundForNormalHoverPressedDisabled : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlItemBackgroundForNormalHoverPressedDisabled();

public:
	/** 设置背景颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] pressedColor 按下颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setItemBackgroundColor(const QColor& normalColor,
								const QColor& hoverColor = QColor(0, 0, 0, 0),
								const QColor& pressedColor = QColor(0, 0, 0, 0),
								const QColor& disabledColor = QColor(0, 0, 0, 0),
								bool rePaint = false);

	/** 设置节点背景图片，和边框颜色不用存
	@param [in] backgroundImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] backgroundImgStateCount 上下平分几份
	@param [in] backgroundImgNormal 常态图片，如果填1表示选最上面的一份
	@param [in] backgroundImgHover 悬停图片
	@param [in] backgroundImgPressed 按下图片
	@param [in] backgroundImgDisabled 禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setItemBackgroundImage(const QString& backgroundImgPath,
								qint32 backgroundImgStateCount = 4,
								qint32 backgroundImgNormal = 1,
								qint32 backgroundImgHover = 2,
								qint32 backgroundImgPressed = 3,
								qint32 backgroundImgDisabled = 4,
								bool rePaint = false);
};

template <class QBase>
class ControlItemBackgroundForNormalHoverPressedDisabledAndCheck : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlItemBackgroundForNormalHoverPressedDisabledAndCheck();

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
	void setItemBackgroundColor(const QColor& normalColor,
								const QColor& hoverColor = QColor(0, 0, 0, 0),
								const QColor& pressedColor = QColor(0, 0, 0, 0),
								const QColor& disabledColor = QColor(0, 0, 0, 0),
								const QColor& normalCkColor = QColor(0, 0, 0, 0),
								const QColor& hoverCkColor = QColor(0, 0, 0, 0),
								const QColor& pressedCkColor = QColor(0, 0, 0, 0),
								const QColor& disabledCkColor = QColor(0, 0, 0, 0),
								bool rePaint = false);

	/** 设置背景图片
	@param [in] backgroundImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] backgroundImgStateCount 图片纵向切割个数
	@param [in] backgroundImgNormal 非选中常态图片，如果填1表示将图片纵向分割4份或8份，选最上面的第一份
	@param [in] backgroundImgHover 非选中悬停图片
	@param [in] backgroundImgPressed 非选中按下图片
	@param [in] backgroundImgDisabled 非选中禁用图片
	@param [in] backgroundImgCkNormal 选中常态图片
	@param [in] backgroundImgCkHover 选中悬停图片
	@param [in] backgroundImgCkPressed 选中按下图片
	@param [in] backgroundImgCkDisabled 选中禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setItemBackgroundImage(const QString& backgroundImgPath,
								qint32 backgroundImgStateCount = 8,
								qint32 backgroundImgNormal = 1,
								qint32 backgroundImgHover = 2,
								qint32 backgroundImgPressed = 3,
								qint32 backgroundImgDisabled = 4,
								qint32 backgroundImgCkNormal = 5,
								qint32 backgroundImgCkHover = 6,
								qint32 backgroundImgCkPressed = 7,
								qint32 backgroundImgCkDisabled = 8,
								bool rePaint = false);
};

#include "ControlBackground.inl"
