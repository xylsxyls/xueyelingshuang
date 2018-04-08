#pragma once
#include "ControlBase.h"

/** ����ڵ���ع���
*/
template <class QBase>
class ControlItem : virtual public ControlBase < QBase >
{
public:
	/** ��������
	*/
	virtual ~ControlItem();

public:
	/** ���ýڵ�߿�ֶ�
	@param [in] width �߿�ֶ�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemBorderWidth(int32_t width, bool rePaint = false);

	/** ���ڵ���ΪԲ�ǣ�����Բ�ǰ뾶��ֻ�м���߿�Ż���Բ��
	@param [in] radius Բ�ǰ뾶
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemBorderRadius(int32_t radius, bool rePaint = false);

	/** ���ýڵ���
	@param [in] width �ڵ���
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemWidth(int32_t width, bool rePaint = false);

	/** ���ýڵ�߶�
	@param [in] height �ڵ�߶�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemHeight(int32_t height, bool rePaint = false);

	/** ���ýڵ��ı�ƫ����
	@param [in] origin �ڵ��ı�ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemTextOrigin(int32_t origin, bool rePaint = false);

	/** �ڵ㵽���ڵ��ĸ���߾�
	@param [in] leftOrigin ���ƫ����
	@param [in] topOrigin �ϲ�ƫ����
	@param [in] rightOrigin �Ҳ�ƫ����
	@param [in] bottomOrigin �²�ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemAroundOrigin(int32_t leftOrigin,
						     int32_t topOrigin = -1,
							 int32_t rightOrigin = -1,
							 int32_t bottomOrigin = -1,
						     bool rePaint = false);
};

#include "ControlItem.inl"
