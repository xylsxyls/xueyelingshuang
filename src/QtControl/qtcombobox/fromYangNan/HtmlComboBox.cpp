#include "HtmlComboBox.h"
#include <QPainter>
#include <QTextDocument>
#include "ListWidget.h"
#include <QPainter>
#include <QProxyStyle>
#include <QAbstractTextDocumentLayout>
#include <QStyledItemDelegate>

class DIYStyle : public QProxyStyle
{
public:
	DIYStyle(HtmlComboBox* pHtmlComboBox)
	{
		m_htmlComboBox = pHtmlComboBox;
	}

	void drawItemText(QPainter *painter, const QRect &rect, int flags, const QPalette &pal, bool enabled, const QString &text, QPalette::ColorRole textRole) const
	{
		//QProxyStyle::drawItemText(painter, rect, flags, pal, enabled, text, textRole);
		//return;

		painter->save();

		QTextDocument doc;

		std::wstring htmlString = L"<div style='color:%s;'>%s</div>";
		htmlString = CStringManager::Format(htmlString.c_str(),
											pal.color(textRole).name().toStdWString().c_str(),
											text.toStdWString().c_str());
		doc.setHtml(QString::fromStdWString(htmlString));

		//doc.setDefaultStyleSheet("");

		QPen savedPen = painter->pen();
		painter->setPen(QPen(pal.brush(QPalette::Text), savedPen.widthF()));
		painter->setFont(painter->font());
		//toption.widget->

		QRect drawRect = rect;
		drawRect.setLeft(rect.left() + m_htmlComboBox->m_comboBoxOrigin);
		painter->translate(drawRect.topLeft());
		//painter->set

		doc.setDefaultFont(painter->font());

		QAbstractTextDocumentLayout::PaintContext context;
		doc.documentLayout()->draw(painter, context);

		painter->setPen(savedPen);
		painter->restore();
	}

	HtmlComboBox* m_htmlComboBox;
};

class DIYDelegate : public QStyledItemDelegate
{
public:
	DIYDelegate(HtmlComboBox* pHtmlComboBox)
	{
		m_htmlComboBox = pHtmlComboBox;
	}
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		//QStyledItemDelegate::paint(painter, option, index);
		//return;
		painter->save();

		QStyleOptionViewItem toption = option;
		initStyleOption(&toption, index);

		toption.widget->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &toption, painter, toption.widget);

		QTextDocument doc;

		QColor drawColor;
		if (!option.state.testFlag(QStyle::State_Enabled))
		{
			drawColor = m_htmlComboBox->m_disabledColor;
		}
		else if (option.state.testFlag(QStyle::State_Selected))
		{
			drawColor = m_htmlComboBox->m_selectedColor;
		}
		else if (option.state.testFlag(QStyle::State_MouseOver))
		{
			drawColor = m_htmlComboBox->m_selectedColor;
		}
		else
		{
			drawColor = m_htmlComboBox->m_normalColor;
		}
		
		doc.setHtml(toption.text);
		//font:18px 'ºÚÌå'; 
		std::wstring htmlString = L"<div style=\"color:%s;\">%s</div>";
		htmlString = CStringManager::Format(htmlString.c_str(),
											drawColor.name().toStdWString().c_str(),
											toption.text.toStdWString().c_str());
		doc.setHtml(QString::fromStdWString(htmlString));

		doc.setDocumentMargin(0);
		
		int32_t itemHeight = toption.rect.height();

		QFontMetrics tfontM(toption.font);
		
		int32_t fontHeight = tfontM.height();
		doc.setPageSize(QSize(toption.rect.width() - 
							  m_htmlComboBox->m_origin - 
							  m_htmlComboBox->m_leftOrigin - 
							  m_htmlComboBox->m_rightOrigin,
							  toption.rect.height()));

		painter->translate(toption.rect.topLeft() + QPoint(m_htmlComboBox->m_origin, (itemHeight - fontHeight) / 2));

		doc.setDefaultFont(toption.font);

		QAbstractTextDocumentLayout::PaintContext context;
		doc.documentLayout()->draw(painter, context);

		painter->restore();
		return;
	}

	HtmlComboBox* m_htmlComboBox;
};

HtmlComboBox::HtmlComboBox(QWidget* parent) :
ComboBox(parent)
{
	m_normalColor = QColor(0, 0, 0, 0);
	m_selectedColor = QColor(0, 0, 0, 0);
	m_disabledColor = QColor(0, 0, 0, 0);
	m_comboBoxOrigin = 0;
	m_origin = 0;
	m_leftOrigin = 0;
	m_topOrigin = 0;
	m_rightOrigin = 0;
	m_bottomOrigin = 0;

	setStyle(new DIYStyle(this));
	m_listWidget->setItemDelegate(new DIYDelegate(this));
}

HtmlComboBox::~HtmlComboBox()
{

}
void HtmlComboBox::setTextOrigin(int32_t origin, bool rePaint)
{
	m_comboBoxOrigin = origin;
	ComboBox::setTextOrigin(origin, rePaint);
}

void HtmlComboBox::setListTextColor(const QColor& normalColor,
									const QColor& hoverColor,
									const QColor& disabledColor,
									bool rePaint)
{
	m_normalColor = normalColor;
	m_selectedColor = hoverColor;
	m_disabledColor = disabledColor;
	ComboBox::setListTextColor(normalColor, hoverColor, disabledColor, rePaint);
}

void HtmlComboBox::setListTextOrigin(int32_t origin, bool rePaint)
{
	m_origin = origin;
	ComboBox::setListTextOrigin(origin, rePaint);
}

void HtmlComboBox::setListItemAroundOrigin(int32_t leftOrigin,
										   int32_t topOrigin,
										   int32_t rightOrigin,
										   int32_t bottomOrigin,
										   bool rePaint)
{
	m_leftOrigin = leftOrigin;
	m_topOrigin = topOrigin;
	m_rightOrigin = rightOrigin;
	m_bottomOrigin = bottomOrigin;
	ComboBox::setListItemAroundOrigin(leftOrigin, topOrigin, rightOrigin, bottomOrigin, rePaint);
}