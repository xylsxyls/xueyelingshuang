#pragma once
#include "LogManagerMacro.h"
#include <stdint.h>
#include <string>

/** 日志初始化配置，用于指定日志目录、滚动策略、旧日志处理方式和控制台同步输出开关
*/
struct LogManagerAPI LogManagerConfig
{
public:
    // 日志文件ID，-1 保留为查询失败；同一进程的不同 ID 写入独立会话
    int32_t m_fileId;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 日志目录，相对路径在打开会话时固定；空串在原生端使用程序目录，Wasm 使用当前目录
    std::string m_path;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    // 单个实体日志文件最大字节数，-1 表示不限，其他非正数使用默认 100 MiB
    int64_t m_maxFileBytes;
    // 最多保留的实体日志文件数量，.0文件永不被程序滚删，-1 表示不限，其他小于 2 的值按 2 处理
    int32_t m_maxFileCount;
    // 每写入多少条日志检查一次文件大小，小于等于0时按1处理
    int32_t m_checkFileSizeInterval;
    // 是否在文件提交成功后同步复制到标准输出；C++ 输出流失败时仍可关闭及归档文件
    bool m_outputConsole;
    // 正常关闭时是否归档本次会话日志；false留在原目录，启动不处理任何历史日志
    bool m_archiveOldLog;

    /** 构造函数，设置默认日志策略
    */
    LogManagerConfig();
};