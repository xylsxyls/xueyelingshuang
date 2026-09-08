#include "TreeViewDelegate.h"
#include <QPainter>
#include "TreeView.h"
#include <QTextOption>
#include <vector>

TreeViewDelegate::TreeViewDelegate()
{
	init();
}

TreeViewDelegate::~TreeViewDelegate()
{
	
}

void TreeViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if (painter == nullptr || !index.isValid())
	{
		return;
	}
	TreeView* view = qobject_cast<TreeView*>(parent());
	if (view == nullptr || view->model() == nullptr || view->model()->bodyText() == nullptr)
	{
		QStyledItemDelegate::paint(painter, option, index);
		return;
	}
	int32_t row = index.row();
	int32_t column = index.column();
	auto bodyText = view->model()->bodyText();
	auto itRowText = bodyText->find(row);
	if (itRowText == bodyText->end())
	{
		QStyledItemDelegate::paint(painter, option, index);
		return;
	}
	auto itColumnText = itRowText->second.find(column);
	if (itColumnText == itRowText->second.end())
	{
		QStyledItemDelegate::paint(painter, option, index);
		return;
	}
	const TreeText& text = itColumnText->second;

	painter->save();
	QColor normalColor = QColor(0, 0, 0, 0);
	QColor hoverColor = QColor(0, 0, 0, 0);
	QColor selectedColor = QColor(0, 0, 0, 0);
	QColor disabledColor = QColor(0, 0, 0, 0);
	// readTreeStateColors入参：colors是行背景颜色的四态数组。
	// readTreeStateColors出参：normalColor、hoverColor、selectedColor、disabledColor会在数组完整时被更新。
	// readTreeStateColors返回值：返回true表示颜色数组完整并已读取。
	auto readTreeStateColors = [&normalColor, &hoverColor, &selectedColor, &disabledColor](const std::vector<QColor>& colors) -> bool
	{
		if (colors.size() < 4)
		{
			return false;
		}
		normalColor = colors[0];
		hoverColor = colors[1];
		selectedColor = colors[2];
		disabledColor = colors[3];
		return true;
	};
	std::map<int32_t, std::vector<QColor>>* itemBackgroundColor = view->model()->itemBackgroundColor();
	auto itDefaultColor = itemBackgroundColor->find(-1);
	if (itDefaultColor != itemBackgroundColor->end())
	{
		readTreeStateColors(itDefaultColor->second);
	}
	auto itColor = itemBackgroundColor->find(row);
	if (itColor != itemBackgroundColor->end())
	{
		readTreeStateColors(itColor->second);
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
	if (view == nullptr || view->model() == nullptr || !index.isValid())
	{
		return QStyledItemDelegate::createEditor(parentPtr, option, index);
	}
	int32_t row = index.row();
	int32_t column = index.column();
	auto bodyText = view->model()->bodyText();
	auto itRowText = bodyText->find(row);
	if (itRowText == bodyText->end())
	{
		return QStyledItemDelegate::createEditor(parentPtr, option, index);
	}
	auto itColumnText = itRowText->second.find(column);
	if (itColumnText == itRowText->second.end())
	{
		return QStyledItemDelegate::createEditor(parentPtr, option, index);
	}
	TreeText& text = itColumnText->second;
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