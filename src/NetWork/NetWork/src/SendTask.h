#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class LibuvTcp;
typedef struct uv_tcp_s uv_tcp_t;

class SendTask : public CTask
{
public:
	SendTask();

public:
	void setLibuvTcp(LibuvTcp* libuvTcp);

	void setParam(uv_tcp_t* dest, char* buffer, int32_t length);

	virtual void DoTask();
	
private:
	uv_tcp_t* m_dest;
	char* m_buffer;
	int32_t m_length;
	LibuvTcp* m_libuvTcp;
};