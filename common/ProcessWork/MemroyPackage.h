#pragma once
#include <stdint.h>

class SharedMemory;

struct MemoryPackage
{
	SharedMemory* m_position;
	SharedMemory* m_key;
	SharedMemory* m_data;

	MemoryPackage()
	{
		m_position = nullptr;
		m_key = nullptr;
		m_data = nullptr;
	}
};