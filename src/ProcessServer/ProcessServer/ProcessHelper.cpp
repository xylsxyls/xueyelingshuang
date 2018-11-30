#include "ProcessHelper.h"
#include "LogManager/LogManagerAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "SharedMemory/SharedMemoryAPI.h"

std::string ProcessHelper::positionMapName()
{
	return "ProcessWorkPosition";
}

std::string ProcessHelper::dataMapName(void* position)
{
	return ProcessHelper::dataMapName(ProcessHelper::dataIndex(position));
}

std::string ProcessHelper::dataMapName(int32_t index)
{
	return CStringManager::Format("ProcessData_%d", index);
}

std::string ProcessHelper::keyMapName(void* position)
{
	return ProcessHelper::keyMapName(ProcessHelper::keyIndex(position));
}

std::string ProcessHelper::keyMapName(int32_t index)
{
	return CStringManager::Format("ProcessKey_%d", index);
}

int32_t& ProcessHelper::dataIndex(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position == nullptr");
	}
	return *((int32_t*)position);
}

int32_t& ProcessHelper::dataPosition(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position == nullptr");
	}
	return *(((int32_t*)position) + 1);
}

int32_t& ProcessHelper::keyIndex(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position == nullptr");
	}
	return *(((int32_t*)position) + 2);
}

int32_t& ProcessHelper::keyPosition(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position == nullptr");
	}
	return *(((int32_t*)position) + 3);
}

int32_t ProcessHelper::positionLength()
{
	return sizeof(int32_t) * 7;
}

int32_t ProcessHelper::dataMemoryLength()
{
	return 4096;
}

int32_t ProcessHelper::keyMemoryLength()
{
	return sizeof(KeyPackage) * 200;
}

std::string ProcessHelper::positionMutexName()
{
	return "ProcessPositionMutex";
}

void ProcessHelper::changeToCurrentData(SharedMemory** data, SharedMemory* position)
{
	std::string dataMapName = ProcessHelper::dataMapName(position->writeWithoutLock());
	if ((*data)->mapName() != dataMapName)
	{
		delete (*data);
		(*data) = new SharedMemory(dataMapName);
	}
}

SharedMemory* ProcessHelper::createDataMemory(int32_t index)
{
	return new SharedMemory(ProcessHelper::dataMapName(index));
}

SharedMemory* ProcessHelper::createKeyMemory(int32_t index)
{
	return new SharedMemory(ProcessHelper::keyMapName(index));
}

int32_t& ProcessHelper::deleteDataIndex(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position == nullptr");
	}
	return *(((int32_t*)position) + 6);
}

bool ProcessHelper::addDataPosition(SharedMemory* data, int32_t length)
{
	int32_t newPosition = ProcessHelper::dataPosition(data->writeWithoutLock()) + length;
	if (newPosition >= ProcessHelper::dataMemoryLength() - (int32_t)(sizeof(int32_t)))
	{
		return false;
	}
	ProcessHelper::dataPosition(data->writeWithoutLock()) = newPosition;
	return true;
}

void ProcessHelper::addDataAlreadyUsed(SharedMemory* data, int32_t length)
{
	*(int32_t*)(((char*)(data->writeWithoutLock())) + ProcessHelper::dataMemoryLength() - sizeof(int32_t)) += length;
}

void ProcessHelper::changeToCurrentKey(SharedMemory** key, SharedMemory* position)
{
	std::string keyMapName = ProcessHelper::keyMapName(position->writeWithoutLock());
	if ((*key)->mapName() != keyMapName)
	{
		delete (*key);
		(*key) = new SharedMemory(keyMapName);
	}
}

bool ProcessHelper::addKeyPosition(SharedMemory* key)
{
	int32_t newPosition = ProcessHelper::keyPosition(key->writeWithoutLock()) + sizeof(KeyPackage);
	if (newPosition >= ProcessHelper::keyMemoryLength())
	{
		return false;
	}
	ProcessHelper::keyPosition(key->writeWithoutLock()) = newPosition;
	return true;
}

void ProcessHelper::writeKey(SharedMemory* key, const KeyPackage& keyPackage)
{
	void* keyPtr = key->writeWithoutLock();
	*(KeyPackage*)(((char*)keyPtr) + ProcessHelper::keyPosition(keyPtr)) = keyPackage;
}

int32_t ProcessHelper::semMaxCount()
{
	return 4000000;
}

KeyPackage ProcessHelper::readKey(SharedMemory* position, SharedMemory* key)
{
	void* keyPtr = key->writeWithoutLock();
	return *(KeyPackage*)(((char*)keyPtr) + ProcessHelper::readKeyPosition(keyPtr));
}

void ProcessHelper::readData(char*& buffer, const KeyPackage& keyPackage, SharedMemory*& data)
{
	ProcessHelper::changeToCurrentReadData(&data, keyPackage.m_index);
	::memcpy(buffer, (char*)(data->readWithoutLock()) + keyPackage.m_begin, keyPackage.m_length);
}

void ProcessHelper::reduceDataAlreadyUsed(SharedMemory* data, int32_t length)
{
	*(int32_t*)((char*)(data->writeWithoutLock()) + ProcessHelper::dataMemoryLength() - sizeof(int32_t)) -= length;
}

bool ProcessHelper::needDeleteDataMemory(SharedMemory* data, SharedMemory* position)
{
	std::vector<std::string> vecName = CStringManager::split(data->mapName(),"_");
	if (vecName.size() != 2)
	{
		LOGERROR("size != 2");
		return false;
	}
	return (*(int32_t*)((char*)(data->writeWithoutLock()) + ProcessHelper::dataMemoryLength() - sizeof(int32_t)) == 0) &&
		ProcessHelper::dataIndex(position->writeWithoutLock()) != atoi(vecName[1].c_str());
}

void ProcessHelper::writeDeleteDataIndex(SharedMemory* position, int32_t index)
{
	ProcessHelper::deleteDataIndex(position->writeWithoutLock()) = index;
}

int32_t ProcessHelper::readPID(SharedMemory* keyMemory, int32_t position)
{
	void* key = keyMemory->readWithoutLock();
	return ((KeyPackage*)((char*)key + position))->m_receivePid;
}

std::string ProcessHelper::clientReadKeyName(int32_t pid)
{
	return CStringManager::Format("ProcessClientReadKey_%d", pid);
}

std::string ProcessHelper::clientReadKeyEndName(int32_t pid)
{
	return CStringManager::Format("ProcessClientReadKeyEnd_%d", pid);
}

int32_t& ProcessHelper::readKeyIndex(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position == nullptr");
	}
	return *(((int32_t*)position) + 4);
}

int32_t& ProcessHelper::readKeyPosition(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position == nullptr");
	}
	return *(((int32_t*)position) + 5);
}

void ProcessHelper::changeToCurrentReadKey(SharedMemory** readKey, SharedMemory* position)
{
	std::string keyMapName = ProcessHelper::readKeyMapName(position->writeWithoutLock());
	if ((*readKey)->mapName() != keyMapName)
	{
		delete (*readKey);
		(*readKey) = new SharedMemory(keyMapName);
	}
}

void ProcessHelper::addReadKey(SharedMemory* position)
{
	void* positionPtr = position->writeWithoutLock();
	int32_t newPosition = ProcessHelper::readKeyPosition(positionPtr) + sizeof(KeyPackage);
	if (newPosition + int32_t(sizeof(KeyPackage)) >= ProcessHelper::keyMemoryLength())
	{
		++ProcessHelper::readKeyIndex(positionPtr);
		ProcessHelper::readKeyPosition(positionPtr) = 0;
		return;
	}
	ProcessHelper::readKeyPosition(positionPtr) = newPosition;
}

void ProcessHelper::changeToCurrentReadData(SharedMemory** readData, int32_t readIndex)
{
	std::string dataMapName = ProcessHelper::dataMapName(readIndex);
	if ((*readData)->mapName() != dataMapName)
	{
		delete (*readData);
		(*readData) = new SharedMemory(dataMapName);
	}
}

std::string ProcessHelper::readKeyMapName(void* position)
{
	return ProcessHelper::keyMapName(ProcessHelper::readKeyIndex(position));
}