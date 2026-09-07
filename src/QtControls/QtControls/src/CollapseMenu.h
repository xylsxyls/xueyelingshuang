#pragma once
#include "Menu.h"
#include "QtControlsMacro.h"

class Label;
/** 定做类，折叠式边框菜单
*/
class QtControlsAPI CollapseMenu : public Menu
{
	Q_OBJECT
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
							const QColor& selectedColor = QColor(0, 0, 0, 0),
							const QColor& disabledColor = QColor(0, 0, 0, 0),
							bool rePaint = false);

private:
	/** 将边框设置为折叠式，该类为定做类，默认设为折叠式，不用手动再次调用
	*/
	void setBorderCollapse();

private:
	/** 绘制折叠菜单的选中边框和顶部装饰
	@param [in] eve Qt绘制事件
	*/
	void paintEvent(QPaintEvent *eve);

	/** 鼠标离开菜单时清理悬停状态
	@param [in] eve Qt离开事件
	*/
	void leaveEvent(QEvent* eve);

	/** 鼠标移动时更新当前悬停菜单项
	@param [in] eve Qt鼠标事件
	*/
	void mouseMoveEvent(QMouseEvent *eve);

private:
	// 常态边框颜色
	QColor m_normalColor;
	// 选中状态边框颜色
	QColor m_selectedColor;
	// 是否已经设置选中项顶部装饰标签
	bool m_hasSetSelectedLabelTop;
	// 选中项顶部装饰标签
	Label* m_selectedLabelTop;
	// 是否已经设置选中项底部装饰标签
	bool m_hasSetSelectedLabelBottom;
	// 选中项底部装饰标签
	Label* m_selectedLabelBottom;
	// 菜单顶部装饰标签
	Label* m_topLabel;
	// 是否已经设置菜单顶部装饰标签
	bool m_hasSetTopLabel;
};