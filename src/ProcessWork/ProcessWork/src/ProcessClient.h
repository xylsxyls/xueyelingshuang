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
	void connect(const std::string& mapName);

    /** �����˷����ַ����������յ����̣߳������յ����߳�
    @param [in] buffer �ַ�����ַ
    @param [in] length ����
    @param [in] isOrdered �Ƿ�����
    */
    void send(char* buffer, int32_t length, bool isOrdered = true);

protected:
	//��¼����
    SharedMemory* m_memory;
	//��¼λ��
	SharedMemory* m_position;
    ProcessReadWriteMutex* m_processMutex;
    HANDLE m_semaphore;
};