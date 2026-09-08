#include "ScrollBar.h"
#include "ControlSubStyle.h"

#include <map>
#include <QVariant>

ScrollBar::ScrollBar(QWidget* parent) :
ControlShow(parent)
{
	init();
}

ScrollBar::ScrollBar(Qt::Orientation orientation, QWidget* parent) :
ControlShow(parent)
{
	setOrientation(orientation);
	init();
}

ScrollBar::~ScrollBar()
{

}

void ScrollBar::setDefault()
{
	setBorderWidth(0);
	setItemName(L"handle");
	setHandleMinSize(12);
	setLineVisible(false);
}

void ScrollBar::setHandleMinSize(qint32 size, bool rePaint)
{
	const qint32 validSize = qMax(size, 0);
	ControlSubStyle::setPxValue(&m_controlStyle, L"handle", L"min-width", validSize);
	ControlSubStyle::setPxValue(&m_controlStyle, L"handle", L"min-height", validSize);
	if (rePaint)
	{
		repaint();
	}
}

void ScrollBar::setLineSize(qint32 size, bool rePaint)
{
	const qint32 validSize = qMax(size, 0);
	ControlSubStyle::setPxValue(&m_controlStyle, L"add-line", L"width", validSize);
	ControlSubStyle::setPxValue(&m_controlStyle, L"add-line", L"height", validSize);
	ControlSubStyle::setPxValue(&m_controlStyle, L"sub-line", L"width", validSize);
	ControlSubStyle::setPxValue(&m_controlStyle, L"sub-line", L"height", validSize);
	if (rePaint)
	{
		repaint();
	}
}

void ScrollBar::setLineVisible(bool visible, bool rePaint)
{
	const qint32 size = visible ? 12 : 0;
	setLineSize(size, false);
	if (visible == false)
	{
		ControlSubStyle::setKeyValue(&m_controlStyle, L"add-line", L"border-image", L"none");
		ControlSubStyle::setKeyValue(&m_controlStyle, L"sub-line", L"border-image", L"none");
	}
	if (rePaint)
	{
		repaint();
	}
}

void ScrollBar::setSubPageBackgroundColor(const QColor& color, bool rePaint)
{
	ControlSubStyle::setKeyValue(&m_controlStyle, L"sub-page", L"background-color", QssHelper::QColorToWString(color));
	if (rePaint)
	{
		repaint();
	}
}

void ScrollBar::setAddPageBackgroundColor(const QColor& color, bool rePaint)
{
	ControlSubStyle::setKeyValue(&m_controlStyle, L"add-page", L"background-color", QssHelper::QColorToWString(color));
	if (rePaint)
	{
		repaint();
	}
}

void ScrollBar::setLineBackgroundColor(const QColor& normalColor,
									   const QColor& hoverColor,
									   const QColor& pressedColor,
									   const QColor& disabledColor,
									   bool rePaint)
{
	std::map<qint32, std::map<qint32, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][HOVER] = hoverColor;
	colorStateMap[NORMAL][PRESSED] = pressedColor;
	colorStateMap[NORMAL][DISABLED] = disabledColor;
	ControlSubStyle::setColorStateMap(&m_controlStyle, L"add-line", colorStateMap, L"background-color");
	ControlSubStyle::setColorStateMap(&m_controlStyle, L"sub-line", colorStateMap, L"background-color");
	if (rePaint)
	{
		repaint();
	}
}

void ScrollBar::setArrowSize(qint32 width, qint32 height, bool rePaint)
{
	const qint32 realWidth = qMax(width, 0);
	const qint32 realHeight = qMax(height < 0 ? realWidth : height, 0);
	ControlSubStyle::setPxValue(&m_controlStyle, L"up-arrow", L"width", realWidth);
	ControlSubStyle::setPxValue(&m_controlStyle, L"up-arrow", L"height", realHeight);
	ControlSubStyle::setPxValue(&m_controlStyle, L"down-arrow", L"width", realWidth);
	ControlSubStyle::setPxValue(&m_controlStyle, L"down-arrow", L"height", realHeight);
	ControlSubStyle::setPxValue(&m_controlStyle, L"left-arrow", L"width", realWidth);
	ControlSubStyle::setPxValue(&m_controlStyle, L"left-arrow", L"height", realHeight);
	ControlSubStyle::setPxValue(&m_controlStyle, L"right-arrow", L"width", realWidth);
	ControlSubStyle::setPxValue(&m_controlStyle, L"right-arrow", L"height", realHeight);
	if (rePaint)
	{
		repaint();
	}
}

void ScrollBar::setUpArrowImage(const QString& imagePath, qint32 stateCount, qint32 normal, qint32 hover, qint32 pressed, qint32 disabled, bool rePaint)
{
	setArrowImage(L"up-arrow", imagePath, stateCount, normal, hover, pressed, disabled, rePaint);
}

void ScrollBar::setDownArrowImage(const QString& imagePath, qint32 stateCount, qint32 normal, qint32 hover, qint32 pressed, qint32 disabled, bool rePaint)
{
	setArrowImage(L"down-arrow", imagePath, stateCount, normal, hover, pressed, disabled, rePaint);
}

void ScrollBar::setLeftArrowImage(const QString& imagePath, qint32 stateCount, qint32 normal, qint32 hover, qint32 pressed, qint32 disabled, bool rePaint)
{
	setArrowImage(L"left-arrow", imagePath, stateCount, normal, hover, pressed, disabled, rePaint);
}

void ScrollBar::setRightArrowImage(const QString& imagePath, qint32 stateCount, qint32 normal, qint32 hover, qint32 pressed, qint32 disabled, bool rePaint)
{
	setArrowImage(L"right-arrow", imagePath, stateCount, normal, hover, pressed, disabled, rePaint);
}

void ScrollBar::init()
{
	setProperty("class", QVariant(QString("ScrollBar")));
	ControlBase::setControlShow(this);
	setMouseTracking(true);
	setDefault();
}

void ScrollBar::setArrowImage(const std::wstring& subControl,
							  const QString& imagePath,
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
	ControlSubStyle::setImageStateMap(&m_controlStyle, subControl, imagePath.toStdWString(), stateCount, imageStateMap, L"image");
	if (rePaint)
	{
		repaint();
	}
}