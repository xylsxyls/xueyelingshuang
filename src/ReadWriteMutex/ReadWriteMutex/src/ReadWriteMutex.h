#pragma once
#include "ReadWriteMutexBase.h"
#include <mutex>

class ReadWriteMutexAPI ReadWriteMutex : public ReadWriteMutexBase
{
public:
	ReadWriteMutex();

public:
	virtual void read();
	virtual void unread();
	virtual void write();
	virtual void unwrite();

private:
	std::mutex m_readMutex;
	std::mutex m_writeMutex;
	//�ѼӶ�������
	int32_t m_readCount;
};