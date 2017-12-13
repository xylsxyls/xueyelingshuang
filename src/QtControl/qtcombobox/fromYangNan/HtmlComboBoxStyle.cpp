#include "HtmlComboBoxStyle.h"
#include <QPainter>
#include "CStringManager.h"
#include "HtmlComboBox.h"
#include <QAbstractTextDocumentLayout>
#include <QTextDocument>

HtmlComboBoxStyle::HtmlComboBoxStyle(HtmlComboBox* htmlComboBox):
m_htmlComboBox(htmlComboBox),
doc(new QTextDocument)
{
	
}

HtmlComboBoxStyle::~HtmlComboBoxStyle()
{
	delete doc;
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

	std::wstring htmlString = L"<div style='color:%s;'>%s</div>";
	htmlString = CStringManager::Format(htmlString.c_str(),
		pal.color(textRole).name().toStdWString().c_str(),
		text.toStdWString().c_str());
	doc->setHtml(QString::fromStdWString(htmlString));

	doc->setDocumentMargin(0);

	QTextOption textOption;
	textOption.setWrapMode(QTextOption::NoWrap);
	textOption.setAlignment(Qt::AlignVCenter);
	doc->setDefaultTextOption(textOption);

	QRect drawRect = rect;
	drawRect.setLeft(rect.left() + m_htmlComboBox->m_comboBoxOrigin);

	int32_t itemHeight = rect.height();

	QFontMetrics tfontM(painter->font());

	int32_t fontHeight = tfontM.height();

	painter->translate(drawRect.topLeft() + QPoint(0, (itemHeight - fontHeight - 3) / 2));

	doc->setDefaultFont(painter->font());

	QAbstractTextDocumentLayout::PaintContext context;
	doc->documentLayout()->draw(painter, context);

	painter->restore();
}