#include "HtmlComboBoxStyle.h"
#include <QPainter>
#include "HtmlComboBox.h"
#include "CGeneralStyle.h"

HtmlComboBoxStyle::HtmlComboBoxStyle(HtmlComboBox* htmlComboBox) :
m_htmlComboBox(htmlComboBox)
{
	m_color = QColor(0, 0, 0, 0);
	setParent(htmlComboBox);
}

HtmlComboBoxStyle::~HtmlComboBoxStyle()
{

}

void HtmlComboBoxStyle::drawItemText(QPainter* painter,
									 const QRect& rect,
									 int flags,
									 const QPalette& pal,
									 bool enabled,
									 const QString& text,
									 QPalette::ColorRole textRole) const
{
	painter->save();

	painter->setPen(pal.color(textRole));
	CGeneralStyle::instance()->drawHtmlTextByLabel(painter,
												   rect,
												   text,
												   Qt::AlignLeft | Qt::AlignVCenter);

	painter->restore();
}

void HtmlComboBoxStyle::setToolTipBackgroundColor(const QColor& color)
{
	m_color = color;
}

void HtmlComboBoxStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption* opt, QPainter* painter, const QWidget* widget) const
{
	if (m_color == QColor(0, 0, 0, 0))
	{
		QProxyStyle::drawPrimitive(pe, opt, painter, widget);
		return;
	}
	painter->save();
	switch (pe)
	{
	case QStyle::PE_PanelTipLabel:
	{
		painter->fillRect(opt->rect, m_color);
		break;
	}
	default:
	{
		QProxyStyle::drawPrimitive(pe, opt, painter, widget);
		break;
	}
	}
	painter->restore();
}
