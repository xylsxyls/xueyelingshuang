#include "HtmlComboBox.h"
#include "ListWidget.h"
#include "HtmlComboBoxStyle.h"
#include "HtmlComboBoxDelegate.h"

HtmlComboBox::HtmlComboBox(QWidget* parent) :
IdItemComboBox(parent)
{
	INIT(L"drop-down");
	ComboBox::setDefault();
	m_normalColor = QColor(0, 0, 0, 0);
	m_selectedColor = QColor(0, 0, 0, 0);
	m_disabledColor = QColor(0, 0, 0, 0);
	m_origin = 0;
	m_leftOrigin = 0;
	m_topOrigin = 0;
	m_rightOrigin = 0;
	m_bottomOrigin = 0;
	m_itemBorderWidth = 0;

	setStyle(new HtmlComboBoxStyle(this));
	m_listWidget->setItemDelegate(new HtmlComboBoxDelegate(this));

	///*((QWidget*)(view()->parent()))->*/setStyleSheet("QToolTip{text-align:alignvcenter;}");
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
	m_topOrigin = GetInt(topOrigin, leftOrigin);
	m_rightOrigin = GetInt(rightOrigin, leftOrigin);
	m_bottomOrigin = GetInt(bottomOrigin, GetInt(topOrigin, leftOrigin));
	ComboBox::setListItemAroundOrigin(leftOrigin, topOrigin, rightOrigin, bottomOrigin, rePaint);
}

void HtmlComboBox::setListItemBorderWidth(int32_t width, bool rePaint)
{
	m_itemBorderWidth = width;
	ComboBox::setListItemBorderWidth(width, rePaint);
}