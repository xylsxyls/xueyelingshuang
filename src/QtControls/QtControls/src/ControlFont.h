#pragma once
#include "ControlBase.h"
#include "QtControlsMacro.h"

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
	void setFontSize(qint32 fontSize, bool rePaint = false);

	/** �����������
	@param [in] bold �Ƿ�Ϊ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setFontBold(bool bold, bool rePaint = false);

	/** ��������ƫ����
	@param [in] origin ����ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setTextOrigin(qint32 origin, bool rePaint = false);

	/** ���������Ҳ�ƫ����
	@param [in] origin ����ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setTextRightOrigin(qint32 origin, bool rePaint = false);
};

#include "ControlFont.inl"
