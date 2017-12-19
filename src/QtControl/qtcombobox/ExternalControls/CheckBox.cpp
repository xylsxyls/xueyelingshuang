#include "CheckBox.h"

CheckBox::CheckBox(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	INIT(L"indicator");
}

CheckBox::~CheckBox()
{

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

void CheckBox::setTextOrigin(int32_t origin, bool rePaint)
{
	ControlBase::setPxValue(L"spacing", origin, false, rePaint);
}

void CheckBox::setIndicatorSize(int32_t width, int32_t height, bool rePaint)
{
	ControlBase::setPxValue(L"width", width, true, false);
	ControlBase::setPxValue(L"height", GetInt(height, width), true, rePaint);
}