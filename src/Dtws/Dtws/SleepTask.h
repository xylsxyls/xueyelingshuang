#include "SleepBaseTask.h"

class SleepTask : public SleepBaseTask
{
public:
	SleepTask();

public:
	void DoTask();

	void setParam(int32_t time);

private:
	int32_t m_time;
};