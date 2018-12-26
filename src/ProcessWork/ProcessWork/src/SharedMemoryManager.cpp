#include "SharedMemoryManager.h"

SharedMemoryManager::SharedMemoryManager()
{

}

SharedMemoryManager& SharedMemoryManager::instance()
{
	static SharedMemoryManager s_sharedMemoryManager;
	return s_sharedMemoryManager;
}

bool SharedMemoryManager::addDataPosition(int32_t pid, int32_t length)
{
	return false;
}

SharedMemory* SharedMemoryManager::getCurrentSendData(int32_t pid)
{
	return nullptr;
}

void SharedMemoryManager::addDataAlreadyUsed(int32_t pid, int32_t length)
{

}

int32_t& SharedMemoryManager::sendDataIndex(int32_t pid)
{
	int32_t* result = new int32_t;
	return *result;
}

int32_t& SharedMemoryManager::sendDataPosition(int32_t pid)
{
	int32_t* result = new int32_t;
	return *result;
}

bool SharedMemoryManager::addKeyPosition(int32_t pid)
{
	return false;
}

void SharedMemoryManager::writeKey(int32_t pid, const KeyPackage& keyPackage)
{

}

void SharedMemoryManager::createData()
{

}

void SharedMemoryManager::createKey()
{

}

KeyPackage SharedMemoryManager::readKey()
{
	return KeyPackage();
}

void SharedMemoryManager::readData(char*& buffer, const KeyPackage& keyPackage)
{

}

bool SharedMemoryManager::reduceDataValid(int32_t length)
{
	return false;
}

bool SharedMemoryManager::addReadKeyPosition()
{
	return false;
}

void SharedMemoryManager::deleteData(int32_t index)
{

}

void SharedMemoryManager::deleteKey()
{

}