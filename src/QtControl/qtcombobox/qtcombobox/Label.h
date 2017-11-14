#pragma once
#include <QtWidgets/QLabel>
#include "ControlBase.h"

class Label : public ControlBase < QLabel >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	Label(QWidget* parent = NULL);

	/** 设置背景图片（没有按下效果）
	@param [in] backgroundImg 背景图片路径，如果路径中必须使用正斜杠
	@param [in] backgroundImgStateCount 图片纵向切割个数
	@param [in] backgroundImgNormal 常态图片
	@param [in] backgroundImgHover 悬停图片
	@param [in] backgroundImgDisabled 禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundImage(const QString& backgroundImg,
			   			    int32_t backgroundImgStateCount = 4,
						    int32_t backgroundImgNormal = 1,
						    int32_t backgroundImgHover = 2,
						    int32_t backgroundImgDisabled = 4,
							bool rePaint = false);

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

	/** 设置字体颜色（没有按下效果）
	@param [in] textNormalColor 常态颜色
	@param [in] textHoverColor 悬停颜色
	@param [in] textDisabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setTextColor(const QColor& textNormalColor,
	 	 		 	  const QColor& textHoverColor,
					  const QColor& textDisabledColor,
					  bool rePaint = false);

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

	/** 重画
	*/
	void repaint();

private:
	std::wstring m_str;
	int32_t m_leftOrigin;
	std::wstring m_fontName;
	int32_t m_fontSize;
};