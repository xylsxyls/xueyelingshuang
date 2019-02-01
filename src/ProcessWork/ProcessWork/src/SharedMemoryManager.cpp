#include "SharedMemoryManager.h"
#include "ProcessHelper.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CSystem/CSystemAPI.h"

SharedMemoryManager::SharedMemoryManager():
m_position(nullptr),
m_pid(0),
m_readKey(nullptr),
m_readIndex(0),
m_readPosition(0)
{
	m_pid = CSystem::processPid();
}

SharedMemoryManager& SharedMemoryManager::instance()
{
	static SharedMemoryManager s_sharedMemoryManager;
	return s_sharedMemoryManager;
}

void SharedMemoryManager::uninit()
{
	{
		std::unique_lock<std::mutex> mu(m_sendMutex);
		for (auto itSend = m_send.begin(); itSend != m_send.end(); ++itSend)
		{
			if (itSend->second.m_data != nullptr)
			{
				delete itSend->second.m_data;
			}
			if (itSend->second.m_key != nullptr)
			{
				delete itSend->second.m_key;
			}
			if (itSend->second.m_position != nullptr)
			{
				delete itSend->second.m_position;
			}
		}
		m_send.clear();
		m_send.swap(std::map<int32_t, MemoryPackage>());
	}
	
	{
		std::unique_lock<std::mutex> mu(m_readMutex);
		for (auto itData = m_dataMap.begin(); itData != m_dataMap.end(); ++itData)
		{
			if (itData->second != nullptr)
			{
				delete itData->second;
			}
		}
		m_dataMap.clear();
		m_dataMap.swap(std::map<int32_t, SharedMemory*>());
	}
	
	SharedMemory* key = nullptr;
	while (m_keyList.pop(&key))
	{
		if (key != nullptr)
		{
			delete key;
		}
	}
	delete m_position;
}

bool SharedMemoryManager::addDataPosition(int32_t pid, int32_t length)
{
	if (!createSendMemory(pid))
	{
		return false;
	}
	void* position = getPositionMemory(pid);
	if (position == nullptr)
	{
		return false;
	}
	return ProcessHelper::addDataPosition(position, length);
}

SharedMemory* SharedMemoryManager::getCurrentSendData(int32_t pid)
{
	if (!createSendMemory(pid))
	{
		return nullptr;
	}
	std::unique_lock<std::mutex> mu(m_sendMutex);
	auto itSend = m_send.find(pid);
	if (itSend == m_send.end())
	{
		return nullptr;
	}
	return itSend->second.m_data;
}

bool SharedMemoryManager::addDataAlreadyUsed(int32_t pid, int32_t length)
{
	if (!createSendMemory(pid))
	{
		return false;
	}
	void* data = getDataMemory(pid);
	if (data == nullptr)
	{
		return false;
	}
	ProcessHelper::addDataAlreadyUsed(data, length);
	return true;
}

int32_t SharedMemoryManager::sendDataIndex(int32_t pid)
{
	if (!createSendMemory(pid))
	{
		return 0;
	}
	void* position = getPositionMemory(pid);
	if (position == nullptr)
	{
		return 0;
	}
	return ProcessHelper::dataIndex(position);
}

int32_t SharedMemoryManager::sendDataPosition(int32_t pid)
{
	if (!createSendMemory(pid))
	{
		return 0;
	}
	void* position = getPositionMemory(pid);
	if (position == nullptr)
	{
		return 0;
	}
	return ProcessHelper::dataPosition(position);
}

bool SharedMemoryManager::addKeyPosition(int32_t pid)
{
	if (!createSendMemory(pid))
	{
		return false;
	}
	void* position = getPositionMemory(pid);
	if (position == nullptr)
	{
		return false;
	}
	return ProcessHelper::addKeyPosition(position);
}

bool SharedMemoryManager::raiseSendKey(int32_t pid)
{
	if (!createSendMemory(pid))
	{
		return false;
	}
	void* position = getPositionMemory(pid);
	if (position == nullptr)
	{
		return false;
	}
	++ProcessHelper::keyIndex(position);
	ProcessHelper::keyPosition(position) = sizeof(KeyPackage);
	return true;
}

bool SharedMemoryManager::raiseSendData(int32_t pid, int32_t length)
{
	if (!createSendMemory(pid))
	{
		return false;
	}
	void* position = getPositionMemory(pid);
	if (position == nullptr)
	{
		return false;
	}
	++ProcessHelper::dataIndex(position);
	ProcessHelper::dataPosition(position) = length;
	return true;
}

bool SharedMemoryManager::writeKey(int32_t pid, const KeyPackage& keyPackage)
{
	if (!createSendMemory(pid))
	{
		return false;
	}
	void* position = getPositionMemory(pid);
	if (position == nullptr)
	{
		return false;
	}
	void* key = getKeyMemory(pid);
	if (key == nullptr)
	{
		return false;
	}
	ProcessHelper::writeKey(key, position, keyPackage);
	return true;
}

void SharedMemoryManager::initReceiveMemory()
{
	if (m_position == nullptr)
	{
		m_position = new SharedMemory(ProcessHelper::positionMapName(m_pid), ProcessHelper::positionLength());
	}
	m_position->clear();
	if (m_readKey != nullptr)
	{
		delete m_readKey;
		m_readKey = nullptr;
	}
	m_readKey = new SharedMemory(ProcessHelper::keyMapName(m_pid, 0), ProcessHelper::keyMemoryLength());
	SharedMemory* memory = nullptr;
	while (m_keyList.pop(&memory))
	{
		delete memory;
	}

	std::unique_lock<std::mutex> mu(m_readMutex);
	for (auto itData = m_dataMap.begin(); itData != m_dataMap.end(); ++itData)
	{
		delete itData->second;
	}
	m_dataMap.clear();
	m_dataMap.swap(std::map<int32_t, SharedMemory*>());
	SharedMemory* dataMemory = new SharedMemory(ProcessHelper::dataMapName(m_pid, 0), ProcessHelper::dataMemoryLength());
	void* data = dataMemory->writeWithoutLock();
	if (data == nullptr)
	{
		return;
	}
	*(int32_t*)((char*)data + ProcessHelper::dataMemoryLength() - sizeof(int32_t)) = 0;
	m_dataMap[0] = dataMemory;
}

bool SharedMemoryManager::createData()
{
	if (m_position == nullptr)
	{
		return false;
	}
	void* position = m_position->writeWithoutLock();
	if (position == nullptr)
	{
		return false;
	}
	int32_t newIndex = ProcessHelper::dataIndex(position) + 1;
	SharedMemory* dataMemory = new SharedMemory(ProcessHelper::dataMapName(m_pid, newIndex), ProcessHelper::dataMemoryLength());
	void* data = dataMemory->writeWithoutLock();
	if (data == nullptr)
	{
		return false;
	}
	*(int32_t*)((char*)data + ProcessHelper::dataMemoryLength() - sizeof(int32_t)) = 0;
	std::unique_lock<std::mutex> mu(m_readMutex);
	m_dataMap[newIndex] = dataMemory;
	return true;
}

bool SharedMemoryManager::createKey()
{
	if (m_position == nullptr)
	{
		return false;
	}
	void* position = m_position->writeWithoutLock();
	if (position == nullptr)
	{
		return false;
	}
	if (m_readKey == nullptr)
	{
		m_readKey = new SharedMemory(ProcessHelper::keyMapName(m_pid, ProcessHelper::keyIndex(position) + 1), ProcessHelper::keyMemoryLength());
		void* key = m_readKey->writeWithoutLock();
		if (key == nullptr)
		{
			return false;
		}
		return true;
	}
	SharedMemory* keyInList = new SharedMemory(ProcessHelper::keyMapName(m_pid, ProcessHelper::keyIndex(position) + 1), ProcessHelper::keyMemoryLength());
	if (keyInList == nullptr)
	{
		return false;
	}
	void* keyInListPtr = keyInList->writeWithoutLock();
	if (keyInListPtr == nullptr)
	{
		return false;
	}
	m_keyList.push(keyInList);
	return true;
}

bool SharedMemoryManager::readKey(KeyPackage& keyPackage, SharedMemory*& deleteMemory)
{
	deleteMemory = nullptr;
	if (m_readPosition + int32_t(sizeof(KeyPackage)) > ProcessHelper::keyMemoryLength())
	{
		++m_readIndex;
		m_readPosition = 0;
		deleteMemory = m_readKey;
		if (!m_keyList.pop(&m_readKey))
		{
			return false;
		}
	}
	void* key = m_readKey->writeWithoutLock();
	if (key == nullptr)
	{
		return false;
	}
	keyPackage = *(KeyPackage*)((char*)key + m_readPosition);
	m_readPosition += int32_t(sizeof(KeyPackage));
	return true;
}

void SharedMemoryManager::readData(char*& buffer, const KeyPackage& keyPackage)
{
	std::unique_lock<std::mutex> mu(m_readMutex);
	auto itData = m_dataMap.find(keyPackage.m_index);
	if (itData == m_dataMap.end())
	{
		return;
	}
	void* data = itData->second->writeWithoutLock();
	data = itData->second->writeWithoutLock();
	if (data == nullptr)
	{
		return;
	}
	ProcessHelper::readData(buffer, keyPackage, data);
}

bool SharedMemoryManager::reduceDataValid(int32_t index, int32_t length)
{
	std::unique_lock<std::mutex> mu(m_readMutex);
	auto itData = m_dataMap.find(index);
	if (itData == m_dataMap.end())
	{
		return false;
	}
	void* data = itData->second->writeWithoutLock();
	if (data == nullptr)
	{
		return false;
	}
	int32_t lastDataIndex = index;
	if (m_dataMap.size() >= 2)
	{
		lastDataIndex = m_dataMap.rbegin()->first;
	}
	return ProcessHelper::reduceDataValid(data, length, index, lastDataIndex);
}

//bool SharedMemoryManager::addReadKeyPosition(SharedMemory*& deleteMemory)
//{
//	if (m_readKey == nullptr)
//	{
//		return false;
//	}
//	deleteMemory = nullptr;
//	void* position = m_position->writeWithoutLock();
//	if (position == nullptr)
//	{
//		return false;
//	}
//	if (!ProcessHelper::addReadKeyPosition(position))
//	{
//		deleteMemory = m_readKey;
//		m_keyList.pop(&m_readKey);
//		return false;
//	}
//	return true;
//}

void SharedMemoryManager::deleteData(int32_t index)
{
	std::unique_lock<std::mutex> mu(m_readMutex);
	auto itData = m_dataMap.find(index);
	if (itData == m_dataMap.end())
	{
		return;
	}
	if (itData->second != nullptr)
	{
		delete itData->second;
		m_dataMap.erase(itData);
	}
}

void SharedMemoryManager::deleteKey(SharedMemory* deleteMemory)
{
	delete deleteMemory;
}

bool SharedMemoryManager::createSendMemory(int32_t pid)
{
	std::unique_lock<std::mutex> mu(m_sendMutex);
	auto itSend = m_send.find(pid);
	if (itSend == m_send.end())
	{
		MemoryPackage memoryPackage;
		memoryPackage.m_position = new SharedMemory(ProcessHelper::positionMapName(pid));
		void* position = memoryPackage.m_position->writeWithoutLock();
		if (position == nullptr)
		{
			return false;
		}
		memoryPackage.m_key = new SharedMemory(ProcessHelper::keyMapName(pid, ProcessHelper::keyIndex(position)));
		memoryPackage.m_data = new SharedMemory(ProcessHelper::dataMapName(pid, ProcessHelper::dataIndex(position)));
		m_send[pid] = memoryPackage;
		return true;
	}
	MemoryPackage& memoryPackage = itSend->second;
	void* position = memoryPackage.m_position->writeWithoutLock();
	if (position == nullptr)
	{
		return false;
	}
	std::string lastKeyMapName = ProcessHelper::keyMapName(pid, ProcessHelper::keyIndex(position));
	if (memoryPackage.m_key->mapName() != lastKeyMapName)
	{
		delete memoryPackage.m_key;
		memoryPackage.m_key = new SharedMemory(lastKeyMapName);
	}
	std::string lastDataMapName = ProcessHelper::dataMapName(pid, ProcessHelper::dataIndex(position));
	if (memoryPackage.m_data->mapName() != lastDataMapName)
	{
		delete memoryPackage.m_data;
		memoryPackage.m_data = new SharedMemory(lastDataMapName);
	}
	return true;
}

void* SharedMemoryManager::getPositionMemory(int32_t pid)
{
	std::unique_lock<std::mutex> mu(m_sendMutex);
	auto itSend = m_send.find(pid);
	if (itSend == m_send.end())
	{
		return nullptr;
	}
	return itSend->second.m_position->writeWithoutLock();
}

void* SharedMemoryManager::getDataMemory(int32_t pid)
{
	std::unique_lock<std::mutex> mu(m_sendMutex);
	auto itSend = m_send.find(pid);
	if (itSend == m_send.end())
	{
		return nullptr;
	}
	return itSend->second.m_data->writeWithoutLock();
}

void* SharedMemoryManager::getKeyMemory(int32_t pid)
{
	std::unique_lock<std::mutex> mu(m_sendMutex);
	auto itSend = m_send.find(pid);
	if (itSend == m_send.end())
	{
		return nullptr;
	}
	return itSend->second.m_key->writeWithoutLock();
}