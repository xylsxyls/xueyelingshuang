#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;
/** ��������
*/
class ReceiveTask : public CTask
{
public:
	/** ���캯��
	*/
	ReceiveTask();

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

private:
	HANDLE m_hAssgin;
	ProcessWork* m_client;
	bool m_exit;
};