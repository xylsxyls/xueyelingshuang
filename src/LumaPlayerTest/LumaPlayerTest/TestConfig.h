#pragma once
#include <QString>
#include <QStringList>
#include <stdint.h>
#include <string>

/** 测试程序的文字、规模和超时配置，构造后只读使用
*/
class TestConfig
{
public:
    /** 初始化本轮已授权的默认测试规模和界面文字
    */
    TestConfig();

public:
    // 真实素材目录、提示和有界探测参数
    QString m_videoLabel;
    // 测试媒体目录标签文本
    QString m_videoDirectory;
    // 素材准备中的状态文本
    QString m_mediaPreparing;
    // 素材生成取消文本
    QString m_mediaCanceled;
    // 未发现可用视频的诊断文本
    QString m_noVideo;
    // 测试自产素材生成失败文本
    QString m_fixtureFailed;
    // 媒体探测失败文本
    QString m_mediaProbeFailed;
    // 可读测试媒体扩展名列表
    QStringList m_videoFilters;
    // 最多探测的候选媒体数量
    int32_t m_videoProbeLimit;
    // 单次媒体探测超时，毫秒
    int32_t m_videoProbeMs;
    // 路径配置界面文案
    QString m_debugLabel;
    // Release路径标签文本
    QString m_releaseLabel;
    // 路径选择按钮文本
    QString m_choosePath;
    // 缺少有效路径的错误文本
    QString m_pathRequired;
    // 缺少Debug测试产物的错误文本
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
    // 未执行用例的说明文本
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