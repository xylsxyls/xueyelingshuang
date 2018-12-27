#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;
/** ����Կ���ڴ�����
*/
class CreateKeyTask : public CTask
{
public:
	/** ���캯��
	*/
	CreateKeyTask();

public:
	/** ִ������
	*/
	virtual void DoTask();

	/** �ж�����
	*/
	virtual void StopTask();

	/** ���ÿͻ���
	@param [in] client �ͻ���
	*/
	void setClient(ProcessWork* client);

protected:
	ProcessWork* m_client;
	bool m_exit;
	HANDLE m_hCreateKey;
};