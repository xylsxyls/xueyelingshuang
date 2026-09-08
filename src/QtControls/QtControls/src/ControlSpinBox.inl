#ifndef CONTROLSPINBOX_INL__
#define CONTROLSPINBOX_INL__

#include "ControlSpinBox.h"
#include "ControlShow.h"
#include "ControlSubStyle.h"

template<class QBase>
ControlSpinBox<QBase>::~ControlSpinBox()
{

}

template<class QBase>
void ControlSpinBox<QBase>::setSpinButtonSize(qint32 width, qint32 height, bool rePaint)
{
	const qint32 realWidth = qMax(width, 0);
	const qint32 realHeight = qMax(height < 0 ? realWidth : height, 0);
	setSpinButtonWidth(realWidth, false);
	setSpinButtonHeight(realHeight, rePaint);
}

template<class QBase>
void ControlSpinBox<QBase>::setSpinButtonWidth(qint32 width, bool rePaint)
{
	ControlShow<QBase>* show = spinControlShow();
	if (show == nullptr)
	{
		return;
	}
	ControlSubStyle::setPxValue(&show->m_controlStyle, L"up-button", L"width", qMax(width, 0));
	ControlSubStyle::setPxValue(&show->m_controlStyle, L"down-button", L"width", qMax(width, 0));
	repaintSpinControl(rePaint);
}

template<class QBase>
void ControlSpinBox<QBase>::setSpinButtonHeight(qint32 height, bool rePaint)
{
	ControlShow<QBase>* show = spinControlShow();
	if (show == nullptr)
	{
		return;
	}
	ControlSubStyle::setPxValue(&show->m_controlStyle, L"up-button", L"height", qMax(height, 0));
	ControlSubStyle::setPxValue(&show->m_controlStyle, L"down-button", L"height", qMax(height, 0));
	repaintSpinControl(rePaint);
}

template<class QBase>
void ControlSpinBox<QBase>::setSpinButtonBorderWidth(qint32 width, bool rePaint)
{
	ControlShow<QBase>* show = spinControlShow();
	if (show == nullptr)
	{
		return;
	}
	const qint32 validWidth = qMax(width, 0);
	ControlSubStyle::setPxSolidValue(&show->m_controlStyle, L"up-button", L"border", validWidth);
	ControlSubStyle::setPxSolidValue(&show->m_controlStyle, L"down-button", L"border", validWidth);
	repaintSpinControl(rePaint);
}

template<class QBase>
void ControlSpinBox<QBase>::setSpinButtonBackgroundColor(const QColor& normalColor,
														 const QColor& hoverColor,
														 const QColor& pressedColor,
														 const QColor& disabledColor,
														 bool rePaint)
{
	ControlShow<QBase>* show = spinControlShow();
	if (show == nullptr)
	{
		return;
	}
	const std::map<qint32, std::map<qint32, QColor>> colorStateMap = buildSpinStateMap(normalColor, hoverColor, pressedColor, disabledColor);
	ControlSubStyle::setColorStateMap(&show->m_controlStyle, L"up-button", colorStateMap, L"background-color");
	ControlSubStyle::setColorStateMap(&show->m_controlStyle, L"down-button", colorStateMap, L"background-color");
	repaintSpinControl(rePaint);
}

template<class QBase>
void ControlSpinBox<QBase>::setSpinButtonBorderColor(const QColor& normalColor,
													 const QColor& hoverColor,
													 const QColor& pressedColor,
													 const QColor& disabledColor,
													 bool rePaint)
{
	ControlShow<QBase>* show = spinControlShow();
	if (show == nullptr)
	{
		return;
	}
	const std::map<qint32, std::map<qint32, QColor>> colorStateMap = buildSpinStateMap(normalColor, hoverColor, pressedColor, disabledColor);
	ControlSubStyle::setColorStateMap(&show->m_controlStyle, L"up-button", colorStateMap, L"border-color");
	ControlSubStyle::setColorStateMap(&show->m_controlStyle, L"down-button", colorStateMap, L"border-color");
	repaintSpinControl(rePaint);
}

template<class QBase>
void ControlSpinBox<QBase>::setSpinButtonBackgroundImage(const QString& imagePath,
														 qint32 stateCount,
														 qint32 normal,
														 qint32 hover,
														 qint32 pressed,
														 qint32 disabled,
														 bool rePaint)
{
	ControlShow<QBase>* show = spinControlShow();
	if (show == nullptr)
	{
		return;
	}
	const std::map<qint32, std::map<qint32, qint32>> imageStateMap = buildSpinStateMap(normal, hover, pressed, disabled);
	ControlSubStyle::setImageStateMap(&show->m_controlStyle, L"up-button", imagePath.toStdWString(), stateCount, imageStateMap, L"border-image");
	ControlSubStyle::setImageStateMap(&show->m_controlStyle, L"down-button", imagePath.toStdWString(), stateCount, imageStateMap, L"border-image");
	repaintSpinControl(rePaint);
}

template<class QBase>
void ControlSpinBox<QBase>::setSpinArrowSize(qint32 width, qint32 height, bool rePaint)
{
	ControlShow<QBase>* show = spinControlShow();
	if (show == nullptr)
	{
		return;
	}
	const qint32 realWidth = qMax(width, 0);
	const qint32 realHeight = qMax(height < 0 ? realWidth : height, 0);
	ControlSubStyle::setPxValue(&show->m_controlStyle, L"up-arrow", L"width", realWidth);
	ControlSubStyle::setPxValue(&show->m_controlStyle, L"up-arrow", L"height", realHeight);
	ControlSubStyle::setPxValue(&show->m_controlStyle, L"down-arrow", L"width", realWidth);
	ControlSubStyle::setPxValue(&show->m_controlStyle, L"down-arrow", L"height", realHeight);
	repaintSpinControl(rePaint);
}

template<class QBase>
void ControlSpinBox<QBase>::setSpinUpArrowImage(const QString& imagePath,
												qint32 stateCount,
												qint32 normal,
												qint32 hover,
												qint32 pressed,
												qint32 disabled,
												bool rePaint)
{
	ControlShow<QBase>* show = spinControlShow();
	if (show == nullptr)
	{
		return;
	}
	const std::map<qint32, std::map<qint32, qint32>> imageStateMap = buildSpinStateMap(normal, hover, pressed, disabled);
	ControlSubStyle::setImageStateMap(&show->m_controlStyle, L"up-arrow", imagePath.toStdWString(), stateCount, imageStateMap, L"image");
	repaintSpinControl(rePaint);
}

template<class QBase>
void ControlSpinBox<QBase>::setSpinDownArrowImage(const QString& imagePath,
												  qint32 stateCount,
												  qint32 normal,
												  qint32 hover,
												  qint32 pressed,
												  qint32 disabled,
												  bool rePaint)
{
	ControlShow<QBase>* show = spinControlShow();
	if (show == nullptr)
	{
		return;
	}
	const std::map<qint32, std::map<qint32, qint32>> imageStateMap = buildSpinStateMap(normal, hover, pressed, disabled);
	ControlSubStyle::setImageStateMap(&show->m_controlStyle, L"down-arrow", imagePath.toStdWString(), stateCount, imageStateMap, L"image");
	repaintSpinControl(rePaint);
}

template<class QBase>
void ControlSpinBox<QBase>::setSpinButtonVisible(bool visible, bool rePaint)
{
	ControlShow<QBase>* show = spinControlShow();
	if (show == nullptr)
	{
		return;
	}
	show->setButtonSymbols(visible ? QAbstractSpinBox::UpDownArrows : QAbstractSpinBox::NoButtons);
	ControlSubStyle::setWidgetKeyValue(&show->m_controlStyle, L"button-symbols", visible ? L"up-down-arrows" : L"no-buttons");
	repaintSpinControl(rePaint);
}

template<class QBase>
ControlShow<QBase>* ControlSpinBox<QBase>::spinControlShow()
{
	return dynamic_cast<ControlShow<QBase>*>(this);
}

template<class QBase>
template<typename TValue>
std::map<qint32, std::map<qint32, TValue>> ControlSpinBox<QBase>::buildSpinStateMap(const TValue& normal,
																					const TValue& hover,
																					const TValue& pressed,
																					const TValue& disabled) const
{
	std::map<qint32, std::map<qint32, TValue>> stateMap;
	stateMap[NORMAL][NORMAL] = normal;
	stateMap[NORMAL][HOVER] = hover;
	stateMap[NORMAL][PRESSED] = pressed;
	stateMap[NORMAL][DISABLED] = disabled;
	return stateMap;
}

template<class QBase>
void ControlSpinBox<QBase>::repaintSpinControl(bool rePaint)
{
	if (rePaint)
	{
		ControlShow<QBase>* show = spinControlShow();
		if (show != nullptr)
		{
			show->repaint();
		}
	}
}

#endif