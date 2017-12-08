#include "HtmlLabelComboBox.h"
#include "Label.h"
#include <QListWidgetItem>
#include "ListWidget.h"
#include <QEvent>
#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QProxyStyle>
#include <QPalette>

class DIYLabelStyle : public QProxyStyle
{
public:
	DIYLabelStyle(HtmlLabelComboBox* pHtmlComboBox)
	{
		m_htmlComboBox = pHtmlComboBox;
	}

	void drawItemText(QPainter *painter, const QRect &rect, int flags, const QPalette &pal, bool enabled, const QString &text, QPalette::ColorRole textRole) const
	{
		//QProxyStyle::drawItemText(painter, rect, flags, palbk, enabled, text, textRole);
		painter->save();

		QTextDocument doc;

		std::wstring htmlString = L"<div style='color:%s;'>%s</div>";
		htmlString = CStringManager::Format(htmlString.c_str(),
											pal.color(textRole).name().toStdWString().c_str(),
											text.toStdWString().c_str());
		doc.setHtml(QString::fromStdWString(htmlString));

		QRect drawRect = rect;
		drawRect.setLeft(rect.left());
		painter->translate(drawRect.topLeft() + QPoint(m_htmlComboBox->m_textOrigin, 0));

		doc.setDefaultFont(painter->font());

		QAbstractTextDocumentLayout::PaintContext context;
		doc.documentLayout()->draw(painter, context);

		painter->restore();
	}

	HtmlLabelComboBox* m_htmlComboBox;
};

HtmlLabelComboBox::HtmlLabelComboBox(QWidget* parent):
ComboBox(parent),
m_textOrigin(0)
{
	setStyle(new DIYLabelStyle(this));
	m_listWidget->setStyle(new QProxyStyle);
}

HtmlLabelComboBox::~HtmlLabelComboBox()
{

}

void HtmlLabelComboBox::addItem(const QString& text)
{
	Label* label = new Label;
	label->setText(text);
	QListWidgetItem* widgetItem = new QListWidgetItem(m_listWidget);
	m_listWidget->setItemWidget(widgetItem, label);
	widgetItem->setText(text);
	widgetItem->setToolTip(text);
	m_mapLabel[widgetItem] = label;
	m_listWidget->setItemTextColor(QColor(0, 0, 0, 0), QColor(0, 0, 0, 0), QColor(0, 0, 0, 0));
}

void HtmlLabelComboBox::addItems(const QStringList& textList)
{

}

void HtmlLabelComboBox::setListFontFace(const QString& fontName, bool rePaint)
{
	int32_t index = -1;
	while (index++ != m_listWidget->count() - 1)
	{
		Label* label = (Label*)m_listWidget->itemWidget(m_listWidget->item(index));
		if (label != nullptr)
		{
			label->setFontFace(fontName, rePaint);
		}
	}
}

void HtmlLabelComboBox::setListFontSize(int32_t fontSize, bool rePaint)
{
	int32_t index = -1;
	while (index++ != m_listWidget->count() - 1)
	{
		Label* label = (Label*)m_listWidget->itemWidget(m_listWidget->item(index));
		if (label != nullptr)
		{
			label->setFontSize(fontSize, rePaint);
		}
	}
}

void HtmlLabelComboBox::setListTextColor(const QColor& normalColor,
										 const QColor& disabledColor,
										 bool rePaint)
{
	int32_t index = -1;
	while (index++ != m_listWidget->count() - 1)
	{
		Label* label = (Label*)m_listWidget->itemWidget(m_listWidget->item(index));
		if (label != nullptr)
		{
			label->setTextColor(normalColor, normalColor, disabledColor, rePaint);
		}
	}
}

void HtmlLabelComboBox::setTextOrigin(int32_t origin, bool rePaint)
{
	m_textOrigin = origin;
	ComboBox::setTextOrigin(origin, rePaint);
}

void HtmlLabelComboBox::setListTextColor(const QColor& normalColor,
										 const QColor& hoverColor,
										 const QColor& disabledColor,
										 bool rePaint)
{
	ComboBox::setListTextColor(normalColor, hoverColor, disabledColor, rePaint);
}