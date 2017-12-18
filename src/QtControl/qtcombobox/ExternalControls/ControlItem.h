#pragma once
#include "ControlBase.h"

template <class QBase>
class ControlItem : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlItem();

public:
	/** 设置节点边框粗度
	@param [in] width 边框粗度
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderWidth(int32_t width, bool rePaint = false);

	/** 设置节点宽度
	@param [in] width 节点宽度
	@param [in] rePaint 是否立即重画
	*/
	void setItemWidth(int32_t width, bool rePaint = false);

	/** 设置节点高度
	@param [in] height 节点高度
	@param [in] rePaint 是否立即重画
	*/
	void setItemHeight(int32_t height, bool rePaint = false);

	/** 设置节点文本偏移量
	@param [in] origin 节点文本偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setItemTextOrigin(int32_t origin, bool rePaint = false);

	/** 节点到窗口的四个外边距
	@param [in] leftOrigin 左侧偏移量
	@param [in] topOrigin 上侧偏移量
	@param [in] rightOrigin 右侧偏移量
	@param [in] bottomOrigin 下侧偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setItemAroundOrigin(int32_t leftOrigin,
						     int32_t topOrigin,
						     int32_t rightOrigin,
						     int32_t bottomOrigin,
						     bool rePaint = false);
};

#include "ControlItem.inl"
