#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "TestCaseRegistry.h"
#include <QStringList>
#include <QRegExp>

const std::map<TestCaseId, TestCaseMetadata> kTestCases =
{
    {CaseCppLifecycle, {QStringLiteral("未初始化、空路径及倍率边界应返回约定结果"), TestInstant, "legacy", 30000}},
    {CaseRepeatedInit, {QStringLiteral("反复初始化20次并释放，不得残留初始化状态"), TestPressure, "legacy", 60000}},
    {CaseCApiLifecycle, {QStringLiteral("C接口生命周期和非法参数返回值正确"), TestInstant, "legacy", 30000}},
    {CaseCApiMedia, {QStringLiteral("C接口打开、预览、逐帧及异步播放暂停正确"), TestInstant, "legacy", 30000}},
    {CaseMediaControl, {QStringLiteral("预览覆盖目标帧，短AB包括B帧，暂停不清音频"), TestInstant, "legacy", 30000}},
    {CaseConfiguration, {QStringLiteral("C和C++独立配置、错误描述及参数校验一致"), TestInstant, "legacy", 30000}},
    {CaseFullLoopCache, {QStringLiteral("完整缓存循环接缝及单帧100次循环均正确"), TestPressure, "legacy", 60000}},
    {CasePrefixLoopCache, {QStringLiteral("小缓存循环不得跳过A帧或越过B边界"), TestPressure, "legacy", 60000}},
    {CaseLatestPreview, {QStringLiteral("频繁拖动预览应保留最新目标且安全退出"), TestInstant, "legacy", 30000}},
    {CaseLoopMoveCancel, {QStringLiteral("AB逐帧连续移动与取消不得残留旧操作"), TestInstant, "legacy", 30000}},
    {CaseFrameAndRate, {QStringLiteral("时间溢出保护和实际PCM倍速转换正确"), TestInstant, "legacy", 30000}},
    {CaseReentry, {QStringLiteral("回调重入应拒绝，关闭后不得再输出旧帧"), TestInstant, "legacy", 30000}},
    {CaseMediaAndPixels, {QStringLiteral("AVI和MP4可打开，切镜颜色及错误后恢复正确"), TestInstant, "core", 30000}},
    {CaseAbBoundary, {QStringLiteral("25fps独立帧表的AB左右边界与末帧正确"), TestInstant, "core", 30000}},
    {CasePlayingAb, {QStringLiteral("播放中设置AB不得暂停、清音频或重新启动"), TestInstant, "core", 30000}},
    {CaseAbReentry, {QStringLiteral("手动跳出AB继续播放，重入恢复循环，结尾从零开始"), TestInstant, "core", 30000}},
    {CaseExitRace, {QStringLiteral("打开、定位、预览、逐帧及播放中立即退出各5次"), TestPressure, "core", 60000}},
    {CaseMultiCore, {QStringLiteral("两个Core同时播放，销毁一个不影响另一个"), TestInstant, "core", 30000}},
    {CaseLongPlayback, {QStringLiteral("持续播放2分钟，帧持续输出且结尾正常重播"), TestPressure, "stress", 180000}},
    {CaseLongLoop, {QStringLiteral("短AB持续循环2分钟，输出不断流且循环正常"), TestPressure, "stress", 180000}},
    {CaseEmptyUi, {QStringLiteral("空窗口只出现上浮框，中央加号无提示"), TestInstant, "gui", 30000}},
    {CasePlayPauseUi, {QStringLiteral("键盘、视频单击及播放按钮切换实际播放状态"), TestInstant, "gui", 30000}},
    {CaseDragUi, {QStringLiteral("32次拖动进度即时跟手，松开定位到正确帧"), TestInstant, "gui", 30000}},
    {CaseSeekKeys, {QStringLiteral("普通左右键按默认2秒跳转，不受AB范围限制"), TestInstant, "gui", 30000}},
    {CaseFractionalKeys, {QStringLiteral("配置为0.5秒时左右键按小数步长跳转"), TestInstant, "gui", 30000}},
    {CaseAbMenu, {QStringLiteral("播放中右键菜单及设置AB不改变播放状态"), TestInstant, "gui", 30000}},
    {CasePausedMenu, {QStringLiteral("暂停时右击定位，画面和进度同步"), TestInstant, "gui", 30000}},
    {CaseFrameKey, {QStringLiteral("悬停AB按方向键只移动一个实际帧并暂停"), TestInstant, "gui", 30000}},
    {CaseResetUi, {QStringLiteral("平移缩放倍率重置，保留AB进度和播放状态"), TestInstant, "gui", 30000}},
    {CaseWindowState, {QStringLiteral("最大化、全屏与图钉独立，恢复最新普通矩形"), TestInstant, "gui", 30000}},
    {CasePinnedUi, {QStringLiteral("固定浮框避让视频，取消固定后自动隐藏"), TestInstant, "gui", 30000}},
    {CaseHelpUi, {QStringLiteral("关于弹窗居中、版本左对齐、可拖动且两按钮关闭"), TestInstant, "gui", 30000}},
    {CaseQueuedClose, {QStringLiteral("重置尚在执行时关闭窗口，应正常收敛退出"), TestInstant, "gui", 30000}},
    {CaseStartup, {QStringLiteral("独立播放器重复启动，窗口可见并正常退出"), TestPressure, "player", 60000}},
    {CaseDiagnostics, {QStringLiteral("日志实际落盘并包含源码文件信息"), TestInstant, "core", 30000}},
    {CaseSelectionRules, {QStringLiteral("指定范围去重、删除跳过和非法输入判定正确"), TestInstant, "core", 30000}},
    {CaseButtonVisuals, {QStringLiteral("工具提示、按钮按下态和进度圆点悬停按下态正确"), TestInstant, "gui", 30000}},
    {CaseThreadStopRace, {QStringLiteral("任务切换中停止当前任务并回收线程200次，不得崩溃或残留线程"), TestPressure, "core", 60000}}
};

const std::set<int32_t> kDeletedTestIds;

QString TestCaseRegistry::validate()
{
    const TestCaseId declared[] = {CaseCppLifecycle, CaseRepeatedInit, CaseCApiLifecycle, CaseCApiMedia, CaseMediaControl, CaseConfiguration, CaseFullLoopCache, CasePrefixLoopCache, CaseLatestPreview, CaseLoopMoveCancel, CaseFrameAndRate, CaseReentry, CaseMediaAndPixels, CaseAbBoundary, CasePlayingAb, CaseAbReentry, CaseExitRace, CaseMultiCore, CaseLongPlayback, CaseLongLoop, CaseEmptyUi, CasePlayPauseUi, CaseDragUi, CaseSeekKeys, CaseFractionalKeys, CaseAbMenu, CasePausedMenu, CaseFrameKey, CaseResetUi, CaseWindowState, CasePinnedUi, CaseHelpUi, CaseQueuedClose, CaseStartup, CaseDiagnostics, CaseSelectionRules, CaseButtonVisuals, CaseThreadStopRace};
    std::set<int32_t> ids;
    for (size_t i = 0; i < sizeof(declared) / sizeof(declared[0]); ++i)
    {
        if (!ids.insert(declared[i]).second || kTestCases.count(declared[i]) != 1 || kDeletedTestIds.count(declared[i]) != 0)
        {
            return QStringLiteral("用例枚举存在重复、缺登记或与删除ID冲突");
        }
        const TestCaseMetadata& item = kTestCases.at(declared[i]);
        if (item.m_description.isEmpty() || item.m_timeoutMs <= 0 ||
            (item.m_type != TestInstant && item.m_type != TestPressure))
        {
            return QStringLiteral("用例中文说明、类型或超时配置无效");
        }
    }
    return ids.size() == kTestCases.size() ? QString() : QStringLiteral("存在未声明的用例登记");
}

QString TestCaseRegistry::parse(const QString& text, const std::set<int32_t>& active,
    const std::set<int32_t>& deleted, std::vector<int32_t>* selected, std::vector<int32_t>* skipped)
{
    selected->clear();
    skipped->clear();
    std::set<int32_t> unique;
    std::set<int32_t> removed;
    const QStringList parts = QString(text).replace(QStringLiteral("，"), ",").split(',');
    QRegExp expression("^([0-9]+)(?:\\s*-\\s*([0-9]+))?$");
    for (int32_t i = 0; i < parts.size(); ++i)
    {
        if (!expression.exactMatch(parts[i].trimmed()))
        {
            return QStringLiteral("编号格式错误：") + parts[i];
        }
        bool firstOk = false;
        bool lastOk = false;
        int32_t first = expression.cap(1).toInt(&firstOk);
        int32_t last = expression.cap(2).isEmpty() ? first : expression.cap(2).toInt(&lastOk);
        if (!firstOk || (!expression.cap(2).isEmpty() && !lastOk) || first <= 0 || last < first || last - first > 100000)
        {
            return QStringLiteral("编号或范围无效：") + parts[i];
        }
        for (int64_t id = first; id <= last; ++id)
        {
            if (deleted.count(static_cast<int32_t>(id)))
            {
                removed.insert(static_cast<int32_t>(id));
            }
            else if (active.count(static_cast<int32_t>(id)))
            {
                unique.insert(static_cast<int32_t>(id));
            }
            else
            {
                return QStringLiteral("未知用例编号：") + QString::number(id);
            }
        }
    }
    selected->assign(unique.begin(), unique.end());
    skipped->assign(removed.begin(), removed.end());
    return selected->empty() ? QStringLiteral("没有可执行的测试用例") : QString();
}

QString TestCaseRegistry::select(const QString& mode, const QString& text,
    std::vector<int32_t>* selected, std::vector<int32_t>* skipped)
{
    selected->clear();
    skipped->clear();
    const QString error = validate();
    if (!error.isEmpty())
    {
        return error;
    }
    std::set<int32_t> active;
    for (std::map<TestCaseId, TestCaseMetadata>::const_iterator it = kTestCases.begin(); it != kTestCases.end(); ++it)
    {
        active.insert(it->first);
    }
    if (mode == "specified")
    {
        return parse(text, active, kDeletedTestIds, selected, skipped);
    }
    if (mode != "instant" && mode != "pressure" && mode != "all")
    {
        return QStringLiteral("未知测试方式");
    }
    for (int32_t type = 0; type <= 1; ++type)
    {
        for (std::map<TestCaseId, TestCaseMetadata>::const_iterator it = kTestCases.begin(); it != kTestCases.end(); ++it)
        {
            if (it->second.m_type == type && (mode == "all" || (mode == "instant" && type == 0) || (mode == "pressure" && type == 1)))
            {
                selected->push_back(it->first);
            }
        }
    }
    return QString();
}