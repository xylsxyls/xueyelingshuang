#pragma once
#include "LumaPlayerCoreBridge.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <string>
#include <vector>
#include <QString>
#include <QObject>
class LumaPlayerKaraokeTask;

/** Owns one worker and its immutable result; teardown joins before releasing the result
*/
class LumaPlayerKaraokeJob : public QObject, public CTask
{
public:
    /** 初始化LumaPlayerKaraokeJob对象的状态，不转移输入借用对象的所有权
    @param [in] source 原媒体UTF-8路径，构造时复制保存
    @param [in] runtime ONNX Runtime路径，构造时复制保存
    @param [in] model ONNX模型路径，构造时复制保存
    */
    LumaPlayerKaraokeJob(const std::string& source, const std::string& runtime, const std::string& model);

    /** 初始化LumaPlayerKaraokeJob对象的状态，不转移输入借用对象的所有权
    @param [in] core 借用的Core桥接对象，全部任务退出前不得销毁
    @param [in] source 原媒体路径，构造时复制保存
    @param [in] destination 输出路径，构造时复制保存
    @param [in] mix 是否混合背景音
    @param [in] video 是否包含原视频画面
    @param [in] pitch 背景音升降调，-7至7半音
    @param [in] gain 背景音增益百分比，0至500，100表示不放大
    @param [in] backing 借用的伴奏句柄，可为空，导出任务结束前不得销毁
    */
    LumaPlayerKaraokeJob(LumaPlayerCoreBridge* core, const QString& source, const QString& destination,
        bool mix, bool video, int pitch, int gain, LumaPlayerCoreCBacking backing);

    /** Wait off the GUI thread for recording writes to settle, then complete skipped silence
    @param [in] captures 待结算任务的借用列表，结算调用结束前不得销毁
    @param [in] core 借用的Core桥接对象，全部任务退出前不得销毁
    @param [in] end100ns 结算终点，100纳秒单位
    */
    LumaPlayerKaraokeJob(const std::vector<LumaPlayerKaraokeTask*>& captures, LumaPlayerCoreBridge* core, int64_t end100ns);

    /** 释放对象持有的资源，退出前按实现等待其任务或回调
    */
    ~LumaPlayerKaraokeJob();

    /** 创建或启动当前工作任务，返回值不代表长任务完成
    @return true表示线程创建且任务已提交，false表示初始化失败
    */
    bool start();

    /** 执行当前任务的处理步骤，完成状态由实现发布
    */
    virtual void DoTask();

    /** 发布任务取消或退出请求，不代表执行线程已退出
    */
    virtual void StopTask();

    /** 读取任务完成标记，完成后才能读取结果数据
    @return true表示任务已退出处理并发布结果
    */
    bool done() const;

    /** 读取已完成工作的进度百分比
    @return 实际完成百分比，0至100
    */
    int progress() const;

    /** 读取取消请求标记
    @return true表示收到取消请求
    */
    bool canceled() const;

    /** 读取任务最终成功状态，应在done为true后调用
    @return true表示完成成功，未完成时不能作为成功证据
    */
    bool succeeded() const;

    /** 取得当前任务模式的弹窗标题
    @return 对应任务模式的标题文本
    */
    QString title() const;

    /** 取得最近错误诊断文本
    @return 诊断文本，无错误时为空
    */
    QString error() const;

    /** 转移伴奏句柄所有权，任务不再负责销毁返回的句柄
    @return 伴奏句柄，可能为空，调用方负责销毁
    */
    LumaPlayerCoreCBacking takeResult();

private:
    /** Copy a diagnostic into the fixed result buffer without allocating
    @param [in] message Error text, nullptr uses the fallback diagnostic
    */
    void setError(const char* message);

    /** Settle pending capture writes and publish silence; failures retain the recording
    */
    void finishRecording();

    /** 同步记录实际任务进度并响应取消标记
    @param [in] percent 实际已完成工作的百分比，0至100
    @param [in] user 借用的回调上下文，同步调用返回或异步完成前保持有效
    @return 非零表示继续，零表示已请求取消
    */
    static int onProgress(int percent, void* user);

    /** 查询最新停止录制控制是否已结算，最后一次写入是否退出
    @return true表示最新控制已应用且待写入已完成
    */
    bool recordingSettled() const;

    /** 在任务线程编码录音，成功后提交目标文件
    */
    void exportRecording();

private:
    // 源媒体路径，分离/导出任务拥有字符串
    std::string m_source;
    // ONNX Runtime路径字符串
    std::string m_runtime;
    // 人声分离模型路径字符串
    std::string m_model;
    // CTaskThreadManager工作线程标识，零表示未初始化
    uint32_t m_threadId;
    // 取消请求标记，不等于任务已经退出
    std::atomic<bool> m_cancel;
    // 任务完成发布标记，完成后才能读取非原子结果
    std::atomic<bool> m_done;
    // 实际任务完成百分比，0至100
    std::atomic<int> m_progress;
    // 任务持有的伴奏结果句柄，takeResult时转移所有权
    LumaPlayerCoreCBacking m_result;
    // 拥有的错误文本缓冲区，任务完成发布后读取
    char m_error[2048];
    // 借用的Core桥接对象，所有相关任务退出前保持有效
    LumaPlayerCoreBridge* m_core;
    // 导出目标路径字符串
    QString m_destination;
    // 导出是否混合背景音
    bool m_mix;
    // 导出是否包含原视频画面
    bool m_video;
    // 导出背景音的升降调半音数
    int m_pitch;
    // 导出背景音的软件增益百分比
    int m_gain;
    // 分离模型单次输入PCM帧数
    int m_separationWindowFrames;
    // 相邻推理窗口的重叠PCM帧数
    int m_separationOverlapFrames;
    // 借用的导出伴奏句柄，任务退出前由宿主保留
    LumaPlayerCoreCBacking m_backing;
    // 任务最终成功状态，在完成标记发布后读取
    bool m_success;
    // 结算期间借用的录音任务列表，全部任务退出前不得销毁
    std::vector<LumaPlayerKaraokeTask*> m_captures;
    // 结算终点，原媒体100纳秒单位
    int64_t m_end100ns;
};