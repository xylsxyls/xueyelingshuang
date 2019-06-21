#ifndef CONTROLBACKGROUND_INL__
#define CONTROLBACKGROUND_INL__
//#pragma once
#include "ControlBackground.h"

template<class QBase>
ControlBackgroundForNormal<QBase>::~ControlBackgroundForNormal()
{

}

template<class QBase>
void ControlBackgroundForNormal<QBase>::setBackgroundColor(const QColor& color, bool rePaint)
{
	std::map<qint32, std::map<qint32, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = color;
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"background-color", false, rePaint);
}

template<class QBase>
void ControlBackgroundForNormal<QBase>::setBackgroundImage(const QString& backgroundImgPath, bool rePaint)
{
	std::map<qint32, std::map<qint32, qint32>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = 1;
	std::wstring wstrImgPath = backgroundImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, 1, L"background-image", false, rePaint);
}

template<class QBase>
ControlBackgroundForNormalHoverDisabled<QBase>::~ControlBackgroundForNormalHoverDisabled()
{

}

template<class QBase>
void ControlBackgroundForNormalHoverDisabled<QBase>::setBackgroundColor(const QColor& normalColor,
																		const QColor& hoverColor,
																		const QColor& disabledColor,
																		bool rePaint)
{
	std::map<qint32, std::map<qint32, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][HOVER] = GetQColor(hoverColor, normalColor);
	colorStateMap[NORMAL][DISABLED] = GetQColor(disabledColor, normalColor);
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"background-color", false, rePaint);
}

template<class QBase>
void ControlBackgroundForNormalHoverDisabled<QBase>::setBackgroundImage(const QString& backgroundImgPath,
																		qint32 backgroundImgStateCount,
																		qint32 backgroundImgNormal,
																		qint32 backgroundImgHover,
																		qint32 backgroundImgDisabled,
																		bool rePaint)
{
	std::map<qint32, std::map<qint32, qint32>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = backgroundImgNormal;
	imageStateMap[NORMAL][HOVER] = backgroundImgHover;
	imageStateMap[NORMAL][DISABLED] = backgroundImgDisabled;
	std::wstring wstrImgPath = backgroundImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, backgroundImgStateCount, L"background-image", false, rePaint);
}

template<class QBase>
ControlBackgroundForNormalSelectedDisabled<QBase>::~ControlBackgroundForNormalSelectedDisabled()
{

}

template<class QBase>
void ControlBackgroundForNormalSelectedDisabled<QBase>::setBackgroundColor(const QColor& normalColor,
																		   const QColor& selectedColor,
																		   const QColor& disabledColor,
																		   bool rePaint)
{
	std::map<qint32, std::map<qint32, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][SELECTED] = GetQColor(selectedColor, normalColor);
	colorStateMap[NORMAL][DISABLED] = GetQColor(disabledColor, normalColor);
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"background-color", false, rePaint);
}

template<class QBase>
void ControlBackgroundForNormalSelectedDisabled<QBase>::setBackgroundImage(const QString& backgroundImgPath,
																		   qint32 backgroundImgStateCount,
																		   qint32 backgroundImgNormal,
																		   qint32 backgroundImgSelected,
																		   qint32 backgroundImgDisabled,
																		   bool rePaint)
{
	std::map<qint32, std::map<qint32, qint32>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = backgroundImgNormal;
	imageStateMap[NORMAL][SELECTED] = backgroundImgSelected;
	imageStateMap[NORMAL][DISABLED] = backgroundImgDisabled;
	std::wstring wstrImgPath = backgroundImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, backgroundImgStateCount, L"background-image", false, rePaint);
}

template<class QBase>
ControlBackgroundForNormalHoverPressedDisabled<QBase>::~ControlBackgroundForNormalHoverPressedDisabled()
{

}

template<class QBase>
void ControlBackgroundForNormalHoverPressedDisabled<QBase>::setBackgroundColor(const QColor& normalColor,
																			   const QColor& hoverColor,
																			   const QColor& pressedColor,
																			   const QColor& disabledColor,
																			   bool rePaint)
{
	std::map<qint32, std::map<qint32, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][HOVER] = GetQColor(hoverColor, normalColor);
	colorStateMap[NORMAL][PRESSED] = GetQColor(pressedColor, normalColor);
	colorStateMap[NORMAL][DISABLED] = GetQColor(disabledColor, normalColor);
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"background-color", false, rePaint);
}

template<class QBase>
void ControlBackgroundForNormalHoverPressedDisabled<QBase>::setBackgroundImage(const QString& backgroundImgPath,
																			   qint32 backgroundImgStateCount,
																			   qint32 backgroundImgNormal,
																			   qint32 backgroundImgHover,
																			   qint32 backgroundImgPressed,
																			   qint32 backgroundImgDisabled,
																			   bool rePaint)
{
	std::map<qint32, std::map<qint32, qint32>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = backgroundImgNormal;
	imageStateMap[NORMAL][HOVER] = backgroundImgHover;
	imageStateMap[NORMAL][PRESSED] = backgroundImgPressed;
	imageStateMap[NORMAL][DISABLED] = backgroundImgDisabled;
	std::wstring wstrImgPath = backgroundImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, backgroundImgStateCount, L"background-image", false, rePaint);
}

template<class QBase>
ControlBackgroundForNormalHoverPressedDisabledAndCheck<QBase>::~ControlBackgroundForNormalHoverPressedDisabledAndCheck()
{

}

template<class QBase>
void ControlBackgroundForNormalHoverPressedDisabledAndCheck<QBase>::setBackgroundColor(const QColor& normalColor,
																					   const QColor& hoverColor,
																					   const QColor& pressedColor,
																					   const QColor& disabledColor,
																					   const QColor& normalCkColor,
																					   const QColor& hoverCkColor,
																					   const QColor& pressedCkColor,
																					   const QColor& disabledCkColor,
																					   bool rePaint)
{
	std::map<qint32, std::map<qint32, QColor>> colorStateMap;
	colorStateMap[UNCHECK][NORMAL] = normalColor;
	colorStateMap[UNCHECK][HOVER] = GetQColor(hoverColor, normalColor);
	colorStateMap[UNCHECK][PRESSED] = GetQColor(pressedColor, normalColor);
	colorStateMap[UNCHECK][DISABLED] = GetQColor(disabledColor, normalColor);
	colorStateMap[CHECK][NORMAL] = GetQColor(normalCkColor, normalColor);
	colorStateMap[CHECK][HOVER] = GetQColor(hoverCkColor, GetQColor(normalCkColor, normalColor));
	colorStateMap[CHECK][PRESSED] = GetQColor(pressedCkColor, GetQColor(normalCkColor, normalColor));
	colorStateMap[CHECK][DISABLED] = GetQColor(disabledCkColor, GetQColor(normalCkColor, normalColor));
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"background-color", false, rePaint);
}

template<class QBase>
void ControlBackgroundForNormalHoverPressedDisabledAndCheck<QBase>::setBackgroundImage(const QString& backgroundImgPath,
																					   qint32 backgroundImgStateCount,
																					   qint32 backgroundImgNormal,
																					   qint32 backgroundImgHover,
																					   qint32 backgroundImgPressed,
																					   qint32 backgroundImgDisabled,
																					   qint32 backgroundImgCkNormal,
																					   qint32 backgroundImgCkHover,
																					   qint32 backgroundImgCkPressed,
																					   qint32 backgroundImgCkDisabled,
																					   bool rePaint)
{
	std::map<qint32, std::map<qint32, qint32>> imageStateMap;
	imageStateMap[UNCHECK][NORMAL] = backgroundImgNormal;
	imageStateMap[UNCHECK][HOVER] = backgroundImgHover;
	imageStateMap[UNCHECK][PRESSED] = backgroundImgPressed;
	imageStateMap[UNCHECK][DISABLED] = backgroundImgDisabled;
	imageStateMap[CHECK][NORMAL] = backgroundImgCkNormal;
	imageStateMap[CHECK][HOVER] = backgroundImgCkHover;
	imageStateMap[CHECK][PRESSED] = backgroundImgCkPressed;
	imageStateMap[CHECK][DISABLED] = backgroundImgCkDisabled;
	std::wstring wstrImgPath = backgroundImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, backgroundImgStateCount, L"background-image", false, rePaint);
}

template<class QBase>
ControlItemBackgroundForNormal<QBase>::~ControlItemBackgroundForNormal()
{

}

template<class QBase>
void ControlItemBackgroundForNormal<QBase>::setItemBackgroundColor(const QColor& color, bool rePaint)
{
	std::map<qint32, std::map<qint32, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = color;
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"background-color", true, rePaint);
}

template<class QBase>
void ControlItemBackgroundForNormal<QBase>::setItemBackgroundImage(const QString& backgroundImgPath, bool rePaint)
{
	std::map<qint32, std::map<qint32, qint32>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = 1;
	std::wstring wstrImgPath = backgroundImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, 1, L"background-image", true, rePaint);
}

template<class QBase>
ControlItemBackgroundForNormalHoverDisabled<QBase>::~ControlItemBackgroundForNormalHoverDisabled()
{

}

template<class QBase>
void ControlItemBackgroundForNormalHoverDisabled<QBase>::setItemBackgroundColor(const QColor& normalColor,
																				const QColor& hoverColor,
																				const QColor& disabledColor,
																				bool rePaint)
{
	std::map<qint32, std::map<qint32, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][HOVER] = GetQColor(hoverColor, normalColor);
	colorStateMap[NORMAL][DISABLED] = GetQColor(disabledColor, normalColor);
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"background-color", true, rePaint);
}

template<class QBase>
void ControlItemBackgroundForNormalHoverDisabled<QBase>::setItemBackgroundImage(const QString& backgroundImgPath,
																				qint32 backgroundImgStateCount,
																				qint32 backgroundImgNormal,
																				qint32 backgroundImgHover,
																				qint32 backgroundImgDisabled,
																				bool rePaint)
{
	std::map<qint32, std::map<qint32, qint32>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = backgroundImgNormal;
	imageStateMap[NORMAL][HOVER] = backgroundImgHover;
	imageStateMap[NORMAL][DISABLED] = backgroundImgDisabled;
	std::wstring wstrImgPath = backgroundImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, backgroundImgStateCount, L"background-image", true, rePaint);
}

template<class QBase>
ControlItemBackgroundForNormalSelectedDisabled<QBase>::~ControlItemBackgroundForNormalSelectedDisabled()
{

}

template<class QBase>
void ControlItemBackgroundForNormalSelectedDisabled<QBase>::setItemBackgroundColor(const QColor& normalColor,
																				   const QColor& selectedColor,
																				   const QColor& disabledColor,
																				   bool rePaint)
{
	std::map<qint32, std::map<qint32, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][SELECTED] = GetQColor(selectedColor, normalColor);
	colorStateMap[NORMAL][DISABLED] = GetQColor(disabledColor, normalColor);
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"background-color", true, rePaint);
}

template<class QBase>
void ControlItemBackgroundForNormalSelectedDisabled<QBase>::setItemBackgroundImage(const QString& backgroundImgPath,
																				   qint32 backgroundImgStateCount,
																				   qint32 backgroundImgNormal,
																				   qint32 backgroundImgSelected,
																				   qint32 backgroundImgDisabled,
																				   bool rePaint)
{
	std::map<qint32, std::map<qint32, qint32>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = backgroundImgNormal;
	imageStateMap[NORMAL][SELECTED] = backgroundImgSelected;
	imageStateMap[NORMAL][DISABLED] = backgroundImgDisabled;
	std::wstring wstrImgPath = backgroundImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, backgroundImgStateCount, L"background-image", true, rePaint);
}

template<class QBase>
ControlItemBackgroundForNormalHoverPressedDisabled<QBase>::~ControlItemBackgroundForNormalHoverPressedDisabled()
{

}

template<class QBase>
void ControlItemBackgroundForNormalHoverPressedDisabled<QBase>::setItemBackgroundColor(const QColor& normalColor,
																					   const QColor& hoverColor,
																					   const QColor& pressedColor,
																					   const QColor& disabledColor,
																					   bool rePaint)
{
	std::map<qint32, std::map<qint32, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][HOVER] = GetQColor(hoverColor, normalColor);
	colorStateMap[NORMAL][PRESSED] = GetQColor(pressedColor, normalColor);
	colorStateMap[NORMAL][DISABLED] = GetQColor(disabledColor, normalColor);
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"background-color", true, rePaint);
}

template<class QBase>
void ControlItemBackgroundForNormalHoverPressedDisabled<QBase>::setItemBackgroundImage(const QString& backgroundImgPath,
																					   qint32 backgroundImgStateCount,
																					   qint32 backgroundImgNormal,
																					   qint32 backgroundImgHover,
																					   qint32 backgroundImgPressed,
																					   qint32 backgroundImgDisabled,
																					   bool rePaint)
{
	std::map<qint32, std::map<qint32, qint32>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = backgroundImgNormal;
	imageStateMap[NORMAL][HOVER] = backgroundImgHover;
	imageStateMap[NORMAL][PRESSED] = backgroundImgPressed;
	imageStateMap[NORMAL][DISABLED] = backgroundImgDisabled;
	std::wstring wstrImgPath = backgroundImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, backgroundImgStateCount, L"background-image", true, rePaint);
}

template<class QBase>
ControlItemBackgroundForNormalHoverPressedDisabledAndCheck<QBase>::~ControlItemBackgroundForNormalHoverPressedDisabledAndCheck()
{

}

template<class QBase>
void ControlItemBackgroundForNormalHoverPressedDisabledAndCheck<QBase>::setItemBackgroundColor(const QColor& normalColor,
																							   const QColor& hoverColor,
																							   const QColor& pressedColor,
																							   const QColor& disabledColor,
																							   const QColor& normalCkColor,
																							   const QColor& hoverCkColor,
																							   const QColor& pressedCkColor,
																							   const QColor& disabledCkColor,
																							   bool rePaint)
{
	std::map<qint32, std::map<qint32, QColor>> colorStateMap;
	colorStateMap[UNCHECK][NORMAL] = normalColor;
	colorStateMap[UNCHECK][HOVER] = GetQColor(hoverColor, normalColor);
	colorStateMap[UNCHECK][PRESSED] = GetQColor(pressedColor, normalColor);
	colorStateMap[UNCHECK][DISABLED] = GetQColor(disabledColor, normalColor);
	colorStateMap[CHECK][NORMAL] = GetQColor(normalCkColor, normalColor);
	colorStateMap[CHECK][HOVER] = GetQColor(hoverCkColor, GetQColor(normalCkColor, normalColor));
	colorStateMap[CHECK][PRESSED] = GetQColor(pressedCkColor, GetQColor(normalCkColor, normalColor));
	colorStateMap[CHECK][DISABLED] = GetQColor(disabledCkColor, GetQColor(normalCkColor, normalColor));
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"background-color", true, rePaint);
}

template<class QBase>
void ControlItemBackgroundForNormalHoverPressedDisabledAndCheck<QBase>::setItemBackgroundImage(const QString& backgroundImgPath,
																							   qint32 backgroundImgStateCount,
																							   qint32 backgroundImgNormal,
																							   qint32 backgroundImgHover,
																							   qint32 backgroundImgPressed,
																							   qint32 backgroundImgDisabled,
																							   qint32 backgroundImgCkNormal,
																							   qint32 backgroundImgCkHover,
																							   qint32 backgroundImgCkPressed,
																							   qint32 backgroundImgCkDisabled,
																							   bool rePaint)
{
	std::map<qint32, std::map<qint32, qint32>> imageStateMap;
	imageStateMap[UNCHECK][NORMAL] = backgroundImgNormal;
	imageStateMap[UNCHECK][HOVER] = backgroundImgHover;
	imageStateMap[UNCHECK][PRESSED] = backgroundImgPressed;
	imageStateMap[UNCHECK][DISABLED] = backgroundImgDisabled;
	imageStateMap[CHECK][NORMAL] = backgroundImgCkNormal;
	imageStateMap[CHECK][HOVER] = backgroundImgCkHover;
	imageStateMap[CHECK][PRESSED] = backgroundImgCkPressed;
	imageStateMap[CHECK][DISABLED] = backgroundImgCkDisabled;
	std::wstring wstrImgPath = backgroundImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, backgroundImgStateCount, L"background-image", true, rePaint);
}

#endif