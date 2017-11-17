#ifndef CONTROLSELF_H__
#define CONTROLSELF_H__
//#pragma once
#include "ControlBase.h"

template <class QBase>
class ControlSelf : virtual public ControlBase < QBase >
{
public:
	/** 将点击框设为圆角，输入圆角半径，只有加入边框才会有圆角
	@param [in] radius 圆角半径
	@param [in] rePaint 是否立即重画
	*/
	void setBorderRadius(int32_t radius, bool rePaint = false);

	/** 设置边框粗度
	@param [in] width 边框粗度
	@param [in] rePaint 是否立即重画
	*/
	void setBorderWidth(int32_t width, bool rePaint = false);
};

#include "ControlSelf.inl"

#endif