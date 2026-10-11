#pragma once
#include "miniaudio/miniaudio.h"

/** Internal native resources, initialized and released only by the audio lifecycle owner
*/
struct KaraokeNativeBackend
{
public:
    // 拥有的miniaudio枚举上下文
    ma_context m_context;
    // 拥有的原生双工音频设备
    ma_device m_device;
    // 已选择的原生输入设备标识
    ma_device_id m_inputId;
    // miniaudio上下文是否成功初始化
    bool m_contextReady;
    // miniaudio双工设备是否成功初始化
    bool m_deviceReady;
    // 请求的设备周期，毫秒
    int32_t m_periodMs;

    /** Zero initialized backend, does not open resources
    */
    KaraokeNativeBackend();
};