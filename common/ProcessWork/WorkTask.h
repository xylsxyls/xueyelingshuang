#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "KeyPackage.h"

class ProcessWork;
/** ��������
*/
class WorkTask : public CTask
{
public:
	/** ���캯��
	*/
	WorkTask();

public:
	/** ִ������
	*/
	virtual void DoTask();

	/** ����Կ��
	@param [in] keyPackage Կ��
	*/
	void setKey(const KeyPackage& keyPackage);

	/** ���ÿͻ���
	@param [in] client �ͻ���
	*/
	void setClient(ProcessWork* client);

private:
	KeyPackage m_keyPackage;
	ProcessWork* m_client;
	HANDLE m_hDeleteEnd;
	bool m_exit;
};