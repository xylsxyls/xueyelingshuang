#include "CheckBox.h"

CheckBox::CheckBox(QWidget* parent) :
ControlBase(parent)
{
	ControlBase::init(L"QCheckBox", L"indicator");
}

void CheckBox::setIndicatorImage(const QString& indicatorImg,
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

void CheckBox::setBackgroundColor(const QColor& backgroundNormalColor,
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

void CheckBox::setTextOrigin(int32_t origin, bool rePaint)
{
	ControlBase::setPxValue(L"spacing", origin, false, rePaint);
}

void CheckBox::setIndicatorSize(int32_t width, int32_t height, bool rePaint)
{
	ControlBase::setPxValue(L"width", width, true, false);
	ControlBase::setPxValue(L"height", height, true, rePaint);
}