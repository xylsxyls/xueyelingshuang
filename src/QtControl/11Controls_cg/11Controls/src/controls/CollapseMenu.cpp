#include "CollapseMenu.h"
#include <QPaintEvent>
#include "Label.h"

CollapseMenu::CollapseMenu(QWidget* parent) :
Menu(parent),
m_hasSetSelectedLabelTop(false),
m_hasSetSelectedLabelBottom(false),
m_hasSetTopLabel(false)
{
	INIT(L"item");
	setBorderCollapse();
	m_topLabel = new Label(this);
	m_selectedLabelTop = new Label(this);
	m_selectedLabelBottom = new Label(this);
}

CollapseMenu::CollapseMenu(const QString& title, const QString& icon, const QSize& size, QWidget* parent) :
Menu(title, icon, size, parent),
m_hasSetSelectedLabelTop(false),
m_hasSetSelectedLabelBottom(false),
m_hasSetTopLabel(false)
{
	INIT(L"item");
	setBorderCollapse();
	m_topLabel = new Label(this);
	m_selectedLabelTop = new Label(this);
	m_selectedLabelBottom = new Label(this);
}

CollapseMenu::~CollapseMenu()
{

}

void CollapseMenu::setBorderCollapse()
{
	Menu::setBorderWidth(0);
	ControlBase::setPxValue(L"margin-top", -1, true, false);
}

void CollapseMenu::setItemBorderColor(const QColor& normalColor,
									  const QColor& selectedColor,
									  const QColor& disabledColor,
									  bool rePaint)
{
	m_normalColor = normalColor;
	m_selectedColor = GetQColor(selectedColor, normalColor);
	Menu::setItemBorderColor(normalColor, selectedColor, disabledColor, rePaint);
}

void CollapseMenu::paintEvent(QPaintEvent *eve)
{
	m_topLabel->setGeometry(0, 0, eve->rect().width(), 1);
	if (m_hasSetTopLabel == false)
	{
		m_topLabel->setBackgroundColor(m_normalColor, m_normalColor, m_normalColor, true);
		m_hasSetTopLabel = true;
	}
	
	Menu::paintEvent(eve);
	return;
}

void CollapseMenu::leaveEvent(QEvent* eve)
{
	if (m_selectedLabelTop != nullptr && m_hasSetSelectedLabelTop)
	{
		m_selectedLabelTop->setBackgroundColor(m_normalColor, m_normalColor, m_normalColor, true);
	}
	if (m_selectedLabelBottom != nullptr && m_hasSetSelectedLabelBottom)
	{
		m_selectedLabelBottom->setBackgroundColor(m_normalColor, m_normalColor, m_normalColor, true);
	}
	Menu::leaveEvent(eve);
}

void CollapseMenu::mouseMoveEvent(QMouseEvent *eve)
{
	QAction* pAction = actionAt(mapFromGlobal(eve->globalPos()));
	if (pAction == nullptr)
	{
		return;
	}

	QRect rect = actionGeometry(pAction);
	int32_t top = rect.top();
	int32_t bottom = rect.bottom();
	if (m_selectedLabelTop != nullptr)
	{
		m_selectedLabelTop->setGeometry(rect.left(), (top - 1) < 0 ? 0 : (top - 1), rect.width(), 1);
		m_hasSetSelectedLabelTop = true;
		m_selectedLabelTop->setBackgroundColor(m_selectedColor, m_selectedColor, m_normalColor, true);
	}
	
	if (m_selectedLabelBottom != nullptr)
	{
		m_selectedLabelBottom->setGeometry(rect.left(), bottom, rect.width(), 1);
		m_hasSetSelectedLabelBottom = true;
		m_selectedLabelBottom->setBackgroundColor(m_selectedColor, m_selectedColor, m_normalColor, true);
	}

	Menu::mouseMoveEvent(eve);
	return;
}