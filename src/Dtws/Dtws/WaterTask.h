#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class WaterTask : public CTask
{
public:
	WaterTask();

public:
	void DoTask();

	void StopTask();

	void Sleep(int32_t time);

private:
	std::atomic<bool> m_exit;
};