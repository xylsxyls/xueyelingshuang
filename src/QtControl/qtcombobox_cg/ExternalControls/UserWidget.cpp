#include "UserWidget.h"

UserWidget::UserWidget(Label* hover, QWidget* parent):
Label(parent),
m_hover(hover)
{
	if (m_hover != nullptr)
	{
		m_hover->setParent(this);
	}
}

void UserWidget::enterEvent(QEvent* eve)
{
	Label::enterEvent(eve);
	if (m_hover == nullptr)
	{
		return;
	}
	m_hover->setVisible(true);
}

void UserWidget::leaveEvent(QEvent* eve)
{
	Label::leaveEvent(eve);
	if (m_hover == nullptr)
	{
		return;
	}
	m_hover->setVisible(false);
}
