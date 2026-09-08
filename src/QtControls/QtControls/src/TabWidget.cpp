#include "TabWidget.h"
#include "ControlSubStyle.h"
#include "TabBar.h"

#include <QVariant>

TabWidget::TabWidget(QWidget* parent) :
ControlShow(parent)
{
	setProperty("class", QVariant(QString("TabWidget")));
	ControlBase::setControlShow(this);
	setDefault();
}

TabWidget::~TabWidget()
{

}

TabBar* TabWidget::tabBarControl() const
{
	return dynamic_cast<TabBar*>(QTabWidget::tabBar());
}

void TabWidget::setTabBarControl(TabBar* tabBar)
{
	if (tabBar == nullptr)
	{
		return;
	}
	QTabWidget::setTabBar(tabBar);
}

void TabWidget::setPaneBorderWidth(qint32 width, bool rePaint)
{
	ControlSubStyle::setPxSolidValue(&m_controlStyle, L"pane", L"border", qMax(width, 0));
	if (rePaint)
	{
		repaint();
	}
}

void TabWidget::setPaneBorderColor(const QColor& color, bool rePaint)
{
	ControlSubStyle::setKeyValue(&m_controlStyle, L"pane", L"border-color", QssHelper::QColorToWString(color));
	if (rePaint)
	{
		repaint();
	}
}

void TabWidget::setPaneBackgroundColor(const QColor& color, bool rePaint)
{
	ControlSubStyle::setKeyValue(&m_controlStyle, L"pane", L"background-color", QssHelper::QColorToWString(color));
	if (rePaint)
	{
		repaint();
	}
}

void TabWidget::setDefault()
{
	setBorderWidth(0);
	setTabBarControl(new TabBar(this));
}