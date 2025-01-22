#ifndef _TRACE_DATA_MAP_H__
#define _TRACE_DATA_MAP_H__
#include "TraceDataMap.h"

template <typename TraceDataType>
TraceDataMap<TraceDataType>::Stats::Stats():
	m_sum(0.0),
	m_count(0),
	m_min(std::numeric_limits<TraceDataType>::max()),
	m_max(std::numeric_limits<TraceDataType>::min())
{

}

template <typename TraceDataType>
void TraceDataMap<TraceDataType>::addValue(const std::string& key, TraceDataType value)
{
	auto& stats = m_data[key];
	stats.m_sum += static_cast<double>(value);
	stats.m_count += 1;
	stats.m_min = std::min(stats.m_min, value);
	stats.m_max = std::max(stats.m_max, value);
}

template <typename TraceDataType>
double TraceDataMap<TraceDataType>::average(const std::string& key) const
{
	auto it = m_data.find(key);
	if (it != m_data.end() && it->second.m_count > 0) {
		return it->second.m_sum / it->second.m_count;
	}
	return std::numeric_limits<double>::min();
}

template <typename TraceDataType>
TraceDataType TraceDataMap<TraceDataType>::min(const std::string& key) const
{
	auto it = m_data.find(key);
	if (it != m_data.end()) {
		return it->second.m_min;
	}
	return std::numeric_limits<TraceDataType>::max();
}

template <typename TraceDataType>
TraceDataType TraceDataMap<TraceDataType>::max(const std::string& key) const
{
	auto it = m_data.find(key);
	if (it != m_data.end()) {
		return it->second.m_max;
	}
	return std::numeric_limits<TraceDataType>::min();
}

#endif