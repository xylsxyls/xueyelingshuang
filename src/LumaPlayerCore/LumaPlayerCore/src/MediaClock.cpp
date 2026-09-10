#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "MediaClock.h"
#include "LumaPlayerCoreConfig.h"
#include <limits>

MediaClock::MediaClock() :
m_basePosition100ns(0),
m_startTick(std::chrono::steady_clock::now()),
m_ratePermille(LumaPlayerCoreConfig().m_defaultRatePermille),
m_isRunning(false)
{

}

void MediaClock::start(int64_t position100ns, int32_t ratePermille)
{
	m_basePosition100ns = position100ns < 0 ? 0 : position100ns;
	m_ratePermille = ratePermille > 0 ? ratePermille : 1;
	m_startTick = std::chrono::steady_clock::now();
	m_isRunning = true;
}

int64_t MediaClock::pause()
{
	int64_t position = position100ns();
	m_basePosition100ns = position;
	m_startTick = std::chrono::steady_clock::now();
	m_isRunning = false;
	return position;
}

void MediaClock::seek(int64_t position100ns)
{
	m_basePosition100ns = position100ns < 0 ? 0 : position100ns;
	m_startTick = std::chrono::steady_clock::now();
}

void MediaClock::setRate(int32_t ratePermille)
{
	int64_t position = position100ns();
	m_ratePermille = ratePermille > 0 ? ratePermille : 1;
	m_basePosition100ns = position;
	m_startTick = std::chrono::steady_clock::now();
}

int64_t MediaClock::position100ns() const
{
	if (!m_isRunning)
	{
		return m_basePosition100ns;
	}
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	int64_t elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_startTick).count();
	int64_t elapsed100ns = elapsedNs / 100;
	const int64_t maximum = (std::numeric_limits<int64_t>::max)();
	const int64_t whole = elapsed100ns / 1000;
	const int64_t fraction = (elapsed100ns % 1000) * m_ratePermille / 1000;
	if (whole > (maximum - m_basePosition100ns) / m_ratePermille)
	{
		return maximum;
	}
	const int64_t position = m_basePosition100ns + whole * m_ratePermille;
	return fraction > maximum - position ? maximum : position + fraction;
}

bool MediaClock::isRunning() const
{
	return m_isRunning;
}