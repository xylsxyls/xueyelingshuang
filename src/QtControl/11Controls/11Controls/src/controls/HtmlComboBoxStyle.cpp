#include "HtmlComboBoxStyle.h"
#include <QPainter>
#include "HtmlComboBox.h"
#include "CGeneralStyle.h"

HtmlComboBoxStyle::HtmlComboBoxStyle(HtmlComboBox* htmlComboBox) :
m_htmlComboBox(htmlComboBox)
{
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