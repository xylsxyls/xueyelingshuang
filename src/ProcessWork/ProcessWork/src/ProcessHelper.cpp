#include "ProcessHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "SharedMemory/SharedMemoryAPI.h"

int32_t& ProcessHelper::sendPosition(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position = nullptr");
	}
	return *((int32_t*)((char*)position + sizeof(int32_t) * 0));
}

int32_t& ProcessHelper::receivePosition(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position = nullptr");
	}
	return *((int32_t*)((char*)position + sizeof(int32_t) * 1));
}

int32_t& ProcessHelper::sendMemoryIndex(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position = nullptr");
	}
	return *((int32_t*)((char*)position + sizeof(int32_t) * 2));
}

int32_t& ProcessHelper::receiveMemoryIndex(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position = nullptr");
	}
	return *((int32_t*)((char*)position + sizeof(int32_t) * 3));
}

void ProcessHelper::resetSendPosition(void* position)
{
	ProcessHelper::sendPosition(position) = 0;
}

bool ProcessHelper::addSendPosition(void* position, int32_t addNum, int32_t step)
{
	if (position == nullptr)
	{
		LOGERROR("position = nullptr");
	}
	int32_t& sendPosition = ProcessHelper::sendPosition(position);
	if (sendPosition + addNum > step)
	{
		return false;
	}
	sendPosition += addNum;
	return true;
}

void ProcessHelper::addReceivePosition(void* position, int32_t addNum)
{
	if (position == nullptr)
	{
		LOGERROR("position = nullptr");
	}
	ProcessHelper::receivePosition(position) += addNum;
}

void ProcessHelper::addSendMemoryIndex(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position = nullptr");
	}
	++ProcessHelper::sendMemoryIndex(position);
}

void ProcessHelper::addReceiveMemoryIndex(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position = nullptr");
	}
	++ProcessHelper::receiveMemoryIndex(position);
}

std::string ProcessHelper::positionMapName(const std::string& mapName)
{
	if (mapName == "")
	{
		LOGERROR("mapName = \"\"");
	}
	return mapName + "Position";
}

std::string ProcessHelper::sendMemoryMapName(const std::string& mapName, void* position)
{
	if (mapName == "")
	{
		LOGERROR("mapName = \"\"");
	}
	if (position == nullptr)
	{
		LOGERROR("position = nullptr");
	}
	return CStringManager::Format("%s_%d", mapName.c_str(), ProcessHelper::sendMemoryIndex(position));
}

std::string ProcessHelper::sendMemoryMapName(const std::string& mapName, int32_t sendMemoryIndex)
{
	if (mapName == "")
	{
		LOGERROR("mapName = \"\"");
	}
	return CStringManager::Format("%s_%d", mapName.c_str(), sendMemoryIndex);
}

std::string ProcessHelper::receiveMemoryMapName(const std::string& mapName, void* position)
{
	if (mapName == "")
	{
		LOGERROR("mapName = \"\"");
	}
	if (position == nullptr)
	{
		LOGERROR("position = nullptr");
	}
	return CStringManager::Format("%s_%d", mapName.c_str(), ProcessHelper::receiveMemoryIndex(position));
}

std::string ProcessHelper::receiveMemoryMapName(const std::string& mapName, int32_t receiveMemoryIndex)
{
	if (mapName == "")
	{
		LOGERROR("mapName = \"\"");
	}
	if (receiveMemoryIndex == 0)
	{
		LOGERROR("receiveMemoryIndex = 0");
	}
	return CStringManager::Format("%s_%d", mapName.c_str(), receiveMemoryIndex);
}

void ProcessHelper::changeToCurrentSendMemory(SharedMemory** memory, const std::string& mapName, void* position)
{
	std::string sendMemoryMapName = ProcessHelper::sendMemoryMapName(mapName, position);
	if ((*memory)->mapName() != sendMemoryMapName)
	{
		LOGINFO("check memory old = %s, new = %s", (*memory)->mapName(), sendMemoryMapName);
		delete (*memory);
		(*memory) = new SharedMemory(sendMemoryMapName);
	}
}

void ProcessHelper::writeMemory(void* memory, int32_t sendPosition, char* buffer, int32_t length, int32_t protocolId)
{
	if (memory == nullptr)
	{
		LOGERROR("memory = nullptr");
	}
	void* currentMemory = (char*)memory + sendPosition;
	*((int32_t*)currentMemory) = length;
	*((int32_t*)((char*)currentMemory + sizeof(int32_t))) = protocolId;
	::memcpy((char*)currentMemory + sizeof(int32_t) * 2, buffer, length);
}

bool ProcessHelper::readMemory(void* memory, int32_t receivePosition, char** buffer, int32_t* length, int32_t* protocolId)
{
	if (memory == nullptr)
	{
		LOGERROR("memory = nullptr");
	}
	void* currentMemory = (char*)memory + receivePosition;
	*length = *((int32_t*)currentMemory);
	*protocolId = *((int32_t*)((char*)currentMemory + sizeof(int32_t)));
	if (*length == 0 && *protocolId == 0)
	{
		return false;
	}
	*buffer = new char[*length + 1];
	(*buffer)[*length] = 0;
	::memcpy(*buffer, (char*)currentMemory + sizeof(int32_t) * 2, *length);
	return true;
}

void ProcessHelper::writeEndMemory(void* memory, int32_t sendPosition)
{
	if (memory == nullptr)
	{
		LOGERROR("memory = nullptr");
	}
	void* currentMemory = (char*)memory + sendPosition;
	::memset(currentMemory, 0, sizeof(int32_t) * 2);
}