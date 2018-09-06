#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

typedef struct uv_loop_s uv_loop_t;

class RunLoopTask : public CTask
{
public:
	RunLoopTask();

public:
	void setLoop(uv_loop_t* loop);
	virtual void DoTask();

private:
	uv_loop_t* m_loop;
};