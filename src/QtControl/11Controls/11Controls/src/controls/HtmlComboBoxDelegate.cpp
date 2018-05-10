#include "HtmlComboBoxDelegate.h"
#include <QPainter>
#include "HtmlComboBox.h"
#include "CGeneralStyle.h"
#include "../core/CSystem.h"

HtmlComboBoxDelegate::HtmlComboBoxDelegate(HtmlComboBox* htmlComboBox):
m_htmlComboBox(htmlComboBox)
{
	setParent(htmlComboBox);
}

HtmlComboBoxDelegate::~HtmlComboBoxDelegate()
{

}

void HtmlComboBoxDelegate::paint(QPainter* painter,
								 const QStyleOptionViewItem& option,
								 const QModelIndex& index) const
{
	painter->save();

	option.widget->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, option.widget);

	QColor drawColor;
	if (!option.state.testFlag(QStyle::State_Enabled))
	{
		drawColor = m_htmlComboBox->m_disabledColor;
	}
	else
	{
		if (option.state.testFlag(QStyle::State_MouseOver)
			&& option.state.testFlag(QStyle::State_Selected))
		{
			drawColor = m_htmlComboBox->m_selectedColor;
		}
		else
		{
			drawColor = m_htmlComboBox->m_normalColor;
		}
	}
	painter->setPen(drawColor);

	QRect drawRect = option.rect.adjusted(m_htmlComboBox->m_origin +
										  m_htmlComboBox->m_leftOrigin +
										  m_htmlComboBox->m_itemBorderWidth,
										  m_htmlComboBox->m_topOrigin +
										  m_htmlComboBox->m_itemBorderWidth,
										  -m_htmlComboBox->m_rightOrigin -
										  m_htmlComboBox->m_itemBorderWidth,
										  -m_htmlComboBox->m_bottomOrigin -
										  m_htmlComboBox->m_itemBorderWidth);

	CGeneralStyle::instance()->drawHtmlTextByLabel(painter,
                                                   CSystem::rectValid(drawRect),
												   index.data().toString(),
												   Qt::AlignLeft | Qt::AlignVCenter);

	painter->restore();
	return;
}