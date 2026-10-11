#pragma once

#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

#include <QByteArray>

#include <atomic>
#include <stdint.h>

class LumaPlayerCoreBridge;
class LumaPlayerMicrophoneCapture;

/** Lifecycle/recording worker; Core owns native capture and independent duplex monitoring
*/
class LumaPlayerKaraokeTask : public CTask
{
public:
    /** 初始化LumaPlayerKaraokeTask对象的状态，不转移输入借用对象的所有权
    @param [in] capture 采集适配器，生命周期任务退出前保持有效
    @param [in] core 借用的Core桥接对象，全部任务退出前不得销毁
    @param [in] microphone 麦克风槽位索引，0至2
    */
    LumaPlayerKaraokeTask(LumaPlayerMicrophoneCapture* capture,
        LumaPlayerCoreBridge* core, int32_t microphone = 0);

    /** 释放对象持有的资源，退出前按实现等待其任务或回调
    */
    ~LumaPlayerKaraokeTask();

    /** 初始化对象及其工作资源
    @return true表示初始化成功
    */
    bool init();

    /** 请求停止并等待对象持有的任务退出
    */
    void uninit();

    /** 发布麦克风增益、静音、监听及录制控制状态
    @param [in] gainPercent 麦克风软件增益百分比，0至5000，界面显示值与此不同
    @param [in] muted 麦克风静音标记，非零表示静音
    @param [in] monitoring 是否把麦克风PCM直接送入输出设备
    @param [in] recording 是否启用录音写入
    @param [in] timelineActive 媒体时间线是否在推进
    */
    void setControls(int32_t gainPercent, bool muted, bool monitoring,
        bool recording, bool timelineActive);

    /** 取得最近错误诊断
    @return 最近C错误码，零表示无错误
    */
    int32_t lastError() const;

    /** 读取最近输入PCM的峰值幅度
    @return S16采样峰值，0表示静音或尚未收到输入
    */
    int32_t lastInputPeak() const;

    /** 查询当前采集流是否收到过PCM
    @return true表示已收到输入
    */
    bool hasReceivedPcm() const;

    /** 读取已提交实时监听的累计字节数
    @return 累计监听PCM字节数
    */
    uint64_t monitorBytesWritten() const;

    /** 读取原生音频回调周期的观测值
    @return 观测的回调周期，毫秒
    */
    int32_t monitorPeriodMs() const;

    /** 读取当前任务观测到的最大PCM处理耗时
    @return 观测到的最大处理耗时，毫秒
    */
    int32_t maxProcessingMs() const;

    /** 取得监听格式每秒PCM字节数
    @return 监听PCM每秒字节数
    */
    int32_t monitorBytesPerSecond() const;

    /** Observe stop acknowledgment after the last in-flight PCM write has returned
    @return true表示最新控制已应用且待写入已完成
    */
    bool recordingSettled() const;

    /** 执行当前任务的处理步骤，完成状态由实现发布
    */
    virtual void DoTask();

    /** 发布任务取消或退出请求，不代表执行线程已退出
    */
    virtual void StopTask();

private:
    /** 消费已采集PCM并按媒体时间线写入，监听由Core设备回调完成
    */
    void processAvailablePcm();

    /** 保存错误码和诊断日志，不主动终止程序
    @param [in] code 结果码，普通错误通过返回或状态发布
    @param [in] message 借用的诊断文本，空值使用入口约定的默认文本
    */
    void setError(int32_t code, const char* message);

private:
    // 采集适配器，宿主槽位拥有，任务退出前有效
    LumaPlayerMicrophoneCapture* m_capture;
    // 借用的Core桥接对象，所有相关任务退出前保持有效
    LumaPlayerCoreBridge* m_core;
    // 录音轨槽位编号，0至2
    int32_t m_microphone;
    // CTaskThreadManager工作线程标识，零表示未初始化
    uint32_t m_threadId;
    // 任务退出请求标记
    std::atomic<bool> m_exit;
    // 任务完成发布标记，完成后才能读取非原子结果
    std::atomic<bool> m_done;
    // 最近发布的控制版本
    std::atomic<uint64_t> m_controlsVersion;
    // 工作线程已结算的控制版本
    std::atomic<uint64_t> m_appliedVersion;
    // 原生麦克风软件增益百分比，0至5000
    std::atomic<int32_t> m_gainPercent;
    // 麦克风静音开关，静音不代表关闭设备
    std::atomic<bool> m_muted;
    // 麦克风实时监听输出开关
    std::atomic<bool> m_monitoring;
    // 是否接受录音写入，暂停时仍由播放状态限制写入
    std::atomic<bool> m_recording;
    // 原媒体播放时间线是否在推进
    std::atomic<bool> m_timelineActive;
    // 最近任务错误码
    std::atomic<int32_t> m_lastError;
    // 最近输入S16峰值幅度
    std::atomic<int32_t> m_lastInputPeak;
    // 当前采集流是否已收到非空PCM
    std::atomic<bool> m_hasReceivedPcm;
    // 实际观测的最大PCM处理时长，毫秒
    std::atomic<int32_t> m_maxProcessingMs;
    // 唤醒生命周期任务的信号量
    Semaphore m_wake;
    // 已提交录音区间的终点，100纳秒单位
    int64_t m_recordingEnd100ns;
    // 前次媒体时间快照，100纳秒单位
    int64_t m_recordingSnapshot100ns;
    // 本次录制锁定的速率千分值
    int32_t m_recordingRate;
    // 录制状态代次，用于识别新的录制会话
    uint64_t m_recordingSerial;
};