#include "MenuCustomStyle.h"

MenuCustomStyle::MenuCustomStyle(QStyle *style):
QProxyStyle(style),
m_size(20)
{
	
}

void MenuCustomStyle::setMaxSize(qint32 size)
{
	m_size = size;
}

int MenuCustomStyle::pixelMetric(PixelMetric pm, const QStyleOption * opt, const QWidget * widget) const
{
	if (pm == QStyle::PM_SmallIconSize)
	{
		return m_size;
	}
	return QProxyStyle::pixelMetric(pm, opt, widget);
}