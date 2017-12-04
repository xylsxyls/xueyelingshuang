#pragma once
#include <QtWidgets/QRadioButton>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"

class QWidget;
class RadioButton : 
	public ControlShow < QRadioButton >,
	public ControlFont < QRadioButton >,
	public ControlBorderForNormalHoverPressedDisabledAndCheck < QRadioButton >,
	public ControlBackgroundForNormalHoverPressedDisabledAndCheck < QRadioButton >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	RadioButton(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~RadioButton();

public:
	/** 设置CheckBox指示器图片
	@param [in] indicatorImg 背景图片路径，如果路径中必须使用正斜杠
	@param [in] indicatorImgStateCount 只能填4或8，表示4态按钮或8态按钮
	@param [in] indicatorImgNormal 非选中常态图片，如果填1表示将图片纵向分割4份或8份，选最上面的第一份
	@param [in] indicatorImgHover 非选中悬停图片
	@param [in] indicatorImgPressed 非选中按下图片
	@param [in] indicatorImgDisabled 非选中禁用图片
	@param [in] indicatorImgCKNormal 选中常态图片
	@param [in] indicatorImgCKHover 选中悬停图片
	@param [in] indicatorImgCKPressed 选中按下图片
	@param [in] indicatorImgCKDisabled 选中禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setIndicatorImage(const QString& indicatorImg,
			   			   int32_t indicatorImgStateCount = 4,
						   int32_t indicatorImgNormal = 1,
						   int32_t indicatorImgHover = 2,
						   int32_t indicatorImgPressed = 3,
						   int32_t indicatorImgDisabled = 4,
						   int32_t indicatorImgCKNormal = 1,
						   int32_t indicatorImgCKHover = 2,
						   int32_t indicatorImgCKPressed = 3,
						   int32_t indicatorImgCKDisabled = 4,
						   bool rePaint = false);

	/** 设置文本偏移量
	@param [in] origin 文本偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setTextOrigin(int32_t origin, bool rePaint = false);
};