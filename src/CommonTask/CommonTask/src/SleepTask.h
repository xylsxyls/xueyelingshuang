#include "SleepBaseTask.h"
#include "CommonTaskMacro.h"

class CommonTaskAPI SleepTask : public SleepBaseTask
{
public:
	SleepTask();

public:
	void DoTask();

	void setParam(int32_t time);

private:
	int32_t m_time;
};