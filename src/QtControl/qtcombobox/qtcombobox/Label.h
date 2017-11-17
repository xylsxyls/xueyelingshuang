#pragma once
#include <QtWidgets/QLabel>
#include "ControlFont.h"
#include "ControlBorder.h"

class Label : 
	public ControlFont < QLabel >,
	public ControlBorderForNormalHoverDisabled < QLabel >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	Label(QWidget* parent = NULL);

	/** 设置字体
	@param [in] fontName 字体名
	@param [in] rePaint 是否立即重画
	*/
	void setFontFace(const QString& fontName, bool rePaint = false);

	/** 设置字体大小
	@param [in] fontSize 字体大小
	@param [in] rePaint 是否立即重画
	*/
	void setFontSize(int32_t fontSize, bool rePaint = false);

	/** 设置背景颜色（没有按下效果）
	@param [in] backgroundNormalColor 常态颜色
	@param [in] backgroundHoverColor 悬停颜色
	@param [in] backgroundDisabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundColor(const QColor& backgroundNormalColor,
	 	 		 		    const QColor& backgroundHoverColor,
						    const QColor& backgroundDisabledColor,
							bool rePaint = false);

	/** 设置文本偏移量
	@param [in] origin 文本偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setTextOrigin(int32_t origin, bool rePaint = false);

	/** 插入文本，过长显示省略号
	@param [in] str 字符串
	*/
	void setText(const QString& str);

	/** 获取文本
	@return 返回文本
	*/
	QString text();

private:
	std::wstring m_str;
	int32_t m_leftOrigin;
	std::wstring m_fontName;
	int32_t m_fontSize;
};