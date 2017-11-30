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
	ControlBase::setPxSolidValue(L"border", width, true, rePaint);
}

template<class QBase>
void ControlItem<QBase>::setItemWidth(int32_t width, bool rePaint)
{
	ControlBase::setPxValue(L"width", width, true, rePaint);
}

template<class QBase>
void ControlItem<QBase>::setItemHeight(int32_t height, bool rePaint)
{
	ControlBase::setPxValue(L"height", height, true, rePaint);
}

template<class QBase>
void ControlItem<QBase>::setItemTextOrigin(int32_t origin, bool rePaint)
{
	ControlBase::setPxValue(L"padding-left", origin, true, rePaint);
}

template<class QBase>
void ControlItem<QBase>::setItemAroundOrigin(int32_t leftOrigin,
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

#endif