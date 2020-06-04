#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>
#include <string>
#include <map>
#include <vector>
#include <list>
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class LibuvTcp;
class CallbackBase;
typedef struct uv_tcp_s uv_tcp_t;

class WorkTask : public CTask
{
public:
	WorkTask();

public:
	virtual void DoTask();

	void setParam(uv_tcp_t* sender, char* buffer, int32_t length, LibuvTcp* libuvTcp);
	
private:
	uv_tcp_t* m_sender;
	char* m_buffer;
	int32_t m_length;
	int32_t m_protocolId;
	LibuvTcp* m_libuvTcp;
};