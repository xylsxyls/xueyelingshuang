#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;
class SharedMemory;
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

	/** ����Ҫɾ�����ڴ�ָ��
	@param [in] deleteMemory Ҫɾ�����ڴ�ָ��
	*/
	void setDeleteMemory(SharedMemory* deleteMemory);

protected:
	ProcessWork* m_client;
	bool m_exit;
	HANDLE m_hCreateData;
	SharedMemory* m_deleteMemory;
};