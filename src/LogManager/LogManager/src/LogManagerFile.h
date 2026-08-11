#pragma once
#include "LogManagerConfig.h"
#include <fstream>
#include <stdint.h>
#include <string>

/** 单个日志文件ID对应的运行时状态，记录当前实体文件、软链接、滚动序号和删除保护句柄
*/
class LogManagerFile
{
public:
	/** 构造函数
	*/
	LogManagerFile();

	/** 析构函数，只做兜底资源释放
	*/
	~LogManagerFile();

public:
	/** 释放文件流和删除保护句柄
	*/
	void reset();

public:
	// 日志初始化配置
	LogManagerConfig m_config;
	// 日志目录，带路径分隔符
	std::string m_logDir;
	// 日志基础名，格式为进程名_pid_YYYYMMDD_HHMMSS
	std::string m_baseName;
	// 当前日志软链接路径
	std::string m_linkPath;
	// 当前实体日志路径
	std::string m_currentEntityPath;
	// 当前实体日志文件流
	std::ofstream* m_logFile;
	// 当前实体文件下标
	int32_t m_currentIndex;
	// 距离上次检查文件大小已经写入的日志行数
	int32_t m_writeCountSinceSizeCheck;
	// 当前实体文件删除保护句柄
	void* m_currentFileLock;
	// 当前软链接删除保护句柄
	void* m_linkFileLock;
};