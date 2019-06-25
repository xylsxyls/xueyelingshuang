#include "ChooseComboBox.h"
#include "ListWidget.h"
#include "Label.h"
#include <QStylePainter>
#include "CGeneralStyle.h"
#include "core/coreAPI.h"
#include <qmath.h>

ChooseComboBox::ChooseComboBox(QWidget* parent):
HtmlComboBox(parent),
m_choose(nullptr),
m_chooseOrigin(0),
m_listItemHeight(0)
{
	m_choose = new Label(nullptr);
	init();
}

ChooseComboBox::~ChooseComboBox()
{
	if (m_choose != nullptr && m_choose->parent() == nullptr)
	{
		delete m_choose;
		m_choose = nullptr;
	}
}

void ChooseComboBox::setComboBoxText(const QString& text)
{
	m_comboBoxText = text;
}

void ChooseComboBox::setChooseImage(const QString& borderImgPath,
									qint32 borderImgStateCount,
									qint32 borderImgNormal,
									qint32 borderImgHover,
									qint32 borderImgDisabled,
									bool rePaint)
{
	if (!check())
	{
		return;
	}
	m_choose->setBorderImage(borderImgPath, borderImgStateCount, borderImgNormal, borderImgHover, borderImgDisabled, rePaint);
}

void ChooseComboBox::setChooseOrigin(qint32 origin)
{
	m_chooseOrigin = origin;
}

void ChooseComboBox::setChooseByIndex(qint32 index)
{
	if (!check())
	{
		return;
	}
	QListWidgetItem* widgetItem = m_listWidget->item(index);
	if (widgetItem == nullptr)
	{
		m_choose->setParent(nullptr);
		return;
	}
	QWidget* widget = new QWidget;
	if (widget == nullptr)
	{
		return;
	}
	m_choose->setParent(widget);
	m_listWidget->setItemWidget(widgetItem, widget);
	updateChooseGeometry();
}

void ChooseComboBox::setChooseById(qint64 id)
{
	setChooseByIndex(itemIndexByFirstId(id));
}

void ChooseComboBox::setChooseSize(qint32 width, qint32 height)
{
	if (!check())
	{
		return;
	}
	m_choose->resize(width, height);
}

void ChooseComboBox::updateChooseGeometry()
{
	if (!check())
	{
		return;
	}
	QWidget* widget = (QWidget*)m_choose->parent();
	if (widget == nullptr)
	{
		return;
	}
	qint32 height = m_choose->height();
	qint32 chooseOriginHeight = qMax((m_listItemHeight - height) / 2, 0);
	m_choose->setGeometry(m_chooseOrigin, chooseOriginHeight, m_choose->width(), height);
}

void ChooseComboBox::setListItemHeight(qint32 height, bool rePaint)
{
	HtmlComboBox::setListItemHeight(height, rePaint);
	m_listItemHeight = height;
}

void ChooseComboBox::clear()
{
	if (!check())
	{
		return;
	}
	m_choose->setParent(nullptr);
	HtmlComboBox::clear();
}

qint32 ChooseComboBox::currentChoosedIndex()
{
	if (!check())
	{
		return -1;
	}
	QWidget* widget = (QWidget*)m_choose->parent();
	if (widget == nullptr)
	{
		return -1;
	}
	qint32 index = -1;
	while (index++ != m_listWidget->count() - 1)
	{
		if (m_listWidget->itemWidget(m_listWidget->item(index)) == widget)
		{
			return index;
		}
	}
	return 0;
}

qint64 ChooseComboBox::currentChoosedId()
{
	return itemId(currentChoosedIndex());
}

QString ChooseComboBox::currentChoosedText()
{
	if (!check())
	{
		return "";
	}
	QListWidgetItem* item = m_listWidget->item(currentChoosedIndex());
	if (item == nullptr)
	{
		return "";
	}
	return item->text();
}

void ChooseComboBox::clearChoose()
{
	setChooseByIndex(-1);
}

void ChooseComboBox::resizeEvent(QResizeEvent* eve)
{
	HtmlComboBox::resizeEvent(eve);
	updateChooseGeometry();
}

void ChooseComboBox::mouseMoveEvent(QMouseEvent* eve)
{
	HtmlComboBox::mouseMoveEvent(eve);
	setToolTip(currentChoosedText());
}

void ChooseComboBox::paintEvent(QPaintEvent* eve)
{
	QStylePainter painter(this);
	// draw the combobox frame, focusrect and selected etc.
	QStyleOptionComboBox opt;
	initStyleOption(&opt);
	opt.currentText = m_comboBoxText;
	painter.drawComplexControl(QStyle::CC_ComboBox, opt);
	
	// draw the icon and text
	// draw the icon and text
	painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
}

bool ChooseComboBox::check()
{
	return m_choose != nullptr && HtmlComboBox::check();
}

void ChooseComboBox::init()
{
	setSelectEnable(false);
	QObject::connect(this, &ComboBox::itemPressed, this, &ChooseComboBox::onItemPressed);
}

void ChooseComboBox::onItemPressed(qint32 index)
{
	setChooseByIndex(index);
	emit itemChoosed(index, itemId(index), itemText(index));
}