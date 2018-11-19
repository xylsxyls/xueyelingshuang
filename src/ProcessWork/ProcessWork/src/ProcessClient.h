#pragma once
#include "ProcessWorkMacro.h"
#include <string>
#include <stdint.h>
#include <windows.h>

class SharedMemory;
class ProcessReadWriteMutex;

/** �����ڴ�ͻ��ˣ��ͻ���ֻ�ܷ���
*/
class ProcessWorkAPI ProcessClient
{
public:
    ProcessClient();

public:
    /** ���ӹ����ڴ�
    @param [in] mapName �����ڴ���
    */
	bool connect(const std::string& mapName);

    /** �����˷����ַ����������յ����̣߳������յ����߳�
    @param [in] buffer �ַ�����ַ
    @param [in] length ����
    @param [in] isOrdered �Ƿ�����
	@param [in] protocolId Э���
    */
	void send(char* buffer, int32_t length, bool isOrdered = true, int32_t protocolId = 1);

protected:
	//��¼����
    SharedMemory* m_memory;
	//��ǰ������ǰȡ����ǰ���ڴ�أ���ǰȡ�ڴ��
	SharedMemory* m_position;
    ProcessReadWriteMutex* m_processMutex;
    HANDLE m_memorySemaphore;
	HANDLE m_positionBeginSemaphore;
	HANDLE m_positionEndSemaphore;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_mapName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};