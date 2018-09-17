#include "RadioButton.h"

RadioButton::RadioButton(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	setItemName(L"indicator");
}

RadioButton::~RadioButton()
{

}

void RadioButton::setIndicatorImage(const QString& indicatorImg,
									qint32 indicatorImgStateCount,
									qint32 indicatorImgNormal,
									qint32 indicatorImgHover,
									qint32 indicatorImgPressed,
									qint32 indicatorImgDisabled,
									qint32 indicatorImgCkNormal,
									qint32 indicatorImgCkHover,
									qint32 indicatorImgCkPressed,
									qint32 indicatorImgCkDisabled,
									bool rePaint)
{
	std::map<qint32, std::map<qint32, qint32>> indicatorImgMap;

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

void RadioButton::setTextOrigin(qint32 origin, bool rePaint)
{
	ControlBase::setPxValue(L"spacing", origin, false, rePaint);
}