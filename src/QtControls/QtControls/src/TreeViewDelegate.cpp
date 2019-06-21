#include "TreeViewDelegate.h"
#include <QPainter>
#include "TreeView.h"
#include <QTextOption>

TreeViewDelegate::TreeViewDelegate()
{
	init();
}

TreeViewDelegate::~TreeViewDelegate()
{
	
}

void TreeViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	painter->save();
	TreeView* view = qobject_cast<TreeView*>(parent());
	int32_t row = index.row();
	int32_t column = index.column();
	TreeText& text = (*(view->model()->bodyText()))[row][column];
	
	QColor normalColor = QColor(0, 0, 0, 0);
	QColor hoverColor = QColor(0, 0, 0, 0);
	QColor selectedColor = QColor(0, 0, 0, 0);
	QColor disabledColor = QColor(0, 0, 0, 0);
	std::map<int32_t, std::vector<QColor>>* itemBackgroundColor = view->model()->itemBackgroundColor();
	auto itDefaultColor = itemBackgroundColor->find(-1);
	if (itDefaultColor != itemBackgroundColor->end())
	{
		normalColor = itDefaultColor->second[0];
		hoverColor = itDefaultColor->second[1];
		selectedColor = itDefaultColor->second[2];
		disabledColor = itDefaultColor->second[3];
	}
	auto itColor = itemBackgroundColor->find(row);
	if (itColor != itemBackgroundColor->end())
	{
		normalColor = itColor->second[0];
		hoverColor = itColor->second[1];
		selectedColor = itColor->second[2];
		disabledColor = itColor->second[3];
	}
	if (text.m_backgroundColor != QColor(0, 0, 0, 0))
	{
		normalColor = text.m_backgroundColor;
	}
	if (option.state.testFlag(QStyle::State_MouseOver))
	{
		painter->fillRect(option.rect, hoverColor);
	}
	else if (option.state.testFlag(QStyle::State_Selected))
	{
		painter->fillRect(option.rect, selectedColor);
	}
	else if (!option.state.testFlag(QStyle::State_Enabled))
	{
		painter->fillRect(option.rect, disabledColor);
	}
	else
	{
		painter->fillRect(option.rect, normalColor);
	}
	
	if (!text.m_image.isNull())
	{
		painter->drawImage(option.rect.topLeft() + text.m_imageLeftTop, text.m_image);
	}
	QTextOption textOption;
	textOption.setAlignment(text.m_align);
	painter->setPen(text.m_textColor);
	painter->setFont(QFont(text.m_fontFace, text.m_fontSize));
	painter->drawText(QRect(QPoint(option.rect.topLeft() + QPoint(text.m_origin, 0)), option.rect.size()), text.m_text, textOption);
	painter->restore();
}

QWidget* TreeViewDelegate::createEditor(QWidget* parentPtr, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	TreeView* view = qobject_cast<TreeView*>(parent());
	int32_t row = index.row();
	int32_t column = index.column();
	TreeText& text = (*(view->model()->bodyText()))[row][column];
	if (text.m_widget == nullptr)
	{
		return QStyledItemDelegate::createEditor(parentPtr, option, index);
	}
	text.m_widget->setParent(parentPtr);
	return text.m_widget;
}

void TreeViewDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	return QStyledItemDelegate::setEditorData(editor, index);
}

void TreeViewDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index)
{
	return QStyledItemDelegate::setModelData(editor, model, index);
}

void TreeViewDelegate::init()
{
	if (!check())
	{
		return;
	}

}

bool TreeViewDelegate::check()
{
	return true;
}