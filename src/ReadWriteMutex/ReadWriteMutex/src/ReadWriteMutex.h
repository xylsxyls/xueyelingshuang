#pragma once
#include "ReadWriteMutexMacro.h"

class ReadWriteMutexAPI ReadWriteMutex
{
public:
	ReadWriteMutex();

	~ReadWriteMutex();

public:
	void read();

	void unread();

	void write();

	void unwrite();

private:
	void* m_mutex;
};