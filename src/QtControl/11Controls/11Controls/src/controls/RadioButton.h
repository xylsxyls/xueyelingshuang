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

	/** �����ı�ƫ����
	@param [in] origin �ı�ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setTextOrigin(qint32 origin, bool rePaint = false);
};