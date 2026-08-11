#pragma once
#include <stdint.h>
#include <string>

/** 启动时发现的旧日志文件信息，用于决定归档目录和移动目标
*/
struct LogManagerOldFile
{
	/** 构造函数
	*/
	LogManagerOldFile();

	// 旧日志完整路径
	std::string m_filePath;
	// 旧日志文件名
	std::string m_fileName;
	// 不包含.log和滚动序号的基础名
	std::string m_baseName;
	// 日志创建日期，格式为YYYYMMDD
	std::string m_date;
	// 日志创建时间，格式为HHMMSS
	std::string m_time;
	// 日志所属进程ID
	int32_t m_pid;
	// 滚动序号，软链接为-1，实体文件为0、1、2等
	int32_t m_fileIndex;
};