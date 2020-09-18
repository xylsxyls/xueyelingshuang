#include "PerformanceManager.h"
#include <QRegExp>
#include <QProcess>
#include "CStringManager/CStringManagerAPI.h"
#include <QDateTime>
#include <QString>
#include "CSystem/CSystemAPI.h"

PerformanceManager::PerformanceManager()
{

}

PerformanceManager& PerformanceManager::instance()
{
	static PerformanceManager s_performanceManager;
	return s_performanceManager;
}

void PerformanceManager::addCPUPid(uint32_t pid)
{
	auto it = m_cpu.find(pid);
	if (it != m_cpu.end())
	{
		return;
	}
	m_cpu[pid].setpid(pid);
}

std::string PerformanceManager::CPUOccupation(uint32_t pid)
{
	auto it = m_cpu.find(pid);
	if (it == m_cpu.end())
	{
		return "";
	}
	return CStringManager::Format("%.2f", it->second.get_cpu_usage());
}

void PerformanceManager::removeCPUPid(uint32_t pid)
{
	auto it = m_cpu.find(pid);
	if (it == m_cpu.end())
	{
		return;
	}
	m_cpu.erase(it);
}

std::string PerformanceManager::MemoryOccupation(uint32_t pid)
{
	QProcess p;
	p.start("tasklist /FI \"PID EQ " + QString::number(pid) + " \"");
	p.waitForFinished();
	QString result = QString::fromLocal8Bit(p.readAllStandardOutput());
	// 替换掉","
	result = result.replace(",", "");
	// 匹配 '数字+空格+K'部分。
	QRegExp rx("(\\d+)(\\s)(K)");
	// 初始化结果
	QString usedMem("");
	if (rx.indexIn(result) != -1)
	{
		// 匹配成功
		usedMem = rx.cap(0);
	}
	// 截取K前面的字符串，转换为数字，供换算单位使用。
	usedMem = usedMem.left(usedMem.length() - 1);
	// 换算为MB的单位
	result = QString::number(usedMem.toDouble() / 1024, 'g', 4);
	p.close();
	return result.toStdString();
}

std::string PerformanceManager::currentTime()
{
	return (QDateTime::currentDateTime()).toString("yyyy-MM-dd hh:mm:ss.zzz").toStdString();
}

std::string PerformanceManager::performanceWindowString()
{
	
	std::string result;
	for (auto it = m_performance.begin(); it != m_performance.end(); ++it)
	{
		std::string strPid = std::to_string(it->first);
		const std::string& cpu = it->second[0];
		const std::string& memory = it->second[1];

		std::string pidSpace;
		int32_t count = 10 - strPid.size();
		while (count-- != 0)
		{
			pidSpace.push_back(' ');
		}

		std::string cpuSpace;
		count = (cpu == "nil" ? 11 : 10) - cpu.size();
		while (count-- != 0)
		{
			cpuSpace.push_back(' ');
		}

		std::string memorySpace;
		count = (memory == "nil" ? 11 : 10) - memory.size();
		while (count-- != 0)
		{
			memorySpace.push_back(' ');
		}

		result += CStringManager::Format("%s%s%s%s%s%s%s\n", strPid.c_str(),
			pidSpace.c_str(),
			cpu == "nil" ? "nil" : (cpu + "%").c_str(),
			cpuSpace.c_str(),
			memory == "nil" ? "nil" : (memory + "M").c_str(),
			memorySpace.c_str(),
			it->second[2].c_str());
	}
	return result;
}

std::map<uint32_t, std::vector<std::string>> PerformanceManager::performance()
{
	return m_performance;
}

void PerformanceManager::addPerformance(uint32_t pid, bool checkCpu, bool checkMemory)
{
	auto it = m_performance.find(pid);
	if (it != m_performance.end())
	{
		return;
	}
	m_performance[pid].push_back(checkCpu ? "" : "nil");
	m_performance[pid].push_back(checkMemory ? "" : "nil");
	m_performance[pid].push_back(CSystem::processName(pid));
}

void PerformanceManager::modifyCpuPerformance(uint32_t pid, const std::string& cpuOccupation)
{
	auto it = m_performance.find(pid);
	if (it == m_performance.end())
	{
		return;
	}
	it->second[0] = cpuOccupation;
}

void PerformanceManager::modifyMemoryPerformance(uint32_t pid, const std::string& memoryOccupation)
{
	auto it = m_performance.find(pid);
	if (it == m_performance.end())
	{
		return;
	}
	it->second[1] = memoryOccupation;
}