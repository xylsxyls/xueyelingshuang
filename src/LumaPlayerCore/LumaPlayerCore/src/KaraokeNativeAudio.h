#pragma once

#include "LumaPlayerCoreC.h"
#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include "KaraokeMicrophoneControls.h"

struct KaraokeNativeBackend;

/** Owns native duplex audio; one lifecycle owner and one PCM consumer
Device callbacks only process bounded PCM and atomic state, never Core timeline locks
*/
class KaraokeNativeAudio
{
public:
    /** Construct a stopped, muted device
    */
    KaraokeNativeAudio();

    /** Stop callbacks before freeing their buffers
    */
    ~KaraokeNativeAudio();

    /** Enumerate UTF-8 input names; callback borrows each name until return
    @param [in] callback 不可为空，设备名仅在同步枚举回调期间有效
    @param [in] userData 借用的回调上下文，在回调可发生期间保持有效
    @return 零表示同步操作成功，非零为具体C错误码
    */
    static int32_t enumerate(LumaPlayerCoreCMicrophoneNameCallback callback, void* userData);

    /** Prepare selected device while stopped; queue and period are milliseconds
    @param [in] index 从零起的设备或槽位索引
    @param [in] queueMs 采集队列容量时长，毫秒
    @param [in] periodMs 请求的设备周期，毫秒，不代表端到端延迟
    @return 零表示同步操作成功，非零为具体C错误码
    */
    int32_t prepare(int32_t index, int32_t queueMs, int32_t periodMs);

    /** Open/start on the lifecycle worker; returns actual native initialization result
    @return 零表示成功，非零表示C错误码
    */
    int32_t start();

    /** Stop synchronously on lifecycle worker, never on a device callback
    */
    void stop();

    /** Set independent input gain/mute and speaker routing; actual PCM gain is 0..5000
    @param [in] gain 实际麦克风PCM增益百分比，0至5000
    @param [in] muted 麦克风静音标记，非零表示静音
    @param [in] monitoring 是否把麦克风PCM直接送入输出设备
    @return 零表示同步操作成功，非零为具体C错误码
    */
    int32_t controls(int32_t gain, bool muted, bool monitoring);

    /** Single consumer copies at most capacity mono frames; returns count
    @param [out] output 输出缓冲区或结果对象，非空时容量须满足声明，借用至返回
    @param [in] capacity output可容纳的单声道S16采样帧数
    @param [out] remainingMs 可为空，接收剩余采集队列时长，毫秒
    @return 实际复制的PCM帧数，无可用数据时返回零
    */
    size_t read(int16_t* output, size_t capacity, int32_t* remainingMs);

    /** Return an atomic diagnostic snapshot without borrowing backend memory
    @param [out] value 接收设备状态和音频计数的值副本，空值不写入
    */
    void stats(LumaPlayerCoreCMicrophoneStats* value);

    /** Device callback entry; input/output are borrowed mono S16 buffers
    @param [in] input 借用的PCM输入，长度由采样数或帧数乘声道数决定
    @param [out] output 输出缓冲区或结果对象，非空时容量须满足声明，借用至返回
    @param [in] frames PCM帧数，不是交错样本数或字节数
    */
    void process(const int16_t* input, int16_t* output, uint32_t frames);

    /** Mark an unexpected backend stop without releasing device resources on its callback
    */
    void interrupted();

    /** Backend timing only; no logging, allocation, locks or added audio buffering
    @param [in] timestamp100ns 后端捕获时间，100纳秒单位
    @param [in] flags 后端数据包状态标记
    @param [in] padding 输出设备当前排队帧数
    @param [in] rate 设备采样率，Hz
    */
    void observeCapturePacket(uint64_t timestamp100ns, uint32_t flags, uint32_t padding, uint32_t rate);

private:
    /** Prevent device ownership copying
    @param [in] other 禁止拷贝的源对象，不取得所有权
    */
    KaraokeNativeAudio(const KaraokeNativeAudio& other);

    /** Prevent device ownership assignment
    @param [in] other 禁止拷贝的源对象，不取得所有权
    @return 禁止赋值，该函数不提供实现
    */
    KaraokeNativeAudio& operator=(const KaraokeNativeAudio& other);

private:
    // Backend definitions remain private to the Core library
    std::unique_ptr<KaraokeNativeBackend> m_backend;
    // Fixed SPSC buffer; callback writes and worker reads, no resize while running
    std::vector<int16_t> m_ring;
    // Monotonic sample cursors, not byte offsets
    std::atomic<uint64_t> m_written;
    // 单消费者已读取的单调PCM帧下标
    std::atomic<uint64_t> m_read;
    // Diagnostic totals
    std::atomic<uint64_t> m_received;
    // 实际监听输出的累计PCM帧数
    std::atomic<uint64_t> m_monitored;
    // One packed control value makes gain/mute/monitor changes coherent per block
    std::atomic<uint32_t> m_controls;
    // Actual device lifecycle and callback observations
    std::atomic<bool> m_running;
    // 采集队列是否丢弃过输入帧
    std::atomic<bool> m_overflow;
    // 最近S16输入峰值，最大32767
    std::atomic<int32_t> m_peak;
    // 最近原生采集C错误码，原子读取
    std::atomic<int32_t> m_error;
    // 最近音频回调的PCM帧数
    std::atomic<int32_t> m_periodFrames;
    // 最近后端观测的捕获数据年龄，毫秒
    std::atomic<int32_t> m_captureAgeMs;
    // 后端观测到的最大捕获数据年龄，毫秒
    std::atomic<int32_t> m_maxCaptureAgeMs;
    // 后端输出设备排队时长，毫秒
    std::atomic<int32_t> m_outputPaddingMs;
    // 平台单调计数器频率，非Windows可为零
    int64_t m_counterFrequency;
    // 下一次采样诊断的累计输入帧边界
    uint64_t m_nextDiagnosticFrame;
};