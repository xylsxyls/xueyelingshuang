#include "Separator.h"

Separator::Separator(QWidget* parent):
Label(parent),
m_colorLabel(nullptr),
m_isHorizon(true)
{
	ControlBase::setControlShow(this);
    m_colorLabel = new Label(this);
}

Separator::~Separator()
{

}

void Separator::setDisplayMode(bool isHorizon)
{
    m_isHorizon = isHorizon;
}

void Separator::setColor(const QColor& firstColor, const QColor& secondColor)
{
    setBackgroundColor(secondColor);
    repaint();
    if (!check())
    {
        return;
    }
    m_colorLabel->setBackgroundColor(firstColor);
    m_colorLabel->repaint();
}

void Separator::resizeEvent(QResizeEvent* eve)
{
    Label::resizeEvent(eve);
    if (m_isHorizon)
    {
        m_colorLabel->setGeometry(0, 0, width(), height() / 2);
    }
    else
    {
        m_colorLabel->setGeometry(0, 0, width() / 2, height());
    }
}

bool Separator::check()
{
    return m_colorLabel != nullptr;
}
