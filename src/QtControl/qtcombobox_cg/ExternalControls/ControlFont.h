#pragma once
#include "ControlBase.h"

/** ����������ع���
*/
template <class QBase>
class ControlFont : virtual public ControlBase < QBase >
{
public:
	/** ��������
	*/
	virtual ~ControlFont();

public:
	/** ��������
	@param [in] fontName ������
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setFontFace(const QString& fontName, bool rePaint = false);

	/** ���������С
	@param [in] fontSize �����С
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setFontSize(int32_t fontSize, bool rePaint = false);

	/** �����������
	@param [in] bold �Ƿ�Ϊ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setFontBold(bool bold, bool rePaint = false);

	/** ��������ƫ����
	@param [in] origin ����ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setTextOrigin(int32_t origin, bool rePaint = false);

	/** ���������Ҳ�ƫ����
	@param [in] origin ����ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setTextRightOrigin(int32_t origin, bool rePaint = false);
};

#include "ControlFont.inl"
