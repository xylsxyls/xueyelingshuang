#pragma once
#include <QString>
#include <map>
#include <set>
#include <vector>
#include <stdint.h>

/** 稳定用例ID；新增追加，删除保留空缺 */
enum TestCaseId
{
    CaseCppLifecycle = 1,
    CaseRepeatedInit = 2,
    CaseCApiLifecycle = 3,
    CaseCApiMedia = 4,
    CaseMediaControl = 5,
    CaseConfiguration = 6,
    CaseFullLoopCache = 7,
    CasePrefixLoopCache = 8,
    CaseLatestPreview = 9,
    CaseLoopMoveCancel = 10,
    CaseFrameAndRate = 11,
    CaseReentry = 12,
    CaseMediaAndPixels = 13,
    CaseAbBoundary = 14,
    CasePlayingAb = 15,
    CaseAbReentry = 16,
    CaseExitRace = 17,
    CaseMultiCore = 18,
    CaseLongPlayback = 19,
    CaseLongLoop = 20,
    CaseEmptyUi = 21,
    CasePlayPauseUi = 22,
    CaseDragUi = 23,
    CaseSeekKeys = 24,
    CaseFractionalKeys = 25,
    CaseAbMenu = 26,
    CasePausedMenu = 27,
    CaseFrameKey = 28,
    CaseResetUi = 29,
    CaseWindowState = 30,
    CasePinnedUi = 31,
    CaseHelpUi = 32,
    CaseQueuedClose = 33,
    CaseStartup = 34,
    CaseDiagnostics = 35,
    CaseSelectionRules = 36,
    CaseButtonVisuals = 37,
    CaseThreadStopRace = 38
};

/** 测试分类与ID独立 */
enum TestCaseType
{
    TestInstant = 0,
    TestPressure = 1
};

/** 注册元数据，中文说明直接用于界面和报告 */
struct TestCaseMetadata
{
public:
    // 中文功能及预期
    QString m_description;
    // 瞬时或压力
    TestCaseType m_type;
    // legacy/core/gui/stress/player
    QString m_engine;
    // 整个子进程预算，毫秒
    int32_t m_timeoutMs;
};

extern const std::map<TestCaseId, TestCaseMetadata> kTestCases;
extern const std::set<int32_t> kDeletedTestIds;

/** 统一的命令行及按钮选择解析，不执行任何测试 */
class TestCaseRegistry
{
public:
    /** 校验登记完整性
    @return 空字符串表示正确，否则中文原因
    */
    static QString validate();

    /** 解析数字及闭区间，未知ID拒绝，已删除ID跳过
    @param [in] text 1,3,8-12形式
    @param [in] active 当前有效ID
    @param [in] deleted 已删除ID
    @param [out] selected 去重排序后的有效ID
    @param [out] skipped 已删除ID
    @return 空字符串成功，否则中文原因
    */
    static QString parse(const QString& text, const std::set<int32_t>& active,
        const std::set<int32_t>& deleted, std::vector<int32_t>* selected, std::vector<int32_t>* skipped);

    /** 按按钮或命令行选择用例
    @param [in] mode specified/instant/pressure/all
    @param [in] text 指定模式输入
    @param [out] selected 有效ID，all先瞬时后压力
    @param [out] skipped 已删除ID
    @return 空字符串成功，否则中文原因
    */
    static QString select(const QString& mode, const QString& text,
        std::vector<int32_t>* selected, std::vector<int32_t>* skipped);
};