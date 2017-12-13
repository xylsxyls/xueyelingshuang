#ifndef CONTROLFONT_INL__
#define CONTROLFONT_INL__
//#pragma once
#include "ControlFont.h"

template<class QBase>
ControlFont<QBase>::~ControlFont()
{

}

template<class QBase>
void ControlFont<QBase>::setFontFace(const QString& fontName, bool rePaint)
{
	ControlBase<QBase>::setFontFace(fontName.toStdWString(), false, rePaint);
}

template<class QBase>
void ControlFont<QBase>::setFontSize(int32_t fontSize, bool rePaint)
{
	ControlBase<QBase>::setPxValue(L"font-size", fontSize, false, rePaint);
}

template<class QBase>
void ControlFont<QBase>::setFontBold(bool bold, bool rePaint)
{
	if (bold)
	{
		ControlBase<QBase>::setKeyValue(L"font-weight", L"bold", false, rePaint);
	}
}

template<class QBase>
void ControlFont<QBase>::setTextOrigin(int32_t origin, bool rePaint)
{
	ControlBase<QBase>::setPxValue(L"padding-left", origin, false, rePaint);
}

template<class QBase>
void ControlFont<QBase>::setTextRightOrigin(int32_t origin, bool rePaint)
{
	ControlBase<QBase>::setPxValue(L"padding-right", origin, false, rePaint);
}

#endif