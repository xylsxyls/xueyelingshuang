#ifndef CONTROLSELF_INL__
#define CONTROLSELF_INL__

#include "ControlSelf.h"

template<class QBase>
ControlSelf<QBase>::~ControlSelf()
{

}

template<class QBase>
void ControlSelf<QBase>::setBorderRadius(qint32 radius, bool rePaint)
{
	ControlBase<QBase>::setPxValue(L"border-radius", qMax(radius, 0), false, rePaint);
}

template<class QBase>
void ControlSelf<QBase>::setBorderWidth(qint32 width, bool rePaint)
{
	ControlBase<QBase>::setPxSolidValue(L"border", qMax(width, 0), false, rePaint);
}

#endif