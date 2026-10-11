#pragma once

#include <stdint.h>

/** Pack microphone gain and independent output flags into one atomic control word
*/
class KaraokeMicrophoneControls
{
public:
    /** Encode 0..5000 percent gain without overlapping mute or monitor flags
    @param [in] gainPercent 实际PCM增益百分比，0至5000，调用方先校验
    @param [in] muted 麦克风静音标记，非零表示静音
    @param [in] monitoring 是否把麦克风PCM直接送入输出设备
    @return 打包控制字，不覆盖其他标记位
    */
    static uint32_t encode(int32_t gainPercent, bool muted, bool monitoring);

    /** Extract the encoded software gain percentage
    @param [in] controls 打包后的增益和独立控制标记
    @return 软件增益百分比，0至5000
    */
    static int32_t gain(uint32_t controls);

    /** Read the encoded microphone mute flag
    @param [in] controls 打包后的增益和独立控制标记
    @return true表示静音
    */
    static bool muted(uint32_t controls);

    /** Read the encoded speaker-monitor flag
    @param [in] controls 打包后的增益和独立控制标记
    @return true表示开启监听
    */
    static bool monitoring(uint32_t controls);
};