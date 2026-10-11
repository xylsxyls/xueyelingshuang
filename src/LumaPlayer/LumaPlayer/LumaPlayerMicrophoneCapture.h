#pragma once
#ifndef LUMAPLAYERCORE_USE_C_API
#define LUMAPLAYERCORE_USE_C_API
#endif
#include "LumaPlayerCore/LumaPlayerCoreAPI.h"
#include <QByteArray>
#include <QStringList>
#include <atomic>

/** UI adapter only; Core owns the native device and microphone audio
*/
class LumaPlayerMicrophoneCapture
{
public:
    /** Allocate a stopped Core device
    */
    LumaPlayerMicrophoneCapture();

    /** Release after its lifecycle Task has joined
    */
    ~LumaPlayerMicrophoneCapture();

    /** Enumerate current UTF-8 input names via Core
    @return 设备名称值副本
    */
    static QStringList availableDeviceNames();

    /** Prepare/start synchronously, only for a lifecycle worker
    @param [in] deviceIndex 枚举时的输入设备索引
    @param [in] queueDurationMs 采集队列容量时长，毫秒
    @param [in] deviceBufferMs 请求的设备周期，毫秒，不代表端到端延迟
    @return 原生设备准备及启动均成功才返回true
    */
    bool start(int32_t deviceIndex, int32_t queueDurationMs = 2000, int32_t deviceBufferMs = 40);

    /** Prepare while worker is stopped; buffer parameters are milliseconds
    @param [in] deviceIndex 枚举时的输入设备索引
    @param [in] queueDurationMs 采集队列容量时长，毫秒
    @param [in] deviceBufferMs 请求的设备周期，毫秒，不代表端到端延迟
    @return 设备及采集缓冲准备成功才返回true
    */
    bool prepare(int32_t deviceIndex, int32_t queueDurationMs, int32_t deviceBufferMs);

    /** Start the prepared Core device on lifecycle worker
    @return 原生设备实际启动成功才返回true
    */
    bool startPrepared();

    /** Join callbacks on lifecycle worker, never from a device callback
    */
    void stop();

    /** Single consumer copies at most maxBytes; remainingMs is optional
    @param [out] pcm 不可为空，接收当前S16样本的字节副本
    @param [in] maxBytes 最多复制的PCM字节数
    @param [out] remainingMs 可为空，接收剩余采集队列时长，毫秒
    @return true表示已复制非空PCM
    */
    bool takePcm(QByteArray* pcm, int32_t maxBytes, int32_t* remainingMs = nullptr);

    /** Set actual PCM gain 0..5000 and independent mute/monitor switches
    @param [in] gain 实际麦克风PCM增益百分比，0至5000
    @param [in] muted 麦克风静音标记，非零表示静音
    @param [in] monitoring 是否把麦克风PCM直接送入输出设备
    */
    void setControls(int32_t gain, bool muted, bool monitoring);

    /** Obtain native callback observations
    @return 线程安全的统计值副本
    */
    LumaPlayerCoreCMicrophoneStats stats() const;

    /** Query actual running state
    @return true表示设备正在采集
    */
    bool isRunning() const;

    /** Query recording queue loss
    @return true表示队列发生过丢帧
    */
    bool hasOverflowed() const;

    /** Query successful start identity
    @return 成功启动流的单调代次
    */
    uint64_t streamGeneration() const;

    /** Native boundary supplies 48kHz mono S16
    @return 每秒采样帧数，Hz
    */
    int32_t sampleRate() const;

    /** Native boundary supplies one channel
    @return 声道数
    */
    int32_t channels() const;

    /** Prepared device display name
    @return 已准备设备名称的值副本
    */
    QString deviceName() const;

    /** Human-readable failure; empty when no error
    @return 错误说明值副本，无错误时为空
    */
    QString lastError() const;

private:
    // Core handle lives until all callers have stopped
    LumaPlayerCoreCMicrophone m_handle;
    // Format prepared before worker creation
    std::atomic<bool> m_prepared;
    // Last synchronous error and device incarnation
    std::atomic<int32_t> m_error;
    // 成功启动的采集流代次
    std::atomic<uint64_t> m_generation;
    // Changed only on GUI while worker is stopped
    QString m_deviceName;
};