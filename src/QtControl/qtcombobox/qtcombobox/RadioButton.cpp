#include "RadioButton.h"

RadioButton::RadioButton(QWidget* parent) :
ControlBase(parent)
{
	ControlBase::init(L"QRadioButton", L"indicator");
}

void RadioButton::setIndicatorImage(const QString& indicatorImg,
								 int32_t indicatorImgStateCount,
								 int32_t indicatorImgNormal,
								 int32_t indicatorImgHover,
								 int32_t indicatorImgPressed,
								 int32_t indicatorImgDisabled,
								 int32_t indicatorImgCkNormal,
								 int32_t indicatorImgCkHover,
								 int32_t indicatorImgCkPressed,
								 int32_t indicatorImgCkDisabled,
								 bool rePaint)
{
	std::map<int32_t, std::map<int32_t, int32_t>> indicatorImgMap;

	indicatorImgMap[UNCHECK][NORMAL] = indicatorImgNormal;
	indicatorImgMap[UNCHECK][HOVER] = indicatorImgHover;
	indicatorImgMap[UNCHECK][PRESSED] = indicatorImgPressed;
	indicatorImgMap[UNCHECK][DISABLED] = indicatorImgDisabled;
	indicatorImgMap[CHECK][NORMAL] = indicatorImgCkNormal;
	indicatorImgMap[CHECK][HOVER] = indicatorImgCkHover;
	indicatorImgMap[CHECK][PRESSED] = indicatorImgCkPressed;
	indicatorImgMap[CHECK][DISABLED] = indicatorImgCkDisabled;

	ControlBase::setImageStateMap(indicatorImgMap,
								  indicatorImg.toStdWString(),
								  indicatorImgStateCount,
								  L"border-image",
								  true,
								  rePaint);
}

void RadioButton::setBackgroundImage(const QString& backgroundImg,
								  int32_t backgroundImgStateCount,
								  int32_t backgroundImgNormal,
								  int32_t backgroundImgHover,
								  int32_t backgroundImgPressed,
								  int32_t backgroundImgDisabled,
								  int32_t backgroundImgCkNormal,
								  int32_t backgroundImgCkHover,
								  int32_t backgroundImgCkPressed,
								  int32_t backgroundImgCkDisabled,
								  bool rePaint)
{
	std::map<int32_t, std::map<int32_t, int32_t>> backgroundImgMap;

	backgroundImgMap[UNCHECK][NORMAL] = backgroundImgNormal;
	backgroundImgMap[UNCHECK][HOVER] = backgroundImgHover;
	backgroundImgMap[UNCHECK][PRESSED] = backgroundImgPressed;
	backgroundImgMap[UNCHECK][DISABLED] = backgroundImgDisabled;
	backgroundImgMap[CHECK][NORMAL] = backgroundImgCkNormal;
	backgroundImgMap[CHECK][HOVER] = backgroundImgCkHover;
	backgroundImgMap[CHECK][PRESSED] = backgroundImgCkPressed;
	backgroundImgMap[CHECK][DISABLED] = backgroundImgCkDisabled;

	ControlBase::setImageStateMap(backgroundImgMap,
								  backgroundImg.toStdWString(),
								  backgroundImgStateCount,
								  L"border-image",
								  false,
								  rePaint);
}

void RadioButton::setFontFace(const QString& fontName, bool rePaint)
{
	ControlBase::setFontFace(fontName.toStdWString(), false, rePaint);
}

void RadioButton::setFontSize(int32_t fontSize, bool rePaint)
{
	ControlBase::setPxValue(L"font-size", fontSize, false, rePaint);
}

void RadioButton::setTextColor(const QColor& textNormalColor,
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
	textColorMap[UNCHECK][HOVER] = textHoverColor;
	textColorMap[UNCHECK][PRESSED] = textPressedColor;
	textColorMap[UNCHECK][DISABLED] = textDisabledColor;
	textColorMap[CHECK][NORMAL] = textCkNormalColor;
	textColorMap[CHECK][HOVER] = textCkHoverColor;
	textColorMap[CHECK][PRESSED] = textCkPressedColor;
	textColorMap[CHECK][DISABLED] = textCkDisabledColor;

	ControlBase::setColorStateMap(textColorMap, L"color", false, rePaint);
}

void RadioButton::setBackgroundColor(const QColor& backgroundNormalColor,
	 	 		 				  const QColor& backgroundHoverColor,
								  const QColor& backgroundPressedColor,
								  const QColor& backgroundDisabledColor,
								  const QColor& backgroundCkNormalColor,
								  const QColor& backgroundCkHoverColor,
								  const QColor& backgroundCkPressedColor,
								  const QColor& backgroundCkDisabledColor,
								  bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> backgroundColorMap;

	backgroundColorMap[UNCHECK][NORMAL] = backgroundNormalColor;
	backgroundColorMap[UNCHECK][HOVER] = backgroundHoverColor;
	backgroundColorMap[UNCHECK][PRESSED] = backgroundPressedColor;
	backgroundColorMap[UNCHECK][DISABLED] = backgroundDisabledColor;
	backgroundColorMap[CHECK][NORMAL] = backgroundCkNormalColor;
	backgroundColorMap[CHECK][HOVER] = backgroundCkHoverColor;
	backgroundColorMap[CHECK][PRESSED] = backgroundCkPressedColor;
	backgroundColorMap[CHECK][DISABLED] = backgroundCkDisabledColor;

	ControlBase::setColorStateMap(backgroundColorMap, L"background-color", false, rePaint);
}

void RadioButton::setTextOrigin(int32_t origin, bool rePaint)
{
	ControlBase::setPxValue(L"spacing", origin, false, rePaint);
}