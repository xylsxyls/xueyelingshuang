#pragma once
#include <QtWidgets/QRadioButton>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "ControlsMacro.h"

class QWidget;
class ControlsAPI RadioButton :
	public ControlShow < QRadioButton >,
	public ControlFont < QRadioButton >,
	public ControlBorderForNormalHoverPressedDisabledAndCheck < QRadioButton >,
	public ControlBackgroundForNormalHoverPressedDisabledAndCheck < QRadioButton >
{
	Q_OBJECT
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
			   			   qint32 indicatorImgStateCount = 4,
						   qint32 indicatorImgNormal = 1,
						   qint32 indicatorImgHover = 2,
						   qint32 indicatorImgPressed = 3,
						   qint32 indicatorImgDisabled = 4,
						   qint32 indicatorImgCKNormal = 1,
						   qint32 indicatorImgCKHover = 2,
						   qint32 indicatorImgCKPressed = 3,
						   qint32 indicatorImgCKDisabled = 4,
						   bool rePaint = false);

	/** 设置文本偏移量
	@param [in] origin 文本偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setTextOrigin(qint32 origin, bool rePaint = false);
};