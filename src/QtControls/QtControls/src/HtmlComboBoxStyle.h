#pragma once
#include <QProxyStyle>
#include "QtControlsMacro.h"

class HtmlComboBox;
/** 专门给HtmlComboBox使用的Style
*/
class HtmlComboBoxStyle : public QProxyStyle
{
public:
	/** 构造函数
	*/
	HtmlComboBoxStyle(HtmlComboBox* htmlComboBox);

	/** 析构函数
	*/
	~HtmlComboBoxStyle();

public:
	/** 设置提示框背景色
	@param [in] color 背景颜色
	*/
	void setToolTipBackgroundColor(const QColor& color);

private:
	/** 画节点内容，QProxyStyle中的虚函数
	@param [in] painter 绘画指针
	@param [in] rect 文字所画区域
	@param [in] flags 文字对齐方式
	@param [in] pal 颜色集合类
	@param [in] enabled 节点是否可用
	@param [in] text 节点文字
	@param [in] textRole 颜色类型
	*/
	virtual void drawItemText(QPainter* painter,
							  const QRect& rect,
							  int flags,
							  const QPalette& pal,
							  bool enabled,
							  const QString& text,
							  QPalette::ColorRole textRole) const;

	/** 绘制控件相关内容函数
	@param [in] pe 绘制类型
	@param [in] opt 绘制参数
	@param [in] painter 绘制类
	@param [in] widget 父窗口指针
	*/
	void drawPrimitive(PrimitiveElement pe, const QStyleOption* opt, QPainter* painter, const QWidget* widget = nullptr) const;
	
private:
	HtmlComboBox* m_htmlComboBox;
	QColor m_color;
};