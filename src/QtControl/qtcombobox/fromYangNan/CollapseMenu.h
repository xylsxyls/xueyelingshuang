#pragma once
#include "Menu.h"

class Label;
/** 定做类，折叠式边框菜单
*/
class CollapseMenu : public Menu
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	CollapseMenu(QWidget* parent = nullptr);

	/** 构造函数
	@param [in] title 菜单标题
	@param [in] icon 菜单图标
	@param [in] size 菜单图标大小
	@param [in] parent 父窗口指针
	*/
	CollapseMenu(const QString& title, const QString& icon = QString(), const QSize& size = QSize(), QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~CollapseMenu();

public:
	/** 设置背景颜色
	@param [in] normalColor 常态颜色
	@param [in] selectedColor 悬停颜色（用于list）
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderColor(const QColor& normalColor,
							const QColor& selectedColor,
							const QColor& disabledColor,
							bool rePaint = false);

private:
	/** 将边框设置为折叠式，该类为定做类，默认设为折叠式，不用手动再次调用
	*/
	void setBorderCollapse();

private:
	void paintEvent(QPaintEvent *eve);
	void leaveEvent(QEvent* eve);
	void mouseMoveEvent(QMouseEvent *eve);

private:
	QColor m_normalColor;
	QColor m_selectedColor;

	bool m_hasSetSelectedLabelTop;
	Label* m_selectedLabelTop;
	bool m_hasSetSelectedLabelBottom;
	Label* m_selectedLabelBottom;
	Label* m_topLabel;
	bool m_hasSetTopLabel;
};