#ifndef CONTROLBORDER_INL__
#define CONTROLBORDER_INL__
//#pragma once
#include "ControlBorder.h"

template<class QBase>
ControlBorderForNormal<QBase>::~ControlBorderForNormal()
{

}

template<class QBase>
void ControlBorderForNormal<QBase>::setBorderColor(const QColor& color, bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = color;
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"border-color", false, rePaint);
}

template<class QBase>
void ControlBorderForNormal<QBase>::setBorderImage(const QString& borderImgPath, bool rePaint)
{
	std::map<int32_t, std::map<int32_t, int32_t>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = 1;
	std::wstring wstrImgPath = borderImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, 1, L"border-image", false, rePaint);
}

template<class QBase>
void ControlBorderForNormal<QBase>::setTextColor(const QColor& textColor, bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> textColorMap;
	textColorMap[NORMAL][NORMAL] = textColor;
	ControlBase<QBase>::setColorStateMap(textColorMap, L"color", false, rePaint);
}

template<class QBase>
ControlBorderForNormalHoverDisabled<QBase>::~ControlBorderForNormalHoverDisabled()
{

}

template<class QBase>
void ControlBorderForNormalHoverDisabled<QBase>::setBorderColor(const QColor& normalColor,
																const QColor& hoverColor,
																const QColor& disabledColor,
																bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][HOVER] = GetQColor(hoverColor, normalColor);
	colorStateMap[NORMAL][DISABLED] = GetQColor(disabledColor, normalColor);
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"border-color", false, rePaint);
}

template<class QBase>
void ControlBorderForNormalHoverDisabled<QBase>::setBorderImage(const QString& borderImgPath,
																int32_t borderImgStateCount,
																int32_t borderImgNormal,
																int32_t borderImgHover,
																int32_t borderImgDisabled,
																bool rePaint)
{
	std::map<int32_t, std::map<int32_t, int32_t>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = borderImgNormal;
	imageStateMap[NORMAL][HOVER] = borderImgHover;
	imageStateMap[NORMAL][DISABLED] = borderImgDisabled;
	std::wstring wstrImgPath = borderImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, borderImgStateCount, L"border-image", false, rePaint);
}

template<class QBase>
void ControlBorderForNormalHoverDisabled<QBase>::setTextColor(const QColor& textNormalColor,
															  const QColor& textHoverColor,
															  const QColor& textDisabledColor,
															  bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> textColorMap;

	textColorMap[NORMAL][NORMAL] = textNormalColor;
	textColorMap[NORMAL][HOVER] = GetQColor(textHoverColor, textNormalColor);
	textColorMap[NORMAL][DISABLED] = GetQColor(textDisabledColor, textNormalColor);

	ControlBase<QBase>::setColorStateMap(textColorMap, L"color", false, rePaint);
}

template<class QBase>
ControlBorderForNormalSelectedDisabled<QBase>::~ControlBorderForNormalSelectedDisabled()
{

}

template<class QBase>
void ControlBorderForNormalSelectedDisabled<QBase>::setBorderColor(const QColor& normalColor,
																   const QColor& selectedColor,
																   const QColor& disabledColor,
																   bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][SELECTED] = GetQColor(selectedColor, normalColor);
	colorStateMap[NORMAL][DISABLED] = GetQColor(disabledColor, normalColor);
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"border-color", false, rePaint);
}

template<class QBase>
void ControlBorderForNormalSelectedDisabled<QBase>::setBorderImage(const QString& borderImgPath,
																   int32_t borderImgStateCount,
																   int32_t borderImgNormal,
																   int32_t borderImgSelected,
																   int32_t borderImgDisabled,
																   bool rePaint)
{
	std::map<int32_t, std::map<int32_t, int32_t>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = borderImgNormal;
	imageStateMap[NORMAL][SELECTED] = borderImgSelected;
	imageStateMap[NORMAL][DISABLED] = borderImgDisabled;
	std::wstring wstrImgPath = borderImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, borderImgStateCount, L"border-image", false, rePaint);
}

template<class QBase>
void ControlBorderForNormalSelectedDisabled<QBase>::setTextColor(const QColor& textNormalColor,
																 const QColor& textSelectedColor,
																 const QColor& textDisabledColor,
																 bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> textColorMap;

	textColorMap[NORMAL][NORMAL] = textNormalColor;
	textColorMap[NORMAL][SELECTED] = GetQColor(textSelectedColor, textNormalColor);
	textColorMap[NORMAL][DISABLED] = GetQColor(textDisabledColor, textNormalColor);

	ControlBase<QBase>::setColorStateMap(textColorMap, L"color", false, rePaint);
}

template<class QBase>
ControlBorderForNormalHoverPressedDisabled<QBase>::~ControlBorderForNormalHoverPressedDisabled()
{

}

template<class QBase>
void ControlBorderForNormalHoverPressedDisabled<QBase>::setBorderColor(const QColor& normalColor,
																	   const QColor& hoverColor,
																	   const QColor& pressedColor,
																	   const QColor& disabledColor,
																	   bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][HOVER] = GetQColor(hoverColor, normalColor);
	colorStateMap[NORMAL][PRESSED] = GetQColor(pressedColor, normalColor);
	colorStateMap[NORMAL][DISABLED] = GetQColor(disabledColor, normalColor);
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"border-color", false, rePaint);
}

template<class QBase>
void ControlBorderForNormalHoverPressedDisabled<QBase>::setBorderImage(const QString& borderImgPath,
																	   int32_t borderImgStateCount,
																	   int32_t borderImgNormal,
																	   int32_t borderImgHover,
																	   int32_t borderImgPressed,
																	   int32_t borderImgDisabled,
																	   bool rePaint)
{
	std::map<int32_t, std::map<int32_t, int32_t>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = borderImgNormal;
	imageStateMap[NORMAL][HOVER] = borderImgHover;
	imageStateMap[NORMAL][PRESSED] = borderImgPressed;
	imageStateMap[NORMAL][DISABLED] = borderImgDisabled;
	std::wstring wstrImgPath = borderImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, borderImgStateCount, L"border-image", false, rePaint);
}

template<class QBase>
void ControlBorderForNormalHoverPressedDisabled<QBase>::setTextColor(const QColor& textNormalColor,
																	 const QColor& textHoverColor,
																	 const QColor& textPressedColor,
																	 const QColor& textDisabledColor,
																	 bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> textColorMap;

	textColorMap[NORMAL][NORMAL] = textNormalColor;
	textColorMap[NORMAL][HOVER] = GetQColor(textHoverColor, textNormalColor);
	textColorMap[NORMAL][PRESSED] = GetQColor(textPressedColor, textNormalColor);
	textColorMap[NORMAL][DISABLED] = GetQColor(textDisabledColor, textNormalColor);

	ControlBase<QBase>::setColorStateMap(textColorMap, L"color", false, rePaint);
}

template<class QBase>
ControlBorderForNormalHoverPressedDisabledAndCheck<QBase>::~ControlBorderForNormalHoverPressedDisabledAndCheck()
{

}

template<class QBase>
void ControlBorderForNormalHoverPressedDisabledAndCheck<QBase>::setBorderColor(const QColor& normalColor,
																			   const QColor& hoverColor,
																			   const QColor& pressedColor,
																			   const QColor& disabledColor,
																			   const QColor& normalCkColor,
																			   const QColor& hoverCkColor,
																			   const QColor& pressedCkColor,
																			   const QColor& disabledCkColor,
																			   bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[UNCHECK][NORMAL] = normalColor;
	colorStateMap[UNCHECK][HOVER] = GetQColor(hoverColor, normalColor);
	colorStateMap[UNCHECK][PRESSED] = GetQColor(pressedColor, normalColor);
	colorStateMap[UNCHECK][DISABLED] = GetQColor(disabledColor, normalColor);
	colorStateMap[CHECK][NORMAL] = GetQColor(normalCkColor, normalColor);
	colorStateMap[CHECK][HOVER] = GetQColor(hoverCkColor, GetQColor(normalCkColor, normalColor));
	colorStateMap[CHECK][PRESSED] = GetQColor(pressedCkColor, GetQColor(normalCkColor, normalColor));
	colorStateMap[CHECK][DISABLED] = GetQColor(disabledCkColor, GetQColor(normalCkColor, normalColor));
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"border-color", false, rePaint);
}

template<class QBase>
void ControlBorderForNormalHoverPressedDisabledAndCheck<QBase>::setBorderImage(const QString& borderImgPath,
																			   int32_t borderImgStateCount,
																			   int32_t borderImgNormal,
																			   int32_t borderImgHover,
																			   int32_t borderImgPressed,
																			   int32_t borderImgDisabled,
																			   int32_t borderImgCkNormal,
																			   int32_t borderImgCkHover,
																			   int32_t borderImgCkPressed,
																			   int32_t borderImgCkDisabled,
																			   bool rePaint)
{
	std::map<int32_t, std::map<int32_t, int32_t>> imageStateMap;
	imageStateMap[UNCHECK][NORMAL] = borderImgNormal;
	imageStateMap[UNCHECK][HOVER] = borderImgHover;
	imageStateMap[UNCHECK][PRESSED] = borderImgPressed;
	imageStateMap[UNCHECK][DISABLED] = borderImgDisabled;
	imageStateMap[CHECK][NORMAL] = borderImgCkNormal;
	imageStateMap[CHECK][HOVER] = borderImgCkHover;
	imageStateMap[CHECK][PRESSED] = borderImgCkPressed;
	imageStateMap[CHECK][DISABLED] = borderImgCkDisabled;
	std::wstring wstrImgPath = borderImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, borderImgStateCount, L"border-image", false, rePaint);
}

template<class QBase>
void ControlBorderForNormalHoverPressedDisabledAndCheck<QBase>::setTextColor(const QColor& textNormalColor,
																			 const QColor& textHoverColor,
																			 const QColor& textPressedColor,
																			 const QColor& textDisabledColor,
																			 const QColor& textCkNormalColor,
																			 const QColor& textCkHoverColor,
																			 const QColor& textCkPressedColor,
																			 const QColor& textCkDisabledColor,
																			 bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> textColorMap;

	textColorMap[UNCHECK][NORMAL] = textNormalColor;
	textColorMap[UNCHECK][HOVER] = GetQColor(textHoverColor, textNormalColor);
	textColorMap[UNCHECK][PRESSED] = GetQColor(textPressedColor, textNormalColor);
	textColorMap[UNCHECK][DISABLED] = GetQColor(textDisabledColor, textNormalColor);
	textColorMap[CHECK][NORMAL] = GetQColor(textCkNormalColor, textNormalColor);
	textColorMap[CHECK][HOVER] = GetQColor(textCkHoverColor, GetQColor(textCkNormalColor, textNormalColor));
	textColorMap[CHECK][PRESSED] = GetQColor(textCkPressedColor, GetQColor(textCkNormalColor, textNormalColor));
	textColorMap[CHECK][DISABLED] = GetQColor(textCkDisabledColor, GetQColor(textCkNormalColor, textNormalColor));

	ControlBase<QBase>::setColorStateMap(textColorMap, L"color", false, rePaint);
}

template<class QBase>
ControlItemBorderForNormal<QBase>::~ControlItemBorderForNormal()
{

}

template<class QBase>
void ControlItemBorderForNormal<QBase>::setItemBorderColor(const QColor& color, bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = color;
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"border-color", true, rePaint);
}

template<class QBase>
void ControlItemBorderForNormal<QBase>::setItemBorderImage(const QString& borderImgPath, bool rePaint)
{
	std::map<int32_t, std::map<int32_t, int32_t>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = 1;
	std::wstring wstrImgPath = borderImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, 1, L"border-image", true, rePaint);
}

template<class QBase>
void ControlItemBorderForNormal<QBase>::setItemTextColor(const QColor& textColor, bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> textColorMap;
	textColorMap[NORMAL][NORMAL] = textColor;
	ControlBase<QBase>::setColorStateMap(textColorMap, L"color", true, rePaint);
}

template<class QBase>
ControlItemBorderForNormalHoverDisabled<QBase>::~ControlItemBorderForNormalHoverDisabled()
{

}

template<class QBase>
void ControlItemBorderForNormalHoverDisabled<QBase>::setItemBorderColor(const QColor& normalColor,
																const QColor& hoverColor,
																const QColor& disabledColor,
																bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][HOVER] = GetQColor(hoverColor, normalColor);
	colorStateMap[NORMAL][DISABLED] = GetQColor(disabledColor, normalColor);
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"border-color", true, rePaint);
}

template<class QBase>
void ControlItemBorderForNormalHoverDisabled<QBase>::setItemBorderImage(const QString& borderImgPath,
																int32_t borderImgStateCount,
																int32_t borderImgNormal,
																int32_t borderImgHover,
																int32_t borderImgDisabled,
																bool rePaint)
{
	std::map<int32_t, std::map<int32_t, int32_t>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = borderImgNormal;
	imageStateMap[NORMAL][HOVER] = borderImgHover;
	imageStateMap[NORMAL][DISABLED] = borderImgDisabled;
	std::wstring wstrImgPath = borderImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, borderImgStateCount, L"border-image", true, rePaint);
}

template<class QBase>
void ControlItemBorderForNormalHoverDisabled<QBase>::setItemTextColor(const QColor& textNormalColor,
																	  const QColor& textHoverColor,
																	  const QColor& textDisabledColor,
																	  bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> textColorMap;

	textColorMap[NORMAL][NORMAL] = textNormalColor;
	textColorMap[NORMAL][HOVER] = GetQColor(textHoverColor, textNormalColor);
	textColorMap[NORMAL][DISABLED] = GetQColor(textDisabledColor, textNormalColor);

	ControlBase<QBase>::setColorStateMap(textColorMap, L"color", true, rePaint);
}

template<class QBase>
ControlItemBorderForNormalSelectedDisabled<QBase>::~ControlItemBorderForNormalSelectedDisabled()
{

}

template<class QBase>
void ControlItemBorderForNormalSelectedDisabled<QBase>::setItemBorderColor(const QColor& normalColor,
																		   const QColor& selectedColor,
																		   const QColor& disabledColor,
																		   bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][SELECTED] = GetQColor(selectedColor, normalColor);
	colorStateMap[NORMAL][DISABLED] = GetQColor(disabledColor, normalColor);
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"border-color", true, rePaint);
}

template<class QBase>
void ControlItemBorderForNormalSelectedDisabled<QBase>::setItemBorderImage(const QString& borderImgPath,
																		   int32_t borderImgStateCount,
																		   int32_t borderImgNormal,
																		   int32_t borderImgSelected,
																		   int32_t borderImgDisabled,
																		   bool rePaint)
{
	std::map<int32_t, std::map<int32_t, int32_t>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = borderImgNormal;
	imageStateMap[NORMAL][SELECTED] = borderImgSelected;
	imageStateMap[NORMAL][DISABLED] = borderImgDisabled;
	std::wstring wstrImgPath = borderImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, borderImgStateCount, L"border-image", true, rePaint);
}

template<class QBase>
void ControlItemBorderForNormalSelectedDisabled<QBase>::setItemTextColor(const QColor& textNormalColor,
																		 const QColor& textSelectedColor,
																		 const QColor& textDisabledColor,
																		 bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> textColorMap;

	textColorMap[NORMAL][NORMAL] = textNormalColor;
	textColorMap[NORMAL][SELECTED] = GetQColor(textSelectedColor, textNormalColor);
	textColorMap[NORMAL][DISABLED] = GetQColor(textDisabledColor, textNormalColor);

	ControlBase<QBase>::setColorStateMap(textColorMap, L"color", true, rePaint);
}

template<class QBase>
ControlItemBorderForNormalHoverPressedDisabled<QBase>::~ControlItemBorderForNormalHoverPressedDisabled()
{

}

template<class QBase>
void ControlItemBorderForNormalHoverPressedDisabled<QBase>::setItemBorderColor(const QColor& normalColor,
																			   const QColor& hoverColor,
																			   const QColor& pressedColor,
																			   const QColor& disabledColor,
																			   bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][HOVER] = GetQColor(hoverColor, normalColor);
	colorStateMap[NORMAL][PRESSED] = GetQColor(pressedColor, normalColor);
	colorStateMap[NORMAL][DISABLED] = GetQColor(disabledColor, normalColor);
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"border-color", true, rePaint);
}

template<class QBase>
void ControlItemBorderForNormalHoverPressedDisabled<QBase>::setItemBorderImage(const QString& borderImgPath,
																			   int32_t borderImgStateCount,
																			   int32_t borderImgNormal,
																			   int32_t borderImgHover,
																			   int32_t borderImgPressed,
																			   int32_t borderImgDisabled,
																			   bool rePaint)
{
	std::map<int32_t, std::map<int32_t, int32_t>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = borderImgNormal;
	imageStateMap[NORMAL][HOVER] = borderImgHover;
	imageStateMap[NORMAL][PRESSED] = borderImgPressed;
	imageStateMap[NORMAL][DISABLED] = borderImgDisabled;
	std::wstring wstrImgPath = borderImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, borderImgStateCount, L"border-image", true, rePaint);
}

template<class QBase>
void ControlItemBorderForNormalHoverPressedDisabled<QBase>::setItemTextColor(const QColor& textNormalColor,
																			 const QColor& textHoverColor,
																			 const QColor& textPressedColor,
																			 const QColor& textDisabledColor,
																			 bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> textColorMap;

	textColorMap[NORMAL][NORMAL] = textNormalColor;
	textColorMap[NORMAL][HOVER] = GetQColor(textHoverColor, textNormalColor);
	textColorMap[NORMAL][PRESSED] = GetQColor(textPressedColor, textNormalColor);
	textColorMap[NORMAL][DISABLED] = GetQColor(textDisabledColor, textNormalColor);

	ControlBase<QBase>::setColorStateMap(textColorMap, L"color", true, rePaint);
}

template<class QBase>
ControlItemBorderForNormalHoverPressedDisabledAndCheck<QBase>::~ControlItemBorderForNormalHoverPressedDisabledAndCheck()
{

}

template<class QBase>
void ControlItemBorderForNormalHoverPressedDisabledAndCheck<QBase>::setItemBorderColor(const QColor& normalColor,
																					   const QColor& hoverColor,
																					   const QColor& pressedColor,
																					   const QColor& disabledColor,
																					   const QColor& normalCkColor,
																					   const QColor& hoverCkColor,
																					   const QColor& pressedCkColor,
																					   const QColor& disabledCkColor,
																					   bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[UNCHECK][NORMAL] = normalColor;
	colorStateMap[UNCHECK][HOVER] = GetQColor(hoverColor, normalColor);
	colorStateMap[UNCHECK][PRESSED] = GetQColor(pressedColor, normalColor);
	colorStateMap[UNCHECK][DISABLED] = GetQColor(disabledColor, normalColor);
	colorStateMap[CHECK][NORMAL] = GetQColor(normalCkColor, normalColor);
	colorStateMap[CHECK][HOVER] = GetQColor(hoverCkColor, GetQColor(normalCkColor, normalColor));
	colorStateMap[CHECK][PRESSED] = GetQColor(pressedCkColor, GetQColor(normalCkColor, normalColor));
	colorStateMap[CHECK][DISABLED] = GetQColor(disabledCkColor, GetQColor(normalCkColor, normalColor));
	ControlBase<QBase>::setColorStateMap(colorStateMap, L"border-color", true, rePaint);
}

template<class QBase>
void ControlItemBorderForNormalHoverPressedDisabledAndCheck<QBase>::setItemBorderImage(const QString& borderImgPath,
																					   int32_t borderImgStateCount,
																					   int32_t borderImgNormal,
																					   int32_t borderImgHover,
																					   int32_t borderImgPressed,
																					   int32_t borderImgDisabled,
																					   int32_t borderImgCkNormal,
																					   int32_t borderImgCkHover,
																					   int32_t borderImgCkPressed,
																					   int32_t borderImgCkDisabled,
																					   bool rePaint)
{
	std::map<int32_t, std::map<int32_t, int32_t>> imageStateMap;
	imageStateMap[UNCHECK][NORMAL] = borderImgNormal;
	imageStateMap[UNCHECK][HOVER] = borderImgHover;
	imageStateMap[UNCHECK][PRESSED] = borderImgPressed;
	imageStateMap[UNCHECK][DISABLED] = borderImgDisabled;
	imageStateMap[CHECK][NORMAL] = borderImgCkNormal;
	imageStateMap[CHECK][HOVER] = borderImgCkHover;
	imageStateMap[CHECK][PRESSED] = borderImgCkPressed;
	imageStateMap[CHECK][DISABLED] = borderImgCkDisabled;
	std::wstring wstrImgPath = borderImgPath.toStdWString();
	ControlBase<QBase>::setImageStateMap(imageStateMap, wstrImgPath, borderImgStateCount, L"border-image", true, rePaint);
}

template<class QBase>
void ControlItemBorderForNormalHoverPressedDisabledAndCheck<QBase>::setItemTextColor(const QColor& textNormalColor,
																					 const QColor& textHoverColor,
																					 const QColor& textPressedColor,
																					 const QColor& textDisabledColor,
																					 const QColor& textCkNormalColor,
																					 const QColor& textCkHoverColor,
																					 const QColor& textCkPressedColor,
																					 const QColor& textCkDisabledColor,
																					 bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> textColorMap;

	textColorMap[UNCHECK][NORMAL] = textNormalColor;
	textColorMap[UNCHECK][HOVER] = GetQColor(textHoverColor, textNormalColor);
	textColorMap[UNCHECK][PRESSED] = GetQColor(textPressedColor, textNormalColor);
	textColorMap[UNCHECK][DISABLED] = GetQColor(textDisabledColor, textNormalColor);
	textColorMap[CHECK][NORMAL] = GetQColor(textCkNormalColor, textNormalColor);
	textColorMap[CHECK][HOVER] = GetQColor(textCkHoverColor, GetQColor(textCkNormalColor, textNormalColor));
	textColorMap[CHECK][PRESSED] = GetQColor(textCkPressedColor, GetQColor(textCkNormalColor, textNormalColor));
	textColorMap[CHECK][DISABLED] = GetQColor(textCkDisabledColor, GetQColor(textCkNormalColor, textNormalColor));

	ControlBase<QBase>::setColorStateMap(textColorMap, L"color", true, rePaint);
}

#endif