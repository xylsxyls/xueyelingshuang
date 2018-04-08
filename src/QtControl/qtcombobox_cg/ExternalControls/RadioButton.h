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
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	RadioButton(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~RadioButton();

public:
	/** ����CheckBoxָʾ��ͼƬ
	@param [in] indicatorImg ����ͼƬ·�������·���б���ʹ����б��
	@param [in] indicatorImgStateCount ֻ����4��8����ʾ4̬��ť��8̬��ť
	@param [in] indicatorImgNormal ��ѡ�г�̬ͼƬ�������1��ʾ��ͼƬ����ָ�4�ݻ�8�ݣ�ѡ������ĵ�һ��
	@param [in] indicatorImgHover ��ѡ����ͣͼƬ
	@param [in] indicatorImgPressed ��ѡ�а���ͼƬ
	@param [in] indicatorImgDisabled ��ѡ�н���ͼƬ
	@param [in] indicatorImgCKNormal ѡ�г�̬ͼƬ
	@param [in] indicatorImgCKHover ѡ����ͣͼƬ
	@param [in] indicatorImgCKPressed ѡ�а���ͼƬ
	@param [in] indicatorImgCKDisabled ѡ�н���ͼƬ
	@param [in] rePaint �Ƿ������ػ�
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

	/** �����ı�ƫ����
	@param [in] origin �ı�ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setTextOrigin(int32_t origin, bool rePaint = false);
};