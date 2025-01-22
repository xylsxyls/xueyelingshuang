#include "CStopWatch.h"
#ifdef _WIN32
#include <windows.h>
#elif __unix__
#include <time.h>
#endif

CStopWatch::CStopWatch():
m_stopTime(0),
m_runTime(0),
m_stopOrRun(true)
{
	m_time = GetOpenTime();
}

int32_t CStopWatch::GetHour()
{
	return GetWatchTime() / 3600000;
}

int32_t CStopWatch::GetMinute()
{
	return (GetWatchTime() - GetHour() * 3600000) / 60000;
}

double CStopWatch::GetSeconds()
{
	return (GetWatchTime() - GetHour() * 3600000 - GetMinute() * 60000) / 1000.0;
}

std::string CStopWatch::GetWatchStrTime()
{
	int32_t hour = GetHour();
	std::string strHour = (hour != 0) ? (std::to_string(hour) + "时") : "";
	int32_t minute = GetMinute();
	std::string strMinute = (minute != 0) ? (std::to_string(minute) + "分") : "";
	double seconds = GetSeconds();
	char szSecond[32] = {};
	sprintf(szSecond, "%.3lf秒", seconds);
	return strHour + strMinute + szSecond;
}

unsigned long CStopWatch::GetWatchTime()
{
	return m_stopOrRun == false ? m_stopTime - m_time : GetOpenTime() - m_time;
}

void CStopWatch::SetWatchTime(unsigned long SetTime)
{
	m_time = m_time - (SetTime - GetWatchTime());
}

void CStopWatch::Run()
{
	if (m_stopOrRun == true)
	{
		return;
	}
	m_stopOrRun = true;
	m_runTime = GetOpenTime();
	m_time = m_time + m_runTime - m_stopTime;
}

void CStopWatch::Stop()
{
	if (m_stopOrRun == false)
	{
		return;
	}
	m_stopOrRun = false;
	m_stopTime = GetOpenTime();
}

unsigned long CStopWatch::GetOpenTime()
{
#ifdef _WIN32
	return ::GetTickCount();
#elif __unix__
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

//int main()
//{
//	CStopWatch a;
//	Sleep(1050);
//	printf("%s", a.GetWatchStrTime().c_str());
//	getchar();
//	return 0;
//}