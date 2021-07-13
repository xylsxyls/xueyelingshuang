#pragma once
#include "ReadWriteMutexMacro.h"

class ReadWriteMutexAPI ReadWriteMutexBase
{
public:
	virtual ~ReadWriteMutexBase();

public:
	virtual void read() = 0;
	virtual void write() = 0;
	virtual void unread() = 0;
	virtual void unwrite() = 0;
};