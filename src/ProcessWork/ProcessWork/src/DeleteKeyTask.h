#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;
/** ɾ��Կ���ڴ�����
*/
class DeleteKeyTask : public CTask
{
public:
	/** ���캯��
	*/
	DeleteKeyTask();

public:
	/** ִ������
	*/
	virtual void DoTask();

	/** ���ÿͻ���
	@param [in] client �ͻ���
	*/
	void setClient(ProcessWork* client);

protected:
	ProcessWork* m_client;
	bool m_exit;
	HANDLE m_hCreateData;
};