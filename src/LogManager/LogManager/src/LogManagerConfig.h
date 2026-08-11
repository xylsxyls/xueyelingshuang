#pragma once
#include "LogManagerMacro.h"
#include <stdint.h>
#include <string>

/** 日志初始化配置，用于指定日志目录、滚动策略、旧日志处理方式和控制台同步输出开关
*/
struct LogManagerAPI LogManagerConfig
{
	/** 构造函数，设置默认日志策略
	*/
	LogManagerConfig();

	// 日志文件ID，同一个进程中不同ID会写入不同日志文件
	int32_t m_fileId;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	// 日志目录，空字符串表示使用exe所在目录
	std::string m_path;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	// 单个实体日志文件最大字节数，小于等于0时使用默认100M
	int64_t m_maxFileBytes;
	// 最多保留的实体日志文件数量，.0文件永不被程序滚删，小于2时按2处理
	int32_t m_maxFileCount;
	// 每写入多少条日志检查一次文件大小，小于等于0时按1处理
	int32_t m_checkFileSizeInterval;
	// 是否同步输出到控制台
	bool m_outputConsole;
	// 启动时是否归档旧日志，false表示直接删除旧日志
	bool m_archiveOldLog;
};