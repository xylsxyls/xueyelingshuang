#pragma once
#include "TestResults.h"
#include <QString>
#include "LumaPlayerCore/LumaPlayerCoreAPI.h"
#include "LumaPlayerCore/LumaPlayerCoreC.h"
#include <atomic>
/** 分离用例的同步进度与取消状态，由用例持有至分离返回
*/
struct SeparationTestProgress
{
public:
    // 上次回调进度，0至100
    int m_last;
    // 到达此百分比时取消，101表示不取消
    int m_cancelAt;
    // 回调是否一直单调且不超过100
    bool m_ordered;

public:
    /** 初始化独立的进度观察器
    @param [in] cancelAt 取消阈值百分比，默认101
    */
    explicit SeparationTestProgress(int cancelAt = 101);

    /** 检查单调进度并提供取消条件
    @param [in] percent 实际分离进度，0至100
    @param [in] user 指向用例持有的SeparationTestProgress，不可为空
    @return 非零继续，零取消
    */
    static int callback(int percent, void* user);
};

/** 异步录制状态的测试观察器，借用期覆盖回调完成
*/
struct KaraokeTestCompletion
{
public:
    // 发布结果的完成标记
    std::atomic<bool> m_done;
    // 完成结果，在m_done为true后读取
    int m_result;

public:
    /** 初始化未完成状态
    */
    KaraokeTestCompletion();

    /** 保存异步操作的结果并发布完成标记
    @param [in] value 借用的完成数据，仅回调期间有效且不可为空
    @param [in] user 指向用例观察器，等待期间不可销毁
    */
    static void receive(const LumaPlayerCoreCCompletion* value, void* user);
};

/** 执行指定K歌后端用例，输入素材由测试程序生成
    @param [in] id 稳定用例ID
    @param [in,out] report 记录每个独立预期的结果
    @param [in] directory 用例输出目录
    @param [in] fixture 自产媒体路径，测试期间存在
    */
void RunKaraokeBackendCase(int id, TestResults& report, const QString& directory, const QString& fixture);