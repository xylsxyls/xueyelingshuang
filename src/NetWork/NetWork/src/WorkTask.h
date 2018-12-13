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
	void setCallback(CallbackBase* callback);
	
	void setParam(uv_tcp_t* client, char* buffer, int32_t length);

	virtual void DoTask();
	
private:
	uv_tcp_t* m_client;
	char* m_buffer;
	int32_t m_length;
	int32_t m_protocolId;
	CallbackBase* m_callback;
};