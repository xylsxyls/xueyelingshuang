#pragma once

#include "LumaPlayerMicrophoneCapture.h"
#include "LumaPlayerKaraokeTask.h"

/** UI state and worker lifetime for one stable recording lane
*/
class LumaPlayerMicrophoneChannel
{
public:
    /** 初始化LumaPlayerMicrophoneChannel对象的状态，不转移输入借用对象的所有权
    @param [in] core 借用的Core桥接对象，全部任务退出前不得销毁
    @param [in] slot 麦克风槽位索引，0至2
    */
    LumaPlayerMicrophoneChannel(LumaPlayerCoreBridge* core, int32_t slot);

    /** 释放对象持有的资源，退出前按实现等待其任务或回调
    */
    ~LumaPlayerMicrophoneChannel();

public:
    // 麦克风静音开关，静音不代表关闭设备
    bool m_muted;
    // 麦克风槽位任务是否已初始化
    bool m_initialized;
    // 麦克风显示增益百分比，界面与原生PCM增益按配置换算
    int32_t m_gain;
    // 当前输入设备的枚举索引
    int32_t m_device;
    // 固定录音轨编号，不随活动列表删除重排
    const int32_t m_slot;
    // 生命周期清理完成标记
    std::atomic<bool> m_retired;
    // 麦克风最近状态文本
    QString m_status;
    // 麦克风槽位显示名称
    QString m_label;
    // 拥有的原生采集适配器，停止任务后才销毁
    LumaPlayerMicrophoneCapture m_capture;
    // 拥有的录音生命周期任务，析构前停止并等待退出
    LumaPlayerKaraokeTask m_task;
};