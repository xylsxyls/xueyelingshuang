#include "HtmlComboBoxDelegate.h"
#include <QTextDocument>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include "HtmlComboBox.h"

HtmlComboBoxDelegate::HtmlComboBoxDelegate(HtmlComboBox* htmlComboBox):
m_htmlComboBox(htmlComboBox),
doc(new QTextDocument)
{

}

HtmlComboBoxDelegate::~HtmlComboBoxDelegate()
{
	delete doc;
}

void HtmlComboBoxDelegate::paint(QPainter *painter,
								 const QStyleOptionViewItem &option,
								 const QModelIndex &index) const
{
	painter->save();
		
	QStyleOptionViewItem toption = option;
	initStyleOption(&toption, index);

	toption.widget->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &toption, painter, toption.widget);

	QTextOption textOption;
	textOption.setWrapMode(QTextOption::NoWrap);
	textOption.setAlignment(Qt::AlignVCenter);
	doc->setDefaultTextOption(textOption);

	QColor drawColor;
	if (!option.state.testFlag(QStyle::State_Enabled))
	{
		drawColor = m_htmlComboBox->m_disabledColor;
	}
	else
	{
		if (option.state.testFlag(QStyle::State_MouseOver))
		{
			if (option.state.testFlag(QStyle::State_Selected))
			{
				drawColor = m_htmlComboBox->m_selectedColor;
			}
		}
		else
		{
			drawColor = m_htmlComboBox->m_normalColor;
		}
	}
		
	doc->setHtml(toption.text);

	std::wstring htmlString = L"<div style=\"color:%s;\">%s</div>";
	htmlString = CStringManager::Format(htmlString.c_str(),
										drawColor.name().toStdWString().c_str(),
										toption.text.toStdWString().c_str());
	doc->setHtml(QString::fromStdWString(htmlString));

	doc->setDocumentMargin(0);
		
	int32_t itemHeight = toption.rect.height();

	QFontMetrics tfontM(toption.font);
		
	int32_t fontHeight = tfontM.height();
	doc->setPageSize(QSize(toption.rect.width() -
						   m_htmlComboBox->m_origin - 
						   m_htmlComboBox->m_itemBorderWidth * 2,
						   toption.rect.height()));
	painter->setClipRect(option.rect.adjusted(m_htmlComboBox->m_leftOrigin + 
											  m_htmlComboBox->m_itemBorderWidth,
											  m_htmlComboBox->m_topOrigin,
											  -m_htmlComboBox->m_rightOrigin - 
											  m_htmlComboBox->m_itemBorderWidth,
											  -m_htmlComboBox->m_bottomOrigin));

	painter->translate(toption.rect.topLeft() + 
					   QPoint(m_htmlComboBox->m_origin +
							  m_htmlComboBox->m_leftOrigin +
							  m_htmlComboBox->m_itemBorderWidth,
							  (itemHeight - fontHeight - 3) / 2));

	doc->setDefaultFont(toption.font);

	QAbstractTextDocumentLayout::PaintContext context;
	doc->documentLayout()->draw(painter, context);

	painter->restore();
}