#pragma once
#include <QProxyStyle>
#include "ControlsMacro.h"

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
	
private:
	HtmlComboBox* m_htmlComboBox;
};