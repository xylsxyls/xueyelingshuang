#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class CameraCheckTask : public CTask
{
public:
	CameraCheckTask();

public:
	void DoTask();

	void StopTask();

private:
	std::atomic<bool> m_exit;
};