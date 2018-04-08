#pragma once
#include "ControlBase.h"

/** ����ؼ�����������ع���
*/
template <class QBase>
class ControlSelf : virtual public ControlBase < QBase >
{
public:
	/** ��������
	*/
	virtual ~ControlSelf();

public:
	/** ���������ΪԲ�ǣ�����Բ�ǰ뾶��ֻ�м���߿�Ż���Բ��
	@param [in] radius Բ�ǰ뾶
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setBorderRadius(int32_t radius, bool rePaint = false);

	/** ���ñ߿�ֶ�
	@param [in] width �߿�ֶ�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setBorderWidth(int32_t width, bool rePaint = false);
};

#include "ControlSelf.inl"
