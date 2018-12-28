#include "ProcessHelper.h"
#include "LogManager/LogManagerAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "SharedMemory/SharedMemoryAPI.h"

std::string ProcessHelper::positionMapName(int32_t pid)
{
	return CStringManager::Format("ProcessWorkPosition_%d", pid);
}

std::string ProcessHelper::dataMapName(int32_t pid, int32_t index)
{
	return CStringManager::Format("ProcessWorkData_%d_%d", pid, index);
}

std::string ProcessHelper::keyMapName(int32_t pid, int32_t index)
{
	return CStringManager::Format("ProcessWorkKey_%d_%d", pid, index);
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
	return *((int32_t*)position + 1);
}

int32_t& ProcessHelper::keyIndex(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position == nullptr");
	}
	return *((int32_t*)position + 2);
}

int32_t& ProcessHelper::keyPosition(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position == nullptr");
	}
	return *((int32_t*)position + 3);
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
	return sizeof(KeyPackage) * 200 + sizeof(int32_t);
}

bool ProcessHelper::addDataPosition(void* position, int32_t length)
{
	if (position == nullptr)
	{
		LOGERROR("position == nullptr");
	}
	int32_t newPosition = ProcessHelper::dataPosition(position) + length;
	if (newPosition >= ProcessHelper::dataMemoryLength() - (int32_t)(sizeof(int32_t)))
	{
		return false;
	}
	ProcessHelper::dataPosition(position) = newPosition;
	return true;
}

void ProcessHelper::addDataAlreadyUsed(void* data, int32_t length)
{
	if (data == nullptr)
	{
		LOGERROR("data == nullptr");
	}
	*(int32_t*)(((char*)data) + ProcessHelper::dataMemoryLength() - sizeof(int32_t)) += length;
}

bool ProcessHelper::addKeyPosition(void* key)
{
	int32_t newPosition = ProcessHelper::keyPosition(key) + sizeof(KeyPackage);
	if (newPosition >= ProcessHelper::keyMemoryLength())
	{
		return false;
	}
	ProcessHelper::keyPosition(key) = newPosition;
	return true;
}

void ProcessHelper::writeKey(void* key, void* position, const KeyPackage& keyPackage)
{
	if (key == nullptr)
	{
		LOGERROR("key == nullptr");
	}
	if (position == nullptr)
	{
		LOGERROR("position == nullptr");
	}
	*(KeyPackage*)(((char*)key) + ProcessHelper::keyPosition(position) - sizeof(KeyPackage)) = keyPackage;
}

KeyPackage ProcessHelper::readKey(void* position, void* key)
{
	return *(KeyPackage*)(((char*)(key)) + ProcessHelper::readKeyPosition(position));
}

int32_t& ProcessHelper::readKeyIndex(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position == nullptr");
	}
	return *(((int32_t*)(position)) + 4);
}

int32_t& ProcessHelper::readKeyPosition(void* position)
{
	if (position == nullptr)
	{
		LOGERROR("position == nullptr");
	}
	return *(((int32_t*)(position)) + 5);
}

void ProcessHelper::readData(char*& buffer, const KeyPackage& keyPackage, void* data)
{
	::memcpy(buffer, (char*)data + keyPackage.m_begin, keyPackage.m_length);
}

bool ProcessHelper::reduceDataValid(void* data, int32_t length)
{
	*(int32_t*)((char*)data + ProcessHelper::dataMemoryLength() - sizeof(int32_t)) -= length;
	return *(int32_t*)((char*)data + ProcessHelper::dataMemoryLength() - sizeof(int32_t)) > 0;
}

int32_t ProcessHelper::semMaxCount()
{
	return 80000000;
}

std::string ProcessHelper::positionMutexName(int32_t pid)
{
	return CStringManager::Format("ProcessPositionMutex_%d", pid);
}

//std::string ProcessHelper::positionMapName()
//{
//	return "ProcessWorkPosition";
//}
//
//std::string ProcessHelper::dataMapName(int32_t pid, SharedMemory* position)
//{
//	return ProcessHelper::dataMapName(pid, ProcessHelper::dataIndex(position));
//}
//
//std::string ProcessHelper::dataMapName(int32_t pid, int32_t index)
//{
//	return CStringManager::Format("ProcessData_%d_%d", pid, index);
//}
//
//std::string ProcessHelper::keyMapName(SharedMemory* position)
//{
//	return ProcessHelper::keyMapName(ProcessHelper::keyIndex(position));
//}
//
//std::string ProcessHelper::keyMapName(int32_t index)
//{
//	return CStringManager::Format("ProcessKey_%d", index);
//}
//

//
//
//
//void ProcessHelper::changeToCurrentData(SharedMemory** data, int32_t pid, SharedMemory* position)
//{
//	std::string dataMapName = ProcessHelper::dataMapName(pid, position);
//	if ((*data)->mapName() != dataMapName)
//	{
//		delete (*data);
//		(*data) = new SharedMemory(dataMapName);
//	}
//}
//
//SharedMemory* ProcessHelper::createDataMemory(int32_t index)
//{
//	return nullptr;// new SharedMemory(ProcessHelper::dataMapName(index), ProcessHelper::dataMemoryLength());
//}
//
//SharedMemory* ProcessHelper::createKeyMemory(int32_t index)
//{
//	return new SharedMemory(ProcessHelper::keyMapName(index), ProcessHelper::keyMemoryLength());
//}
//
//int32_t& ProcessHelper::deleteDataIndex(SharedMemory* position)
//{
//	if (position == nullptr)
//	{
//		LOGERROR("position == nullptr");
//	}
//	return *(((int32_t*)(position->writeWithoutLock())) + 6);
//}
//

//
//
//
//void ProcessHelper::changeToCurrentKey(SharedMemory** key, SharedMemory* position)
//{
//	std::string keyMapName = ProcessHelper::keyMapName(position);
//	if ((*key)->mapName() != keyMapName)
//	{
//		delete (*key);
//		(*key) = new SharedMemory(keyMapName);
//	}
//}
//
//
//
//
//
//
//
//
//

//
//
//
//bool ProcessHelper::needDeleteDataMemory(SharedMemory* data, SharedMemory* position)
//{
//	std::vector<std::string> vecName = CStringManager::split(data->mapName(),"_");
//	if (vecName.size() != 2)
//	{
//		LOGERROR("size != 2");
//		return false;
//	}
//	return (*(int32_t*)((char*)(data->writeWithoutLock()) + ProcessHelper::dataMemoryLength() - sizeof(int32_t)) == 0) &&
//		ProcessHelper::dataIndex(position) != atoi(vecName[1].c_str());
//}
//
//void ProcessHelper::writeDeleteDataIndex(SharedMemory* position, int32_t index)
//{
//	ProcessHelper::deleteDataIndex(position) = index;
//}
//
//int32_t ProcessHelper::readPID(SharedMemory* keyMemory, int32_t position)
//{
//	return ((KeyPackage*)((char*)(keyMemory->readWithoutLock()) + position))->m_receivePid;
//}
//
//std::string ProcessHelper::clientReadKeyName(int32_t pid)
//{
//	return CStringManager::Format("ProcessClientReadKey_%d", pid);
//}
//
//std::string ProcessHelper::clientReadKeyEndName(int32_t pid)
//{
//	return CStringManager::Format("ProcessClientReadKeyEnd_%d", pid);
//}
//

//
//void ProcessHelper::changeToCurrentReadKey(SharedMemory** readKey, SharedMemory* position)
//{
//	std::string keyMapName = ProcessHelper::readKeyMapName(position);
//	if ((*readKey)->mapName() != keyMapName)
//	{
//		delete (*readKey);
//		(*readKey) = new SharedMemory(keyMapName);
//	}
//}
//
//bool ProcessHelper::addReadKey(SharedMemory* position)
//{
//	int32_t newPosition = ProcessHelper::readKeyPosition(position) + sizeof(KeyPackage);
//	if (newPosition + int32_t(sizeof(KeyPackage)) >= ProcessHelper::keyMemoryLength())
//	{
//		++ProcessHelper::readKeyIndex(position);
//		ProcessHelper::readKeyPosition(position) = 0;
//		return false;
//	}
//	ProcessHelper::readKeyPosition(position) = newPosition;
//	return true;
//}
//
//void ProcessHelper::changeToCurrentReadData(SharedMemory** readData, int32_t readIndex)
//{
//	//std::string dataMapName = ProcessHelper::dataMapName(readIndex);
//	//if ((*readData)->mapName() != dataMapName)
//	//{
//	//	delete (*readData);
//	//	(*readData) = new SharedMemory(dataMapName);
//	//}
//}
//
//std::string ProcessHelper::readKeyMapName(SharedMemory* position)
//{
//	return ProcessHelper::keyMapName(ProcessHelper::readKeyIndex(position));
//}
//
//void ProcessHelper::clearCalcData(SharedMemory* data)
//{
//	*(int32_t*)((char*)(data->writeWithoutLock()) + ProcessHelper::dataMemoryLength() - sizeof(int32_t)) = 0;
//}
//
//void ProcessHelper::clearCalcKey(SharedMemory* key)
//{
//	*(int32_t*)((char*)(key->writeWithoutLock()) + ProcessHelper::keyMemoryLength() - sizeof(int32_t)) = 0;
//}
//
//void ProcessHelper::updateDataPosition(SharedMemory* position)
//{
//	ProcessHelper::dataPosition(position) = 0;
//	++ProcessHelper::dataIndex(position);
//}
//
//void ProcessHelper::updateKeyPosition(SharedMemory* position)
//{
//	ProcessHelper::keyPosition(position) = 0;
//	++ProcessHelper::keyIndex(position);
//}


