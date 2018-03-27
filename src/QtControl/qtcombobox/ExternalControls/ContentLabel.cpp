#include "ContentLabel.h"

ContentLabel::ContentLabel(QWidget* parent) :
m_up(nullptr),
m_down(nullptr),
m_isOne(true),
m_rect(QRect(0, 0, 0, 0))
{
	m_up = new Label(parent);
	m_up->setAlignment(Qt::AlignCenter);
	m_up->setBorderWidth(0);
	m_down = new Label(parent);
	m_down->setAlignment(Qt::AlignCenter);
	m_down->setBorderWidth(0);
}

void ContentLabel::setUpColor(const QColor& color)
{
	if (!check())
	{
		return;
	}
	m_up->setTextColor(color);
}

void ContentLabel::setDownColor(const QColor& color)
{
	if (!check())
	{
		return;
	}
	m_down->setTextColor(color);
}

void ContentLabel::setOneLine()
{
	m_isOne = true;
	update();
}

void ContentLabel::setTwoLine()
{
	m_isOne = false;
	update();
}

void ContentLabel::setUpText(const QString& text)
{
	if (!check())
	{
		return;
	}
	m_up->setText(text);
}

void ContentLabel::setDownText(const QString& text)
{
	if (!check())
	{
		return;
	}
	m_down->setText(text);
}

void ContentLabel::setGeometry(const QRect& rect)
{
	m_rect = rect;
	update();
}

void ContentLabel::setBackgroundColor(const QColor& color)
{
	if (!check())
	{
		return;
	}
	m_up->setBackgroundColor(color);
	m_down->setBackgroundColor(color);
}

void ContentLabel::setFontFace(const QString& fontFace)
{
	if (!check())
	{
		return;
	}
	m_up->setFontFace(fontFace);
	m_down->setFontFace(fontFace);
}

void ContentLabel::setFontSize(int32_t size)
{
	if (!check())
	{
		return;
	}
	m_up->setFontSize(size);
	m_down->setFontSize(size);
}

void ContentLabel::update()
{
	if (!check())
	{
		return;
	}

	if (m_isOne)
	{
		m_up->setGeometry(m_rect);
		m_down->setVisible(false);
	}
	else
	{
		m_down->setVisible(true);
		m_up->setGeometry(QRect(m_rect.left(), m_rect.top(), m_rect.width(), m_rect.height() / 2));
		m_down->setGeometry(QRect(m_rect.left(), m_rect.top() + m_rect.height() / 2, m_rect.width(), m_rect.height() / 2));
	}
}

bool ContentLabel::check()
{
	return m_up != nullptr && m_down != nullptr;
}
