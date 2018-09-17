#ifndef CONTROLITEM_INL__
#define CONTROLITEM_INL__
//#pragma once
#include "ControlItem.h"

template<class QBase>
ControlItem<QBase>::~ControlItem()
{

}

template<class QBase>
void ControlItem<QBase>::setItemBorderWidth(qint32 width, bool rePaint)
{
	ControlBase<QBase>::setPxSolidValue(L"border", width, true, rePaint);
}

template<class QBase>
void ControlItem<QBase>::setItemBorderRadius(qint32 radius, bool rePaint)
{
	ControlBase<QBase>::setPxValue(L"border-radius", radius, true, rePaint);
}

template<class QBase>
void ControlItem<QBase>::setItemWidth(qint32 width, bool rePaint)
{
	ControlBase<QBase>::setPxValue(L"width", width, true, rePaint);
}

template<class QBase>
void ControlItem<QBase>::setItemHeight(qint32 height, bool rePaint)
{
	ControlBase<QBase>::setPxValue(L"height", height, true, rePaint);
}

template<class QBase>
void ControlItem<QBase>::setItemTextOrigin(qint32 origin, bool rePaint)
{
	ControlBase<QBase>::setPxValue(L"padding-left", origin, true, rePaint);
}

template<class QBase>
void ControlItem<QBase>::setItemAroundOrigin(qint32 leftOrigin,
										     qint32 topOrigin,
										     qint32 rightOrigin,
										     qint32 bottomOrigin,
										     bool rePaint)
{
	ControlBase<QBase>::setPxValue(L"margin-left", leftOrigin, true, false);
	ControlBase<QBase>::setPxValue(L"margin-top", GetInt(topOrigin, leftOrigin), true, false);
	ControlBase<QBase>::setPxValue(L"margin-right", GetInt(rightOrigin, leftOrigin), true, false);
	ControlBase<QBase>::setPxValue(L"margin-bottom", GetInt(bottomOrigin, GetInt(topOrigin, leftOrigin)), true, rePaint);
}

#endif