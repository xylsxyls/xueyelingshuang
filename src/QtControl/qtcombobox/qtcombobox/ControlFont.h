#ifndef CONTROLFONT_H__
#define CONTROLFONT_H__
//#pragma once
#include "ControlBase.h"

template <class QBase>
class ControlFont : virtual public ControlBase < QBase >
{
public:
	/** 设置字体
	@param [in] fontName 字体名
	@param [in] rePaint 是否立即重画
	*/
	void setFontFace(const QString& fontName, bool rePaint = false);

	/** 设置字体大小
	@param [in] fontSize 字体大小
	@param [in] rePaint 是否立即重画
	*/
	void setFontSize(int32_t fontSize, bool rePaint = false);
};

#include "ControlFont.inl"

#endif