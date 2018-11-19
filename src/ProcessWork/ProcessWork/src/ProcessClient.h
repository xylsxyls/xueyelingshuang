#pragma once
#include "ProcessWorkMacro.h"
#include <string>
#include <stdint.h>
#include <windows.h>

class SharedMemory;
class ProcessReadWriteMutex;

/** 共享内存客户端，客户端只能发送
*/
class ProcessWorkAPI ProcessClient
{
public:
    ProcessClient();

public:
    /** 连接共享内存
    @param [in] mapName 共享内存名
    */
	bool connect(const std::string& mapName);

    /** 向服务端发送字符串，有序收到单线程，无序收到多线程
    @param [in] buffer 字符串地址
    @param [in] length 长度
    @param [in] isOrdered 是否有序
	@param [in] protocolId 协议号
    */
	void send(char* buffer, int32_t length, bool isOrdered = true, int32_t protocolId = 1);

protected:
	//记录数据
    SharedMemory* m_memory;
	//当前发，当前取，当前发内存池，当前取内存池
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