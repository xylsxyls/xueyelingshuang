#pragma once
#include <stdint.h>
#include <string>
#include "WorkParam.h"

class SharedMemory;

class ProcessHelper
{
public:
	//发送端内存写入位置
	static int32_t& sendPosition(void* position);
	//接收端内存取出位置
	static int32_t& receivePosition(void* position);
	//发送端内存池位置
	static int32_t& sendMemoryIndex(void* position);
	//接收端内存池位置
	static int32_t& receiveMemoryIndex(void* position);

	static void resetSendPosition(void* position);

	static bool addSendPosition(void* position, int32_t addNum, int32_t step = WorkParam::STEP_LENGTH);
	static void addReceivePosition(void* position, int32_t addNum);
	static void addSendMemoryIndex(void* position);
	static void addReceiveMemoryIndex(void* position);

	static std::string positionMapName(const std::string& mapName);
	static std::string sendMemoryMapName(const std::string& mapName, void* position);
	static std::string sendMemoryMapName(const std::string& mapName, int32_t sendMemoryIndex);
	static std::string receiveMemoryMapName(const std::string& mapName, void* position);
	static std::string receiveMemoryMapName(const std::string& mapName, int32_t receiveMemoryIndex);

	static void changeToCurrentSendMemory(SharedMemory** memory, const std::string& mapName, void* position);

	static void writeMemory(void* memory, int32_t sendPosition, char* buffer, int32_t length, int32_t protocolId);
	static bool readMemory(void* memory, int32_t receivePosition, char** buffer, int32_t* length, int32_t* protocolId);

	static void writeEndMemory(void* memory, int32_t sendPosition);
};