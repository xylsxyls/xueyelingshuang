#include "JidiTask.h"

void JidiTask::setParam(int32_t preSleepTime, int32_t clientIndex)
{
	m_preSleepTime = preSleepTime;
	m_clientIndex = clientIndex;
	m_placeName = "��Ȫ�Ƚ�����";
	m_clickIndex = 0;
	m_findClickTimes = 1;
}