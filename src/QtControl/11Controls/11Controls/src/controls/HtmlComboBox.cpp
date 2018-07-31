#include "HtmlComboBox.h"
#include "ListWidget.h"
#include "HtmlComboBoxStyle.h"
#include "HtmlComboBoxDelegate.h"

HtmlComboBox::HtmlComboBox(QWidget* parent) :
IdItemComboBox(parent),
m_normalColor(QColor(0, 0, 0, 0)),
m_selectedColor(QColor(0, 0, 0, 0)),
m_disabledColor(QColor(0, 0, 0, 0)),
m_origin(0),
m_leftOrigin(0),
m_topOrigin(0),
m_rightOrigin(0),
m_bottomOrigin(0),
m_itemBorderWidth(0)
{
	init();
}

HtmlComboBox::~HtmlComboBox()
{

}

void HtmlComboBox::setListTextColor(const QColor& normalColor,
									const QColor& hoverColor,
									const QColor& disabledColor,
									bool rePaint)
{
	m_normalColor = normalColor;
	m_selectedColor = GetQColor(hoverColor, normalColor);
	m_disabledColor = GetQColor(disabledColor, normalColor);
	ComboBox::setListTextColor(normalColor, hoverColor, disabledColor, rePaint);
}

void HtmlComboBox::setListTextOrigin(qint32 origin, bool rePaint)
{
	m_origin = origin;
	ComboBox::setListTextOrigin(origin, rePaint);
}

void HtmlComboBox::setListItemAroundOrigin(qint32 leftOrigin,
										   qint32 topOrigin,
										   qint32 rightOrigin,
										   qint32 bottomOrigin,
										   bool rePaint)
{
	m_leftOrigin = leftOrigin;
	m_topOrigin = GetInt(topOrigin, leftOrigin);
	m_rightOrigin = GetInt(rightOrigin, leftOrigin);
	m_bottomOrigin = GetInt(bottomOrigin, GetInt(topOrigin, leftOrigin));
	ComboBox::setListItemAroundOrigin(leftOrigin, topOrigin, rightOrigin, bottomOrigin, rePaint);
}

void HtmlComboBox::setListItemBorderWidth(qint32 width, bool rePaint)
{
	m_itemBorderWidth = width;
	ComboBox::setListItemBorderWidth(width, rePaint);
}

void HtmlComboBox::init()
{
	HtmlComboBoxStyle* style = new HtmlComboBoxStyle(this);
	if (style != nullptr)
	{
		setStyle(style);
	}
	
	if (!check())
	{
		return;
	}
	HtmlComboBoxDelegate* comboBoxDelegate = new HtmlComboBoxDelegate(this);
	if (comboBoxDelegate != nullptr)
	{
		m_listWidget->setItemDelegate(comboBoxDelegate);
	}
}