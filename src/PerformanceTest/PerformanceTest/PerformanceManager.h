#pragma once
#include "CPUusage.h"
#include <map>

class PerformanceManager
{
private:
	PerformanceManager();

public:
	static PerformanceManager& instance();

public:
	void addCPUPid(uint32_t pid);

	std::string CPUOccupation(uint32_t pid);

	void removeCPUPid(uint32_t pid);

	std::string MemoryOccupation(uint32_t pid);

	std::string currentTime();

	std::string performanceWindowString();

	std::map<uint32_t, std::vector<std::string>> performance();

	void addPerformance(uint32_t pid, bool checkCpu, bool checkMemory);

	void modifyCpuPerformance(uint32_t pid, const std::string& cpuOccupation);

	void modifyMemoryPerformance(uint32_t pid, const std::string& memoryOccupation);

private:
	std::map<uint32_t, CPUusage> m_cpu;
	std::map<uint32_t, std::vector<std::string>> m_performance;
};