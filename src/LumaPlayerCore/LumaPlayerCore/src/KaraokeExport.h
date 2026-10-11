#pragma once

#include "LumaPlayerCoreC.h"
#include <memory>

class KaraokeStorage;

/** 录音导出实现，借用Core会话并拥有导出期间的临时流，不承担UI或设备采集
*/
class KaraokeExport
{
public:
    /** 同步导出完整歌曲，普通错误及第三方异常转为C结果码
    @param [in] handle 有效Core会话，调用期间不得销毁
    @param [in] source 原视频UTF-8路径，借用至返回
    @param [in] destination 输出MP3或MP4路径，借用至返回
    @param [in] includeBacking 非零时混合背景音
    @param [in] includeVideo 非零时编码原视频画面
    @param [in] pitch 背景音升降调，-7至7半音
    @param [in] gain 背景音增益百分比，0至500
    @param [in] backing 可为空的伴奏句柄，非空时必须对应source
    @param [in] callback 可为空的同步进度回调，返回零取消
    @param [in] user 回调借用上下文，调用期间有效
    @param [out] error 可为空的诊断缓冲区
    @param [in] errorCapacity 诊断容量，包含终止符
    @return C结果码，成功不表示听感已由人工验证
    */
    static int32_t run(LumaPlayerCoreHandle handle, const char* source, const char* destination,
        int32_t includeBacking, int32_t includeVideo, int32_t pitch, int32_t gain,
        LumaPlayerCoreCBacking backing, LumaPlayerCoreCSeparationProgress callback,
        void* user, char* error, size_t errorCapacity);

private:
    /** 有界复制错误文本，不额外分配或抛出异常
    @param [in] message 可为空的错误文本，空时使用默认说明
    @param [out] error 可为空的缓冲区
    @param [in] capacity 缓冲区字节数，包含终止符
    @param [in] result 要返回的错误码
    @return 输入的result
    */
    static int32_t exportFailure(const char* message, char* error, size_t capacity,
        int32_t result = LumaPlayerCoreCResultInternalError);

    /** 同步报告导出进度并处理取消
    @param [in] callback 可为空的回调，零返回值表示取消
    @param [in] user 借用的回调上下文
    @param [in] percent 0至100的进度百分比
    @param [out] error 可为空的错误缓冲区
    @param [in] capacity 缓冲区字节数
    @return Success或Canceled
    */
    static int32_t progress(LumaPlayerCoreCSeparationProgress callback, void* user,
        int percent, char* error, size_t capacity);

    /** 将PCM计算值饱和到S16范围
    @param [in] value 待转换的采样幅度
    @return 限制到-32768至32767的采样值
    */
    static int16_t saturated(double value);

    /** 解码原唱或指定伴奏为48000Hz双声道S16临时流
    @param [in] source 原视频UTF-8路径，借用至返回
    @param [in] backing 可为空，非空时借用匹配source的伴奏
    @param [in] callback 可为空的同步进度回调，零返回值表示取消
    @param [in] user 调用期间有效的借用上下文
    @param [out] output 成功时获得临时PCM存储的共享所有权，不可为空
    @param [out] error 可为空的诊断缓冲区
    @param [in] errorCapacity 诊断容量，包含终止符
    @return C结果码
    */
    static int32_t decodeBacking(const char* source, LumaPlayerCoreCBacking backing,
        LumaPlayerCoreCSeparationProgress callback, void* user,
        std::shared_ptr<KaraokeStorage>* output, char* error, size_t errorCapacity);
};