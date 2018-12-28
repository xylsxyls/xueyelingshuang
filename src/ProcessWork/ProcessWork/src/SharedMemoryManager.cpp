#include "SharedMemoryManager.h"
#include "ProcessHelper.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CSystem/CSystemAPI.h"

SharedMemoryManager::SharedMemoryManager():
m_position(nullptr),
m_pid(0),
m_readKey(nullptr)
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
	for (auto itData = m_dataMap.begin(); itData != m_dataMap.end(); ++itData)
	{
		if (itData->second != nullptr)
		{
			delete itData->second;
		}
	}
	m_dataMap.clear();
	m_dataMap.swap(std::map<int32_t, SharedMemory*>());
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

void SharedMemoryManager::createData()
{
	if (m_position == nullptr)
	{
		m_position = new SharedMemory(ProcessHelper::positionMapName(m_pid));
	}
	void* position = m_position->writeWithoutLock();
	if (position == nullptr)
	{
		return;
	}
	int32_t newIndex = ProcessHelper::dataIndex(position) + 1;
	m_dataMap[newIndex] = new SharedMemory(ProcessHelper::dataMapName(m_pid, newIndex), ProcessHelper::dataMemoryLength());
}

void SharedMemoryManager::createKey()
{
	if (m_position == nullptr)
	{
		m_position = new SharedMemory(ProcessHelper::positionMapName(m_pid));
	}
	void* position = m_position->writeWithoutLock();
	if (position == nullptr)
	{
		return;
	}
	if (m_readKey == nullptr)
	{
		m_readKey = new SharedMemory(ProcessHelper::keyMapName(m_pid, ProcessHelper::keyIndex(position) + 1), ProcessHelper::keyMemoryLength());
		return;
	}
	m_keyList.push(new SharedMemory(ProcessHelper::keyMapName(m_pid, ProcessHelper::keyIndex(position) + 1), ProcessHelper::keyMemoryLength()));
}

KeyPackage SharedMemoryManager::readKey()
{
	void* position = m_position->writeWithoutLock();
	if (position == nullptr)
	{
		return KeyPackage();
	}
	void* key = m_readKey->writeWithoutLock();
	if (key == nullptr)
	{
		return KeyPackage();
	}
	return ProcessHelper::readKey(position, key);
}

void SharedMemoryManager::readData(char*& buffer, const KeyPackage& keyPackage)
{
	auto itData = m_dataMap.find(keyPackage.m_index);
	if (itData == m_dataMap.end())
	{
		return;
	}
	void* data = itData->second->writeWithoutLock();
	if (data == nullptr)
	{
		return;
	}
	ProcessHelper::readData(buffer, keyPackage, data);
}

bool SharedMemoryManager::reduceDataValid(int32_t index, int32_t length)
{
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
	return ProcessHelper::reduceDataValid(data, length);
}

bool SharedMemoryManager::addReadKeyPosition()
{
	void* key = m_readKey->writeWithoutLock();
	if (key == nullptr)
	{
		return false;
	}
	return ProcessHelper::addKeyPosition(key);
}

void SharedMemoryManager::deleteData(int32_t index)
{
	auto itData = m_dataMap.find(index);
	if (itData == m_dataMap.end())
	{
		return;
	}
	if (itData->second != nullptr)
	{
		delete itData->second;
	}
}

void SharedMemoryManager::deleteKey()
{
	if (m_readKey != nullptr)
	{
		delete m_readKey;
		m_readKey = nullptr;
	}
	m_keyList.pop(&m_readKey);
}

bool SharedMemoryManager::createSendMemory(int32_t pid)
{
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
	auto itSend = m_send.find(pid);
	if (itSend == m_send.end())
	{
		return nullptr;
	}
	return itSend->second.m_position->writeWithoutLock();
}

void* SharedMemoryManager::getDataMemory(int32_t pid)
{
	auto itSend = m_send.find(pid);
	if (itSend == m_send.end())
	{
		return nullptr;
	}
	return itSend->second.m_data->writeWithoutLock();
}

void* SharedMemoryManager::getKeyMemory(int32_t pid)
{
	auto itSend = m_send.find(pid);
	if (itSend == m_send.end())
	{
		return nullptr;
	}
	return itSend->second.m_key->writeWithoutLock();
}
