#pragma once
#include "ControlBase.h"

/**************���ļ�������Ҫ��������ɫ��ز���**************/

template <class QBase>
class ControlBorderForNormal : virtual public ControlBase < QBase >
{
public:
	/** ��������
	*/
	virtual ~ControlBorderForNormal();

public:
	/** ���ñ߿���ɫ
	@param [in] color ��̬
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setBorderColor(const QColor& color, bool rePaint = false);

	/** ���ñ߿�ͼƬ
	@param [in] borderImgPath ͼƬ·��
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setBorderImage(const QString& borderImgPath, bool rePaint = false);

	/** ����������ɫ
	@param [in] textColor ��̬��ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setTextColor(const QColor& textColor, bool rePaint = false);
};

template <class QBase>
class ControlBorderForNormalHoverDisabled : virtual public ControlBase < QBase >
{

public:
	/** ��������
	*/
	virtual ~ControlBorderForNormalHoverDisabled();

public:
	/** ���ñ�����ɫ
	@param [in] normalColor ��̬��ɫ
	@param [in] hoverColor ��ͣ��ɫ
	@param [in] disabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setBorderColor(const QColor& normalColor,
						const QColor& hoverColor = QColor(0, 0, 0, 0),
						const QColor& disabledColor = QColor(0, 0, 0, 0),
						bool rePaint = false);

	/** ���ýڵ㱳��ͼƬ���ͱ߿���ɫ���ô�
	@param [in] borderImgPath ����ͼƬ·�������·���б���ʹ����б��
	@param [in] borderImgStateCount ����ƽ�ּ���
	@param [in] borderImgNormal ��̬ͼƬ�������1��ʾѡ�������һ��
	@param [in] borderImgHover ��ͣͼƬ
	@param [in] borderImgDisabled ����ͼƬ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setBorderImage(const QString& borderImgPath,
						int32_t borderImgStateCount = 4,
						int32_t borderImgNormal = 1,
						int32_t borderImgHover = 2,
						int32_t borderImgDisabled = 4,
						bool rePaint = false);

	/** ����������ɫ
	@param [in] textNormalColor ��̬��ɫ
	@param [in] textHoverColor ��ͣ��ɫ
	@param [in] textDisabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setTextColor(const QColor& textNormalColor,
					  const QColor& textHoverColor = QColor(0, 0, 0, 0),
					  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
					  bool rePaint = false);
};

template <class QBase>
class ControlBorderForNormalSelectedDisabled : virtual public ControlBase < QBase >
{
public:
	/** ��������
	*/
	virtual ~ControlBorderForNormalSelectedDisabled();

public:
	/** ���ñ�����ɫ
	@param [in] normalColor ��̬��ɫ
	@param [in] selectedColor ��ͣ��ɫ������list��
	@param [in] disabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setBorderColor(const QColor& normalColor,
						const QColor& selectedColor = QColor(0, 0, 0, 0),
						const QColor& disabledColor = QColor(0, 0, 0, 0),
						bool rePaint = false);

	/** ���ýڵ㱳��ͼƬ���ͱ߿���ɫ���ô�
	@param [in] borderImgPath ����ͼƬ·�������·���б���ʹ����б��
	@param [in] borderImgStateCount ����ƽ�ּ���
	@param [in] borderImgNormal ��̬ͼƬ�������1��ʾѡ�������һ��
	@param [in] borderImgSelected ��ͣͼƬ������list��
	@param [in] borderImgDisabled ����ͼƬ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setBorderImage(const QString& borderImgPath,
						int32_t borderImgStateCount = 4,
						int32_t borderImgNormal = 1,
						int32_t borderImgSelected = 2,
						int32_t borderImgDisabled = 4,
						bool rePaint = false);

	/** ����������ɫ
	@param [in] textNormalColor ��̬��ɫ
	@param [in] textSelectedColor ��ͣ��ɫ������list��
	@param [in] textDisabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setTextColor(const QColor& textNormalColor,
					  const QColor& textSelectedColor = QColor(0, 0, 0, 0),
					  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
					  bool rePaint = false);
};

template <class QBase>
class ControlBorderForNormalHoverPressedDisabled : virtual public ControlBase < QBase >
{
public:
	/** ��������
	*/
	virtual ~ControlBorderForNormalHoverPressedDisabled();

public:
	/** ���ñ�����ɫ
	@param [in] normalColor ��̬��ɫ
	@param [in] hoverColor ��ͣ��ɫ
	@param [in] pressedColor ������ɫ
	@param [in] disabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setBorderColor(const QColor& normalColor,
						const QColor& hoverColor = QColor(0, 0, 0, 0),
						const QColor& pressedColor = QColor(0, 0, 0, 0),
						const QColor& disabledColor = QColor(0, 0, 0, 0),
						bool rePaint = false);

	/** ���ýڵ㱳��ͼƬ���ͱ߿���ɫ���ô�
	@param [in] borderImgPath ����ͼƬ·�������·���б���ʹ����б��
	@param [in] borderImgStateCount ����ƽ�ּ���
	@param [in] borderImgNormal ��̬ͼƬ�������1��ʾѡ�������һ��
	@param [in] borderImgHover ��ͣͼƬ
	@param [in] borderImgPressed ����ͼƬ
	@param [in] borderImgDisabled ����ͼƬ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setBorderImage(const QString& borderImgPath,
						int32_t borderImgStateCount = 4,
						int32_t borderImgNormal = 1,
						int32_t borderImgHover = 2,
						int32_t borderImgPressed = 3,
						int32_t borderImgDisabled = 4,
						bool rePaint = false);

	/** ����������ɫ
	@param [in] textNormalColor ��̬��ɫ
	@param [in] textHoverColor ��ͣ��ɫ
	@param [in] textPressedColor ������ɫ
	@param [in] textDisabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setTextColor(const QColor& textNormalColor,
					  const QColor& textHoverColor = QColor(0, 0, 0, 0),
					  const QColor& textPressedColor = QColor(0, 0, 0, 0),
					  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
					  bool rePaint = false);
};

template <class QBase>
class ControlBorderForNormalHoverPressedDisabledAndCheck : virtual public ControlBase < QBase >
{
public:
	/** ��������
	*/
	virtual ~ControlBorderForNormalHoverPressedDisabledAndCheck();

public:
	/** ���ñ�����ɫ
	@param [in] normalColor ��ѡ�г�̬��ɫ
	@param [in] hoverColor ��ѡ����ͣ��ɫ
	@param [in] pressedColor ��ѡ�а�����ɫ
	@param [in] disabledColor ��ѡ�н�����ɫ
	@param [in] normalCkColor ѡ�г�̬��ɫ
	@param [in] hoverCkColor ѡ����ͣ��ɫ
	@param [in] pressedCkColor ѡ�а�����ɫ
	@param [in] disabledCkColor ѡ�н�����ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setBorderColor(const QColor& normalColor,
						const QColor& hoverColor = QColor(0, 0, 0, 0),
						const QColor& pressedColor = QColor(0, 0, 0, 0),
						const QColor& disabledColor = QColor(0, 0, 0, 0),
						const QColor& normalCkColor = QColor(0, 0, 0, 0),
						const QColor& hoverCkColor = QColor(0, 0, 0, 0),
						const QColor& pressedCkColor = QColor(0, 0, 0, 0),
						const QColor& disabledCkColor = QColor(0, 0, 0, 0),
						bool rePaint = false);

	/** ���ñ���ͼƬ
	@param [in] borderImgPath ����ͼƬ·�������·���б���ʹ����б��
	@param [in] borderImgStateCount ͼƬ�����и����
	@param [in] borderImgNormal ��ѡ�г�̬ͼƬ�������1��ʾ��ͼƬ����ָ�4�ݻ�8�ݣ�ѡ������ĵ�һ��
	@param [in] borderImgHover ��ѡ����ͣͼƬ
	@param [in] borderImgPressed ��ѡ�а���ͼƬ
	@param [in] borderImgDisabled ��ѡ�н���ͼƬ
	@param [in] borderImgCkNormal ѡ�г�̬ͼƬ
	@param [in] borderImgCkHover ѡ����ͣͼƬ
	@param [in] borderImgCkPressed ѡ�а���ͼƬ
	@param [in] borderImgCkDisabled ѡ�н���ͼƬ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setBorderImage(const QString& borderImgPath,
						int32_t borderImgStateCount = 8,
						int32_t borderImgNormal = 1,
						int32_t borderImgHover = 2,
						int32_t borderImgPressed = 3,
						int32_t borderImgDisabled = 4,
						int32_t borderImgCkNormal = 5,
						int32_t borderImgCkHover = 6,
						int32_t borderImgCkPressed = 7,
						int32_t borderImgCkDisabled = 8,
						bool rePaint = false);

	/** ����������ɫ
	@param [in] textNormalColor ��ѡ�г�̬��ɫ
	@param [in] textHoverColor ��ѡ����ͣ��ɫ
	@param [in] textPressedColor ��ѡ�а�����ɫ
	@param [in] textDisabledColor ��ѡ�н�����ɫ
	@param [in] textCkNormalColor ѡ�г�̬��ɫ
	@param [in] textCkHoverColor ѡ����ͣ��ɫ
	@param [in] textCkPressedColor ѡ�а�����ɫ
	@param [in] textCkDisabledColor ѡ�н�����ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setTextColor(const QColor& textNormalColor,
	 	 		 	  const QColor& textHoverColor = QColor(0, 0, 0, 0),
					  const QColor& textPressedColor = QColor(0, 0, 0, 0),
					  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
					  const QColor& textCkNormalColor = QColor(0, 0, 0, 0),
					  const QColor& textCkHoverColor = QColor(0, 0, 0, 0),
					  const QColor& textCkPressedColor = QColor(0, 0, 0, 0),
					  const QColor& textCkDisabledColor = QColor(0, 0, 0, 0),
					  bool rePaint = false);
};

template <class QBase>
class ControlItemBorderForNormal : virtual public ControlBase < QBase >
{
public:
	/** ��������
	*/
	virtual ~ControlItemBorderForNormal();

public:
	/** ���ñ߿���ɫ
	@param [in] color ��̬
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemBorderColor(const QColor& color, bool rePaint = false);

	/** ���ñ߿�ͼƬ
	@param [in] borderImgPath ͼƬ·��
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemBorderImage(const QString& borderImgPath, bool rePaint = false);

	/** ����������ɫ
	@param [in] textColor ��̬��ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemTextColor(const QColor& textColor, bool rePaint = false);
};

template <class QBase>
class ControlItemBorderForNormalHoverDisabled : virtual public ControlBase < QBase >
{
public:
	/** ��������
	*/
	virtual ~ControlItemBorderForNormalHoverDisabled();

public:
	/** ���ñ�����ɫ
	@param [in] normalColor ��̬��ɫ
	@param [in] hoverColor ��ͣ��ɫ
	@param [in] disabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemBorderColor(const QColor& normalColor,
							const QColor& hoverColor = QColor(0, 0, 0, 0),
							const QColor& disabledColor = QColor(0, 0, 0, 0),
							bool rePaint = false);

	/** ���ýڵ㱳��ͼƬ���ͱ߿���ɫ���ô�
	@param [in] borderImgPath ����ͼƬ·�������·���б���ʹ����б��
	@param [in] borderImgStateCount ����ƽ�ּ���
	@param [in] borderImgNormal ��̬ͼƬ�������1��ʾѡ�������һ��
	@param [in] borderImgHover ��ͣͼƬ
	@param [in] borderImgDisabled ����ͼƬ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemBorderImage(const QString& borderImgPath,
							int32_t borderImgStateCount = 4,
							int32_t borderImgNormal = 1,
							int32_t borderImgHover = 2,
							int32_t borderImgDisabled = 4,
							bool rePaint = false);

	/** ����������ɫ
	@param [in] textNormalColor ��̬��ɫ
	@param [in] textHoverColor ��ͣ��ɫ
	@param [in] textDisabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemTextColor(const QColor& textNormalColor,
						  const QColor& textHoverColor = QColor(0, 0, 0, 0),
						  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
						  bool rePaint = false);
};

template <class QBase>
class ControlItemBorderForNormalSelectedDisabled : virtual public ControlBase < QBase >
{
public:
	/** ��������
	*/
	virtual ~ControlItemBorderForNormalSelectedDisabled();

public:
	/** ���ñ�����ɫ
	@param [in] normalColor ��̬��ɫ
	@param [in] selectedColor ��ͣ��ɫ������list��
	@param [in] disabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemBorderColor(const QColor& normalColor,
							const QColor& selectedColor = QColor(0, 0, 0, 0),
							const QColor& disabledColor = QColor(0, 0, 0, 0),
							bool rePaint = false);

	/** ���ýڵ㱳��ͼƬ���ͱ߿���ɫ���ô�
	@param [in] borderImgPath ����ͼƬ·�������·���б���ʹ����б��
	@param [in] borderImgStateCount ����ƽ�ּ���
	@param [in] borderImgNormal ��̬ͼƬ�������1��ʾѡ�������һ��
	@param [in] borderImgSelected ��ͣͼƬ������list��
	@param [in] borderImgDisabled ����ͼƬ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemBorderImage(const QString& borderImgPath,
							int32_t borderImgStateCount = 4,
							int32_t borderImgNormal = 1,
							int32_t borderImgSelected = 2,
							int32_t borderImgDisabled = 4,
							bool rePaint = false);

	/** ����������ɫ
	@param [in] textNormalColor ��̬��ɫ
	@param [in] textSelectedColor ��ͣ��ɫ������list��
	@param [in] textDisabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemTextColor(const QColor& textNormalColor,
						  const QColor& textSelectedColor = QColor(0, 0, 0, 0),
						  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
						  bool rePaint = false);
};

template <class QBase>
class ControlItemBorderForNormalHoverPressedDisabled : virtual public ControlBase < QBase >
{
public:
	/** ��������
	*/
	virtual ~ControlItemBorderForNormalHoverPressedDisabled();

public:
	/** ���ñ�����ɫ
	@param [in] normalColor ��̬��ɫ
	@param [in] hoverColor ��ͣ��ɫ
	@param [in] pressedColor ������ɫ
	@param [in] disabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemBorderColor(const QColor& normalColor,
							const QColor& hoverColor = QColor(0, 0, 0, 0),
							const QColor& pressedColor = QColor(0, 0, 0, 0),
							const QColor& disabledColor = QColor(0, 0, 0, 0),
							bool rePaint = false);

	/** ���ýڵ㱳��ͼƬ���ͱ߿���ɫ���ô�
	@param [in] borderImgPath ����ͼƬ·�������·���б���ʹ����б��
	@param [in] borderImgStateCount ����ƽ�ּ���
	@param [in] borderImgNormal ��̬ͼƬ�������1��ʾѡ�������һ��
	@param [in] borderImgHover ��ͣͼƬ
	@param [in] borderImgPressed ����ͼƬ
	@param [in] borderImgDisabled ����ͼƬ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemBorderImage(const QString& borderImgPath,
							int32_t borderImgStateCount = 4,
							int32_t borderImgNormal = 1,
							int32_t borderImgHover = 2,
							int32_t borderImgPressed = 3,
							int32_t borderImgDisabled = 4,
							bool rePaint = false);

	/** ����������ɫ
	@param [in] textNormalColor ��̬��ɫ
	@param [in] textHoverColor ��ͣ��ɫ
	@param [in] textPressedColor ������ɫ
	@param [in] textDisabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemTextColor(const QColor& textNormalColor,
						  const QColor& textHoverColor = QColor(0, 0, 0, 0),
						  const QColor& textPressedColor = QColor(0, 0, 0, 0),
						  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
						  bool rePaint = false);
};

template <class QBase>
class ControlItemBorderForNormalHoverPressedDisabledAndCheck : virtual public ControlBase < QBase >
{
public:
	/** ��������
	*/
	virtual ~ControlItemBorderForNormalHoverPressedDisabledAndCheck();

public:
	/** ���ñ�����ɫ
	@param [in] normalColor ��ѡ�г�̬��ɫ
	@param [in] hoverColor ��ѡ����ͣ��ɫ
	@param [in] pressedColor ��ѡ�а�����ɫ
	@param [in] disabledColor ��ѡ�н�����ɫ
	@param [in] normalCkColor ѡ�г�̬��ɫ
	@param [in] hoverCkColor ѡ����ͣ��ɫ
	@param [in] pressedCkColor ѡ�а�����ɫ
	@param [in] disabledCkColor ѡ�н�����ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemBorderColor(const QColor& normalColor,
							const QColor& hoverColor = QColor(0, 0, 0, 0),
							const QColor& pressedColor = QColor(0, 0, 0, 0),
							const QColor& disabledColor = QColor(0, 0, 0, 0),
							const QColor& normalCkColor = QColor(0, 0, 0, 0),
							const QColor& hoverCkColor = QColor(0, 0, 0, 0),
							const QColor& pressedCkColor = QColor(0, 0, 0, 0),
							const QColor& disabledCkColor = QColor(0, 0, 0, 0),
							bool rePaint = false);

	/** ���ñ���ͼƬ
	@param [in] borderImgPath ����ͼƬ·�������·���б���ʹ����б��
	@param [in] borderImgStateCount ͼƬ�����и����
	@param [in] borderImgNormal ��ѡ�г�̬ͼƬ�������1��ʾ��ͼƬ����ָ�4�ݻ�8�ݣ�ѡ������ĵ�һ��
	@param [in] borderImgHover ��ѡ����ͣͼƬ
	@param [in] borderImgPressed ��ѡ�а���ͼƬ
	@param [in] borderImgDisabled ��ѡ�н���ͼƬ
	@param [in] borderImgCkNormal ѡ�г�̬ͼƬ
	@param [in] borderImgCkHover ѡ����ͣͼƬ
	@param [in] borderImgCkPressed ѡ�а���ͼƬ
	@param [in] borderImgCkDisabled ѡ�н���ͼƬ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemBorderImage(const QString& borderImgPath,
							int32_t borderImgStateCount = 8,
							int32_t borderImgNormal = 1,
							int32_t borderImgHover = 2,
							int32_t borderImgPressed = 3,
							int32_t borderImgDisabled = 4,
							int32_t borderImgCkNormal = 5,
							int32_t borderImgCkHover = 6,
							int32_t borderImgCkPressed = 7,
							int32_t borderImgCkDisabled = 8,
							bool rePaint = false);

	/** ����������ɫ
	@param [in] textNormalColor ��ѡ�г�̬��ɫ
	@param [in] textHoverColor ��ѡ����ͣ��ɫ
	@param [in] textPressedColor ��ѡ�а�����ɫ
	@param [in] textDisabledColor ��ѡ�н�����ɫ
	@param [in] textCkNormalColor ѡ�г�̬��ɫ
	@param [in] textCkHoverColor ѡ����ͣ��ɫ
	@param [in] textCkPressedColor ѡ�а�����ɫ
	@param [in] textCkDisabledColor ѡ�н�����ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemTextColor(const QColor& textNormalColor,
	 	 		 		  const QColor& textHoverColor = QColor(0, 0, 0, 0),
						  const QColor& textPressedColor = QColor(0, 0, 0, 0),
						  const QColor& textDisabledColor = QColor(0, 0, 0, 0),
						  const QColor& textCkNormalColor = QColor(0, 0, 0, 0),
						  const QColor& textCkHoverColor = QColor(0, 0, 0, 0),
						  const QColor& textCkPressedColor = QColor(0, 0, 0, 0),
						  const QColor& textCkDisabledColor = QColor(0, 0, 0, 0),
						  bool rePaint = false);
};

#include "ControlBorder.inl"
