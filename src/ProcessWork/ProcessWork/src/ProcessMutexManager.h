#pragma once
#include <stdint.h>
#include <map>

class ProcessReadWriteMutex;

/** ������������
*/
class ProcessMutexManager
{
protected:
	/** ���캯��
	*/
	ProcessMutexManager();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static ProcessMutexManager& instance();

	/** ��ȡλ�ý�����
	@param [in] pid ����ID
	@return ����λ�ý�����
	*/
	ProcessReadWriteMutex* positionMutex(int32_t pid);

protected:
	std::map<int32_t, ProcessReadWriteMutex*> m_positionMutex;
};