#include "TreeViewHeader.h"
#include <QPainter>
#include "TreeViewModel.h"

TreeViewHeader::TreeViewHeader(QWidget* parent):
QHeaderView(Qt::Horizontal, parent)
{
	init();
}

TreeViewHeader::~TreeViewHeader()
{
	
}

TreeViewModel* TreeViewHeader::model() const
{
	return (TreeViewModel*)QHeaderView::model();
}

void TreeViewHeader::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const
{
	painter->save();
	TreeText& text = (*(model()->headerText()))[logicalIndex];

	QColor backgroundColor = QColor(0, 0, 0, 0);
	QStyleOptionHeader optionHeader;
	optionHeader.initFrom(this);
	bool hoverd = optionHeader.state.testFlag(QStyle::State_MouseOver);
	bool enabled = optionHeader.state.testFlag(QStyle::State_Enabled);
	bool isCurrentOver = rect.contains(mapFromGlobal(cursor().pos()));

	if (enabled)
	{
		if (hoverd)
		{
			if (isCurrentOver)
			{
				backgroundColor = text.m_hoverColor;
			}
			else
			{
				backgroundColor = text.m_backgroundColor;
			}
		}
		else
		{
			backgroundColor = text.m_backgroundColor;
		}
	}
	else
	{
		backgroundColor = text.m_disabledColor;
	}

	painter->fillRect(rect, backgroundColor);
	if (!text.m_image.isNull())
	{
		painter->drawImage(rect.topLeft() + text.m_imageLeftTop, text.m_image);
	}
	
	QTextOption textOption;
	textOption.setAlignment(text.m_align);
	painter->setPen(text.m_textColor);
	painter->setFont(QFont(text.m_fontFace, text.m_fontSize));
	painter->drawText(QRect(QPoint(rect.topLeft() + QPoint(text.m_origin, 0)), rect.size()), text.m_text, textOption);
	painter->restore();
}

void TreeViewHeader::init()
{
	if (!check())
	{
		return;
	}

}

bool TreeViewHeader::check()
{
	return true;
}