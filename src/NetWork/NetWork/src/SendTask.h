#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class LibuvTcp;
typedef struct uv_tcp_s uv_tcp_t;

class SendTask : public CTask
{
public:
	SendTask();

public:
	void setLibuvTcp(LibuvTcp* libuvTcp);

	void setParam(char* text);

	virtual void DoTask();
	
private:
	char* m_text;
	LibuvTcp* m_libuvTcp;
};