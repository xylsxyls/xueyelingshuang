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
	void connect(const std::string& mapName);

    /** 向服务端发送字符串，有序收到单线程，无序收到多线程
    @param [in] buffer 字符串地址
    @param [in] length 长度
    @param [in] isOrdered 是否有序
    */
    void send(char* buffer, int32_t length, bool isOrdered = true);

protected:
	//记录数据
    SharedMemory* m_memory;
	//记录位置
	SharedMemory* m_position;
    ProcessReadWriteMutex* m_processMutex;
    HANDLE m_semaphore;
};