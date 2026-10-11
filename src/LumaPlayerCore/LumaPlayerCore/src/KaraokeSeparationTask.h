#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "KaraokeStorage.h"
#include <atomic>
#include <string>
#include <vector>

/** 一个MDX模型窗口的串行分析/重建数据
*/
class KaraokeSeparationTask : public CTask
{
public:
    /** 初始化KaraokeSeparationTask对象的状态，不转移输入借用对象的所有权
    @param [in] source 借用的源PCM临时存储，窗口处理完成前有效
    @param [in] frames 原音频PCM总帧数
    @param [in] start 扩展后PCM时间线中的窗口起始帧
    @param [in] total 带边界的处理总帧数
    @param [in] step 相邻窗口步长，PCM帧数
    @param [in] border 模型处理两端边界，PCM帧数
    */
    KaraokeSeparationTask(KaraokeStorage* source, size_t frames, size_t start,
        size_t total, size_t step, size_t border);

    /** 执行当前任务的处理步骤，完成状态由实现发布
    */
    virtual void DoTask();

    /** 发布任务取消或退出请求，不代表执行线程已退出
    */
    virtual void StopTask();

    /** 将分离窗口切换为重建阶段
    */
    void reconstruct();

    /** Merge this window into scratch storage
    @param [in,out] output Existing reconstruction storage, not nullptr
    @return True on success, false with m_error on read/write failure
    */
    bool accumulate(KaraokeStorage* output);

public:
    // 模型窗口在源PCM中的起始帧
    size_t m_start;
    // STFT输入张量，布局1×4×2560×256
    std::vector<float> m_spectrum;
    // 模型伴奏频谱输出张量
    std::vector<float> m_prediction;
    // 任务完成发布标记，完成后才能读取非原子结果
    std::atomic<bool> m_done;
    // 当前窗口处理错误，m_done发布后才能读取
    std::string m_error;
    // 当前窗口STFT分析耗时，毫秒
    int64_t m_analyzeMs;
    // 当前窗口频谱重建耗时，毫秒
    int64_t m_reconstructMs;

private:
    /** 对当前模型窗口计算STFT输入并记录耗时
    */
    void analyze();

private:
    // 借用源PCM临时存储，窗口处理完成前保持有效
    KaraokeStorage* m_source;
    // 源音频PCM总帧数
    size_t m_frames;
    // 包含边界扩展的模型处理总帧数
    size_t m_total;
    // 相邻模型窗口的步进PCM帧数
    size_t m_step;
    // 输入边界扩展PCM帧数
    size_t m_border;
    // 当前窗口有效源PCM帧数
    size_t m_valid;
    // 当前窗口重建后的双声道浮点PCM
    std::vector<float> m_separated;
    // 当前是否执行重建阶段
    bool m_reconstruct;
    // 取消请求标记，不等于任务已经退出
    std::atomic<bool> m_cancel;
};