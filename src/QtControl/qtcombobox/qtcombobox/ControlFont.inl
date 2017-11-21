#ifndef CONTROLFONT_INL__
#define CONTROLFONT_INL__
//#pragma once
#include "ControlFont.h"

template<class QBase>
void ControlFont<QBase>::setFontFace(const QString& fontName, bool rePaint)
{
	ControlBase::setFontFace(fontName.toStdWString(), false, rePaint);
}

template<class QBase>
void ControlFont<QBase>::setFontSize(int32_t fontSize, bool rePaint)
{
	ControlBase::setPxValue(L"font-size", fontSize, false, rePaint);
}

template<class QBase>
void ControlFont<QBase>::setFontBold(bool bold, bool rePaint)
{
	if (bold)
	{
		ControlBase::setKeyValue(L"font-weight", L"bold", false, rePaint);
	}
}

#endif