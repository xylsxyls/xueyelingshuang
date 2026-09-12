#pragma once
#include "TestRunState.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <QString>
#include <memory>

/** 耗时测试在专用线程运行，进程外看门狗处理不可中断旧测试 */
class CoreTestTask : public CTask
{
public:
    /** 保存测试参数
    @param [in] mode legacy/core/stress
    @param [in] media 真实视频
    @param [in] fixture 确定性AVI
    @param [in] directory 独占结果目录
    @param [in] state 跨线程终态
    */
    CoreTestTask(const QString& mode, const QString& media, const QString& fixture,
        const QString& directory, const std::shared_ptr<TestRunState>& state);

    /** 执行有限批次，不在GUI线程等待 */
    virtual void DoTask();

    /** 只设置退出标记；长调用在返回后的安全点检查 */
    virtual void StopTask();

private:
    // 停止请求
    std::atomic<bool> m_exit;
    // 执行组
    QString m_mode;
    // 实际媒体路径
    QString m_media;
    // 规则素材路径
    QString m_fixture;
    // 输出目录
    QString m_directory;
    // 共享终态，Task持有到完成
    std::shared_ptr<TestRunState> m_state;
};