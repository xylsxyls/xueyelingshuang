#ifndef CONTROLSELF_INL__
#define CONTROLSELF_INL__

#include "ControlSelf.h"

template<class QBase>
ControlSelf<QBase>::~ControlSelf()
{

}

template<class QBase>
void ControlSelf<QBase>::setBorderRadius(int32_t radius, bool rePaint)
{
	ControlBase<QBase>::setPxValue(L"border-radius", radius, false, rePaint);
}

template<class QBase>
void ControlSelf<QBase>::setBorderWidth(int32_t width, bool rePaint)
{
	ControlBase<QBase>::setPxSolidValue(L"border", width, false, rePaint);
}

#endif