#include "TabBar.h"
#include "ControlSubStyle.h"

#include <QVariant>

TabBar::TabBar(QWidget* parent) :
ControlShow(parent)
{
	setProperty("class", QVariant(QString("TabBar")));
	ControlBase::setControlShow(this);
	setItemName(L"tab");
	setDefault();
}

TabBar::~TabBar()
{

}

void TabBar::setDefault()
{
	setBorderWidth(0);
	setItemBorderWidth(0);
	setDocumentMode(true);
	setExpanding(false);
	setMouseTracking(true);
}

void TabBar::setTabSize(qint32 width, qint32 height, bool rePaint)
{
	const qint32 realWidth = qMax(width, 0);
	const qint32 realHeight = qMax(height < 0 ? realWidth : height, 0);
	setItemWidth(realWidth, false);
	setItemHeight(realHeight, rePaint);
}

void TabBar::setSelectedTabBackgroundColor(const QColor& color, bool rePaint)
{
	ControlSubStyle::setKeyValue(&m_controlStyle, L"tab:selected", L"background-color", QssHelper::QColorToWString(color));
	if (rePaint)
	{
		repaint();
	}
}

void TabBar::setSelectedTabBorderColor(const QColor& color, bool rePaint)
{
	ControlSubStyle::setKeyValue(&m_controlStyle, L"tab:selected", L"border-color", QssHelper::QColorToWString(color));
	if (rePaint)
	{
		repaint();
	}
}

void TabBar::setSelectedTabTextColor(const QColor& color, bool rePaint)
{
	ControlSubStyle::setKeyValue(&m_controlStyle, L"tab:selected", L"color", QssHelper::QColorToWString(color));
	if (rePaint)
	{
		repaint();
	}
}