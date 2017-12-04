#include "RadioButton.h"

RadioButton::RadioButton(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	init(L"QRadioButton", L"indicator");
}

RadioButton::~RadioButton()
{

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

void RadioButton::setTextOrigin(int32_t origin, bool rePaint)
{
	ControlBase::setPxValue(L"spacing", origin, false, rePaint);
}