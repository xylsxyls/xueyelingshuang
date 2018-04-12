#pragma once
#include "Label.h"
#include "ControlsMacro.h"

class Label;
class ControlsAPI ContentLabel
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	ContentLabel(QWidget* parent = nullptr);

public:
	/** 设置上面的颜色
	@param [in] color 文字颜色
	*/
	void setUpColor(const QColor& color);

	/** 设置下面一行的颜色
	@param [in] color 文字颜色
	*/
	void setDownColor(const QColor& color);

	/** 设置为一行模式
	*/
	void setOneLine();

	/** 设置为两行模式
	*/
	void setTwoLine();

	/** 设置上面一行的文字
	@param [in] text 文字内容
	*/
	void setUpText(const QString& text);

	/** 设置下面一行的文字
	@param [in] text 文字内容
	*/
	void setDownText(const QString& text);

	/** 设置大小
	@param [in] rect 控件大小
	*/
	void setGeometry(const QRect& rect);

	/** 设置背景颜色
	@param [in] color 背景颜色
	*/
	void setBackgroundColor(const QColor& color);

	/** 设置字体
	@param [in] fontFace 字体名
	*/
	void setFontFace(const QString& fontFace);

	/** 设置字体大小
	@param [in] size 字体大小
	*/
	void setFontSize(int32_t size);

private:
	/** 更新
	*/
	void update();

	/**	检测
	@return 返回成员变量是否为空
	*/
	bool check();


public:
	Label* m_up;
	Label* m_down;
	QRect m_rect;
	bool m_isOne;
};