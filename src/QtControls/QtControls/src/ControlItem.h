#pragma once
#include "ControlBase.h"
#include "QtControlsMacro.h"

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
	void setItemBorderWidth(qint32 width, bool rePaint = false);

	/** ���ڵ���ΪԲ�ǣ�����Բ�ǰ뾶��ֻ�м���߿�Ż���Բ��
	@param [in] radius Բ�ǰ뾶
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemBorderRadius(qint32 radius, bool rePaint = false);

	/** ���ýڵ���
	@param [in] width �ڵ���
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemWidth(qint32 width, bool rePaint = false);

	/** ���ýڵ�߶�
	@param [in] height �ڵ�߶�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemHeight(qint32 height, bool rePaint = false);

	/** ���ýڵ��ı�ƫ����
	@param [in] origin �ڵ��ı�ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemTextOrigin(qint32 origin, bool rePaint = false);

	/** �ڵ㵽���ڵ��ĸ���߾�
	@param [in] leftOrigin ���ƫ����
	@param [in] topOrigin �ϲ�ƫ����
	@param [in] rightOrigin �Ҳ�ƫ����
	@param [in] bottomOrigin �²�ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemAroundOrigin(qint32 leftOrigin,
						     qint32 topOrigin = -1,
							 qint32 rightOrigin = -1,
							 qint32 bottomOrigin = -1,
						     bool rePaint = false);
};

#include "ControlItem.inl"
