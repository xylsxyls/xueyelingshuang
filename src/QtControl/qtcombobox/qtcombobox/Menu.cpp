#include "Menu.h"

Menu::Menu(QWidget* parent) :
ControlBase(parent)
{
	ControlBase::init(L"QMenu", L"");
}

void Menu::setBackgroundColor(const QColor& color, bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = color;
	ControlBase::setColorStateMap(colorStateMap, L"background-color", false, rePaint);
}

void Menu::setBorderWidth(int32_t width, bool rePaint)
{
	ControlBase::setPxSolidValue(L"border", width, false, rePaint);
}

void Menu::setBorderColor(const QColor& color, bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = color;
	ControlBase::setColorStateMap(colorStateMap, L"border-color", false, rePaint);
}