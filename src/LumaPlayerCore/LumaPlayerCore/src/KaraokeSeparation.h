#pragma once

#include <stdint.h>
#include <stddef.h>
#include <memory>
#include <string>
#include <vector>
#include "KaraokeStorage.h"

/** Return nonzero to continue; callback runs on the caller's worker, never the UI
*/
typedef int (*KaraokeSeparationProgress)(int percent, void* user);

/** Immutable stereo 44.1 kHz backing, indexed by original media time
*/
struct KaraokeBacking
{
    // 源媒体路径，分离/导出任务拥有字符串
    std::string m_source;
    // 拥有的44100Hz双声道浮点伴奏临时存储
    std::shared_ptr<KaraokeStorage> m_samples;
};

class KaraokeSeparation
{
public:
    /** 同步执行人声分离，返回伴奏共享数据而不替换原唱
    @param [in] source 原媒体来源或PCM存储，借用期覆盖整个调用
    @param [in] runtime ONNX Runtime路径，借用至分离返回
    @param [in] model ONNX模型路径，借用至分离返回
    @param [in] maxDecodedBytes 工作内存预算，字节，按分离入口校验
    @param [in] progress 同步进度回调，可为空，零返回值表示取消
    @param [in] user 借用的回调上下文，同步调用返回或异步完成前保持有效
    @param [out] error 可为空的诊断输出缓冲区或对象
    @param [in] windowFrames 模型单次输入帧数，当前固定261120
    @param [in] overlapFrames 相邻模型窗口重叠帧数，小于windowFrames
    @return 成功时返回拥有伴奏数据的共享指针，失败或取消为空并写入诊断
    */
    static std::shared_ptr<const KaraokeBacking> run(const char* source, const char* runtime,
        const char* model, size_t maxDecodedBytes, KaraokeSeparationProgress progress,
        void* user, std::string* error, int windowFrames = 261120, int overlapFrames = 130560);
};