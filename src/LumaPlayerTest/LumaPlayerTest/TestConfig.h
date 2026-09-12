#pragma once
#include <QString>
#include <QStringList>
#include <stdint.h>
#include <string>

/** 测试程序的文字、规模和超时配置，构造后只读使用 */
class TestConfig
{
public:
    /** 初始化本轮已授权的默认测试规模和界面文字 */
    TestConfig();

public:
    // 真实素材目录、提示和有界探测参数
    QString m_videoLabel;
    QString m_videoDirectory;
    QString m_mediaPreparing;
    QString m_mediaCanceled;
    QString m_noVideo;
    QString m_fixtureFailed;
    QString m_mediaProbeFailed;
    QStringList m_videoFilters;
    int32_t m_videoProbeLimit;
    int32_t m_videoProbeMs;
    // 路径配置界面文案
    QString m_debugLabel;
    QString m_releaseLabel;
    QString m_choosePath;
    QString m_pathRequired;
    QString m_debugRunnerRequired;
    // 窗口标题
    QString m_title;
    // 完成提示，报告区分测试结束与通过
    QString m_finished;
    // 用户关闭运行窗口时的提示
    QString m_running;
    // 平台、主观音画及素材覆盖边界
    QString m_limitations;
    // 非法构建配置提示
    QString m_invalidConfiguration;
    // 命令线程未成功创建或提交
    QString m_commandFailed;
    // 执行前显示实际选择和跳过项
    QString m_selectionText;
    QString m_skippedText;
    // 按钮依次为指定、瞬时、压力、全量
    QStringList m_buttons;
    // 指定输入说明
    QString m_idPrompt;
    // 停止按钮文案
    QString m_stopText;
    // 最终中文统计格式，使用CStringManager::Format
    std::wstring m_summaryFormat;
    // 状态中文名称
    QStringList m_statusNames;
    // 测试用例超时，毫秒
    int32_t m_caseTimeoutMs;
    // Release重复启动次数
    int32_t m_releaseStarts;
    // Debug重复启动次数
    int32_t m_debugStarts;
    // 持续播放与循环各自毫秒数
    int32_t m_stressMs;
    // GUI轮询间隔
    int32_t m_tickMs;
};