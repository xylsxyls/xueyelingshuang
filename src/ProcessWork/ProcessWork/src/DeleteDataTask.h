#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;
/** ɾ�������ڴ�����
*/
class DeleteDataTask : public CTask
{
public:
	/** ���캯��
	*/
	DeleteDataTask();

public:
	/** ִ������
	*/
	virtual void DoTask();

	/** ���ÿͻ���
	@param [in] client �ͻ���
	*/
	void setClient(ProcessWork* client);

	/** ����ɾ�������ڴ������ֵ
	@param [in] index ɾ�������ڴ������ֵ
	*/
	void setDeleteDataIndex(int32_t index);

protected:
	ProcessWork* m_client;
	bool m_exit;
	HANDLE m_hCreateData;
	int32_t m_index;
};