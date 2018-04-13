#pragma once
#include "ControlBase.h"
#include "ControlsMacro.h"

/** 管理字体相关功能
*/
template <class QBase>
class ControlFont : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlFont();

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

	/** 设置字体粗体
	@param [in] bold 是否为粗体
	@param [in] rePaint 是否立即重画
	*/
	void setFontBold(bool bold, bool rePaint = false);

	/** 设置文字偏移量
	@param [in] origin 文字偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setTextOrigin(int32_t origin, bool rePaint = false);

	/** 设置文字右侧偏移量
	@param [in] origin 文字偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setTextRightOrigin(int32_t origin, bool rePaint = false);
};

#include "ControlFont.inl"
