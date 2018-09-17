#pragma once
#include "ControlBase.h"
#include "ControlsMacro.h"

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
	void setBorderRadius(qint32 radius, bool rePaint = false);

	/** ���ñ߿�ֶ�
	@param [in] width �߿�ֶ�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setBorderWidth(qint32 width, bool rePaint = false);
};

#include "ControlSelf.inl"
