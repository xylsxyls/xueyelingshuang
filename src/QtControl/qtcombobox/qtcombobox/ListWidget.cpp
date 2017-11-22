#include "ListWidget.h"

ListWidget::ListWidget(QWidget* parent) :
ControlBase(parent)
{
	init(L"QListWidget", L"item");
}

void ListWidget::setBackgroundColor(const QColor& color, bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = color;
	ControlBase::setColorStateMap(colorStateMap, L"background-color", false, rePaint);
}

void ListWidget::setBorderColor(const QColor& color, bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = color;
	ControlBase::setColorStateMap(colorStateMap, L"border-color", false, rePaint);
}

void ListWidget::setItemBorderColor(const QColor& normalColor,
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

void ListWidget::setItemBackgroundColor(const QColor& normalColor,
										const QColor& hoverColor,
										const QColor& disabledColor,
										bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][HOVER] = hoverColor;
	colorStateMap[NORMAL][DISABLED] = disabledColor;
	ControlBase::setColorStateMap(colorStateMap, L"background-color", true, rePaint);
}

void ListWidget::setItemBorderImage(const QString& borderImgPath,
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

void ListWidget::setTextColor(const QColor& normalColor,
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

void ListWidget::setTextOrigin(int32_t origin, bool rePaint)
{
	ControlBase::setPxValue(L"padding-left", origin, true, rePaint);
}