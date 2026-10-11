#pragma once
#include "LumaPlayerMicrophoneChannel.h"

/** 在现有生命周期线程释放已移除的设备；共享所有权延续到任务退出
*/
class LumaPlayerMicrophoneRetireTask : public CTask
{
public:
    /** 保持待释放麦克风槽位的共享所有权
    @param [in] channel 待释放的有效槽位，任务退出前保留
    */
    explicit LumaPlayerMicrophoneRetireTask(const std::shared_ptr<LumaPlayerMicrophoneChannel>& channel);

    /** 在生命周期线程同步停止写入与采集并发布释放完成
    */
    virtual void DoTask();

    /** 释放步骤不可中断，管理器仍须等待DoTask退出
    */
    virtual void StopTask();

private:
    // 待释放麦克风槽位的共享所有权，保持到任务退出
    std::shared_ptr<LumaPlayerMicrophoneChannel> m_channel;
};