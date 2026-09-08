#include "ToolButton.h"
#include "ControlSubStyle.h"
#include <map>
#include <QSize>
#include <QVariant>

ToolButton::ToolButton(QWidget* parent) :
ControlShow(parent)
{
	setProperty("class", QVariant(QString("ToolButton")));
	ControlBase::setControlShow(this);
	setDefault();
}

ToolButton::~ToolButton()
{

}

void ToolButton::setDefault()
{
	setBorderWidth(0);
	setAutoRaise(true);
}

void ToolButton::setIconSizeValue(qint32 width, qint32 height)
{
	const qint32 realWidth = qMax(width, 0);
	const qint32 realHeight = qMax(height < 0 ? realWidth : height, 0);
	QToolButton::setIconSize(QSize(realWidth, realHeight));
}

void ToolButton::setMenuIndicatorVisible(bool visible, bool rePaint)
{
	if (visible)
	{
		ControlSubStyle::setPxValue(&m_controlStyle, L"menu-indicator", L"width", 12);
		ControlSubStyle::setPxValue(&m_controlStyle, L"menu-indicator", L"height", 12);
	}
	else
	{
		ControlSubStyle::setPxValue(&m_controlStyle, L"menu-indicator", L"width", 0);
		ControlSubStyle::setPxValue(&m_controlStyle, L"menu-indicator", L"height", 0);
		ControlSubStyle::setKeyValue(&m_controlStyle, L"menu-indicator", L"image", L"none");
	}
	if (rePaint)
	{
		repaint();
	}
}

void ToolButton::setMenuIndicatorSize(qint32 width, qint32 height, bool rePaint)
{
	const qint32 realWidth = qMax(width, 0);
	const qint32 realHeight = qMax(height < 0 ? realWidth : height, 0);
	ControlSubStyle::setPxValue(&m_controlStyle, L"menu-indicator", L"width", realWidth);
	ControlSubStyle::setPxValue(&m_controlStyle, L"menu-indicator", L"height", realHeight);
	if (rePaint)
	{
		repaint();
	}
}

void ToolButton::setMenuIndicatorImage(const QString& imagePath,
									   qint32 stateCount,
									   qint32 normal,
									   qint32 hover,
									   qint32 pressed,
									   qint32 disabled,
									   bool rePaint)
{
	std::map<qint32, std::map<qint32, qint32>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = normal;
	imageStateMap[NORMAL][HOVER] = hover;
	imageStateMap[NORMAL][PRESSED] = pressed;
	imageStateMap[NORMAL][DISABLED] = disabled;
	ControlSubStyle::setImageStateMap(&m_controlStyle, L"menu-indicator", imagePath.toStdWString(), stateCount, imageStateMap, L"image");
	if (rePaint)
	{
		repaint();
	}
}