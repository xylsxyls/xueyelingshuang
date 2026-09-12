#pragma once
#include <QString>
#include <QStringList>
#include <stdint.h>

/** 隔离进程任务，结果目录与参数在启动前固定 */
struct TestJob
{
public:
    // 稳定用例ID
    int32_t m_caseId;
    // 显示名称
    QString m_name;
    // 可执行文件
    QString m_program;
    // 参数
    QStringList m_arguments;
    // 独占结果目录
    QString m_directory;
    // 是否实际播放器启动压力
    bool m_player;
    // 最大运行时间
    int32_t m_timeoutMs;
};