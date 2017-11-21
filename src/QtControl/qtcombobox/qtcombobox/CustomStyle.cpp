#include "CustomStyle.h"
#include <QStyleOptionMenuItem>

#include <QPainter>
#include <QComboBox>


CustomStyle::CustomStyle(QStyle *style)
	:QProxyStyle(style),
	m_size(20)
{
	
}

void CustomStyle::setMaxSize(int32_t size)
{
	m_size = size;
}

int CustomStyle::pixelMetric(PixelMetric pm, const QStyleOption * opt, const QWidget * widget) const
{

	if (pm == QStyle::PM_SmallIconSize)
		return m_size;
	return __super::pixelMetric(pm, opt, widget);
}