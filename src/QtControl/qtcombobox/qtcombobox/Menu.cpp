#include "Menu.h"

Menu::Menu(QWidget* parent) :
ControlBase(parent)
{
	ControlBase::init(L"QMenu", L"item");
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

void Menu::setItemBorderColor(const QColor& normalColor,
									const QColor& hoverColor,
									const QColor& disabledColor,
									bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][HOVER] = hoverColor;
	colorStateMap[NORMAL][DISABLED] = disabledColor;
	ControlBase::setColorStateMap(colorStateMap, L"border-color", true, rePaint);
}

void Menu::setItemBorderWidth(int32_t width, bool rePaint)
{
	ControlBase::setPxSolidValue(L"border", width, true, rePaint);
}

void Menu::setItemBorderImage(const QString& borderImgPath,
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
	ControlBase::setImageStateMap(imageStateMap, wstrImgPath, borderImgStateCount, L"border-image", true, rePaint);
}

void Menu::setItemHeight(int32_t height, bool rePaint)
{
	ControlBase::setPxValue(L"height", height, true, rePaint);
}

void Menu::setTextColor(const QColor& normalColor,
							  const QColor& hoverColor,
							  const QColor& disabledColor,
							  bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][HOVER] = hoverColor;
	colorStateMap[NORMAL][DISABLED] = disabledColor;
	ControlBase::setColorStateMap(colorStateMap, L"color", true, rePaint);
}

void Menu::setFontFace(const QString& fontName, bool rePaint)
{
	ControlBase::setFontFace(fontName.toStdWString(), false, rePaint);
}

void Menu::setFontSize(int32_t fontSize, bool rePaint)
{
	ControlBase::setPxValue(L"font-size", fontSize, false, rePaint);
}

void Menu::setTextOrigin(int32_t origin, bool rePaint)
{
	ControlBase::setPxValue(L"padding-left", origin, true, rePaint);
}

void Menu::setItemAroundOrigin(int32_t leftOrigin,
							   int32_t topOrigin,
							   int32_t rightOrigin,
							   int32_t bottomOrigin,
							   bool rePaint)
{
	ControlBase::setPxValue(L"margin-left", leftOrigin, true, false);
	ControlBase::setPxValue(L"margin-top", topOrigin, true, false);
	ControlBase::setPxValue(L"margin-right", rightOrigin, true, false);
	ControlBase::setPxValue(L"margin-bottom", bottomOrigin, true, rePaint);
}