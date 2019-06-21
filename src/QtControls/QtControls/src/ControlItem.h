#pragma once
#include "ControlBase.h"
#include "QtControlsMacro.h"

/** 管理节点相关功能
*/
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
	void setItemBorderWidth(qint32 width, bool rePaint = false);

	/** 将节点设为圆角，输入圆角半径，只有加入边框才会有圆角
	@param [in] radius 圆角半径
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderRadius(qint32 radius, bool rePaint = false);

	/** 设置节点宽度
	@param [in] width 节点宽度
	@param [in] rePaint 是否立即重画
	*/
	void setItemWidth(qint32 width, bool rePaint = false);

	/** 设置节点高度
	@param [in] height 节点高度
	@param [in] rePaint 是否立即重画
	*/
	void setItemHeight(qint32 height, bool rePaint = false);

	/** 设置节点文本偏移量
	@param [in] origin 节点文本偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setItemTextOrigin(qint32 origin, bool rePaint = false);

	/** 节点到窗口的四个外边距
	@param [in] leftOrigin 左侧偏移量
	@param [in] topOrigin 上侧偏移量
	@param [in] rightOrigin 右侧偏移量
	@param [in] bottomOrigin 下侧偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setItemAroundOrigin(qint32 leftOrigin,
						     qint32 topOrigin = -1,
							 qint32 rightOrigin = -1,
							 qint32 bottomOrigin = -1,
						     bool rePaint = false);
};

#include "ControlItem.inl"
