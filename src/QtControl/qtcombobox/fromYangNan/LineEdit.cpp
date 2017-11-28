#include "LineEdit.h"

LineEdit::LineEdit(QWidget* parent) :
ControlBase(parent)
{
	ControlBase::init(L"QLineEdit", L"");
	init();
}

void LineEdit::setBackgroundColor(const QColor& backgroundNormalColor,
								  const QColor& backgroundHoverColor,
								  const QColor& backgroundDisabledColor,
								  bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> backgroundColorMap;

	backgroundColorMap[NORMAL][NORMAL] = backgroundNormalColor;
	backgroundColorMap[NORMAL][HOVER] = backgroundHoverColor;
	backgroundColorMap[NORMAL][DISABLED] = backgroundDisabledColor;

	ControlBase::setColorStateMap(backgroundColorMap, L"background-color", false, rePaint);
}

void LineEdit::init()
{
	setBorderWidth(1);
}