#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "TestConfig.h"

TestConfig::TestConfig() :
m_videoLabel(QStringLiteral("真实视频路径")),
m_videoDirectory(QStringLiteral("D:/Backup/Downloads")),
m_mediaPreparing(QStringLiteral("正在查找真实视频并生成固定测试素材，可点击停止测试取消")),
m_mediaCanceled(QStringLiteral("素材准备已取消，测试未执行")),
m_noVideo(QStringLiteral("当前目录未找到可用视频（不搜索子目录，最多探测20个候选），请选择其他真实视频目录")),
m_fixtureFailed(QStringLiteral("无法生成测试素材：")),
m_mediaProbeFailed(QStringLiteral("无法启动媒体探测程序：")),
m_videoProbeLimit(20),
m_videoProbeMs(5000),
m_debugLabel(QStringLiteral("是否同步测试Debug程序")),
m_releaseLabel(QStringLiteral("Release")),
m_choosePath(QStringLiteral("选择路径")),
m_pathRequired(QStringLiteral("请选择包含以下程序的目录：")),
m_debugRunnerRequired(QStringLiteral("Debug目录缺少LumaPlayerTestd.exe，请部署匹配的Debug测试执行程序")),
m_title(QStringLiteral("流光播放器测试 1.0")),
m_finished(QStringLiteral("测试执行已结束，请查看报告中的失败和未测项。请返回 Codex 继续会话，由 AI 分析报告和日志。报告目录：")),
m_running(QStringLiteral("测试仍在运行，请等待完成提示。直接终止程序会保留未完成状态。")),
m_limitations(QStringLiteral("仅验证Windows x64；Linux/macOS、32位未验证。主观画质与听感由手动试运行确认，未提供VFR及全部编码格式素材。未选压力用例不计通过，短压测不等于长时间稳定性证明。")),
m_invalidConfiguration(QStringLiteral("构建配置必须为release、debug或both")),
m_commandFailed(QStringLiteral("测试命令提交失败，请查看日志")),
m_selectionText(QStringLiteral("本批实际选择ID：")),
m_skippedText(QStringLiteral("已删除跳过ID：")),
m_caseTimeoutMs(600000),
m_releaseStarts(60),
m_debugStarts(8),
m_stressMs(120000),
m_tickMs(20)
{
    m_videoFilters << "*.mp4" << "*.mkv" << "*.avi" << "*.mov" << "*.webm" << "*.flv" << "*.ts" << "*.m4v" << "*.wmv" << "*.mpg" << "*.mpeg";
    m_buttons << QStringLiteral("指定测试") << QStringLiteral("瞬时测试") << QStringLiteral("压力测试") << QStringLiteral("全量测试");
    m_idPrompt = QStringLiteral("输入数字ID，例如 1,3,8-12；已删除编号自动跳过");
    m_stopText = QStringLiteral("停止测试");
    m_summaryFormat = L"实际执行%u个用例：瞬时%u个，压力%u个；通过%u个，失败%u个，取消%u个。未执行%u个，已删除跳过%u个。总耗时%.1f秒。";
    m_statusNames << QStringLiteral("未执行") << QStringLiteral("通过") << QStringLiteral("失败") << QStringLiteral("取消") << QStringLiteral("运行中");
}