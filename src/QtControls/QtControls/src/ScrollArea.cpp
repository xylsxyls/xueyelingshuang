#include "ScrollArea.h"
#include "ControlSubStyle.h"
#include "ScrollBar.h"

#include <QVariant>

ScrollArea::ScrollArea(QWidget* parent) :
ControlShow(parent)
{
	setProperty("class", QVariant(QString("ScrollArea")));
	ControlBase::setControlShow(this);
	setDefault();
}

ScrollArea::~ScrollArea()
{

}

ScrollBar* ScrollArea::verticalScrollBarControl() const
{
	return dynamic_cast<ScrollBar*>(verticalScrollBar());
}

ScrollBar* ScrollArea::horizontalScrollBarControl() const
{
	return dynamic_cast<ScrollBar*>(horizontalScrollBar());
}

void ScrollArea::setVerticalScrollBarControl(ScrollBar* scrollBar)
{
	if (scrollBar == nullptr)
	{
		return;
	}
	scrollBar->setOrientation(Qt::Vertical);
	setVerticalScrollBar(scrollBar);
}

void ScrollArea::setHorizontalScrollBarControl(ScrollBar* scrollBar)
{
	if (scrollBar == nullptr)
	{
		return;
	}
	scrollBar->setOrientation(Qt::Horizontal);
	setHorizontalScrollBar(scrollBar);
}

void ScrollArea::setViewportBackgroundColor(const QColor& color, bool rePaint)
{
	ControlSubStyle::setDescendantKeyValue(&m_controlStyle, L"QWidget", L"background-color", QssHelper::QColorToWString(color));
	if (rePaint)
	{
		repaint();
	}
}

void ScrollArea::setDefault()
{
	setBorderWidth(0);
	setWidgetResizable(true);
	setVerticalScrollBarControl(new ScrollBar(Qt::Vertical, this));
	setHorizontalScrollBarControl(new ScrollBar(Qt::Horizontal, this));
}