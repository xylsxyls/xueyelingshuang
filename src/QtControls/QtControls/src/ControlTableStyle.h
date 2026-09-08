#pragma once
#include <QColor>
#include "ControlBase.h"
#include "QtControlsMacro.h"

/** 表格类控件的通用样式模板，封装网格线、表头、视口和角落按钮样式
*/
template <class QBase>
class ControlTableStyle : virtual public ControlBase < QBase >
{
public:
	/** 析构函数
	*/
	virtual ~ControlTableStyle();

public:
	/** 设置表格网格线颜色
	@param [in] color 网格线颜色
	@param [in] rePaint 是否立即重画
	*/
	void setTableGridLineColor(const QColor& color, bool rePaint = false);

	/** 设置表头section高度
	@param [in] height 高度像素值
	@param [in] rePaint 是否立即重画
	*/
	void setTableHeaderSectionHeight(qint32 height, bool rePaint = false);

	/** 设置表头section宽度
	@param [in] width 宽度像素值
	@param [in] rePaint 是否立即重画
	*/
	void setTableHeaderSectionWidth(qint32 width, bool rePaint = false);

	/** 设置表头section边框宽度
	@param [in] width 边框宽度
	@param [in] rePaint 是否立即重画
	*/
	void setTableHeaderSectionBorderWidth(qint32 width, bool rePaint = false);

	/** 设置表头section边框颜色
	@param [in] color 边框颜色
	@param [in] rePaint 是否立即重画
	*/
	void setTableHeaderSectionBorderColor(const QColor& color, bool rePaint = false);

	/** 设置表头section背景颜色
	@param [in] color 背景颜色
	@param [in] rePaint 是否立即重画
	*/
	void setTableHeaderSectionBackgroundColor(const QColor& color, bool rePaint = false);

	/** 设置表头section文字颜色
	@param [in] color 文字颜色
	@param [in] rePaint 是否立即重画
	*/
	void setTableHeaderSectionTextColor(const QColor& color, bool rePaint = false);

	/** 设置表格角落按钮背景颜色
	@param [in] color 背景颜色
	@param [in] rePaint 是否立即重画
	*/
	void setTableCornerButtonBackgroundColor(const QColor& color, bool rePaint = false);

	/** 设置表格视口背景颜色
	@param [in] color 背景颜色
	@param [in] rePaint 是否立即重画
	*/
	void setTableViewportBackgroundColor(const QColor& color, bool rePaint = false);

protected:
	/** 获取最终显示控件指针
	@return 返回最终控件指针，继承结构不符合控件库约定时返回空指针
	*/
	ControlShow<QBase>* tableControlShow();

	/** 按需刷新最终控件样式
	@param [in] rePaint true表示立即刷新
	*/
	void repaintTableControl(bool rePaint);
};

#include "ControlTableStyle.inl"