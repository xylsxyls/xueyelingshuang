#ifndef CONTROLITEM_INL__
#define CONTROLITEM_INL__
//#pragma once
#include "ControlItem.h"

template<class QBase>
ControlItem<QBase>::~ControlItem()
{

}

template<class QBase>
void ControlItem<QBase>::setItemBorderWidth(int32_t width, bool rePaint)
{
	ControlBase<QBase>::setPxSolidValue(L"border", width, true, rePaint);
}

template<class QBase>
void ControlItem<QBase>::setItemBorderRadius(int32_t radius, bool rePaint)
{
	ControlBase<QBase>::setPxValue(L"border-radius", radius, true, rePaint);
}

template<class QBase>
void ControlItem<QBase>::setItemWidth(int32_t width, bool rePaint)
{
	ControlBase<QBase>::setPxValue(L"width", width, true, rePaint);
}

template<class QBase>
void ControlItem<QBase>::setItemHeight(int32_t height, bool rePaint)
{
	ControlBase<QBase>::setPxValue(L"height", height, true, rePaint);
}

template<class QBase>
void ControlItem<QBase>::setItemTextOrigin(int32_t origin, bool rePaint)
{
	ControlBase<QBase>::setPxValue(L"padding-left", origin, true, rePaint);
}

template<class QBase>
void ControlItem<QBase>::setItemAroundOrigin(int32_t leftOrigin,
										     int32_t topOrigin,
										     int32_t rightOrigin,
										     int32_t bottomOrigin,
										     bool rePaint)
{
	ControlBase<QBase>::setPxValue(L"margin-left", leftOrigin, true, false);
	ControlBase<QBase>::setPxValue(L"margin-top", GetInt(topOrigin, leftOrigin), true, false);
	ControlBase<QBase>::setPxValue(L"margin-right", GetInt(rightOrigin, leftOrigin), true, false);
	ControlBase<QBase>::setPxValue(L"margin-bottom", GetInt(bottomOrigin, GetInt(topOrigin, leftOrigin)), true, rePaint);
}

#endif