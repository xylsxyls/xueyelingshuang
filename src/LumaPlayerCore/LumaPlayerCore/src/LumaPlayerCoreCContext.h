#pragma once
#include "LumaPlayerCore.h"
#include "LumaPlayerCoreCAudioRenderAdapter.h"
#include "LumaPlayerCoreCVideoRenderAdapter.h"
#include "KaraokeRecordingTracks.h"

#include <mutex>
#include <vector>

/** C句柄的内部资源集合，持有Core和音视频适配器，由销毁接口按顺序释放
*/
struct LumaPlayerCoreCContext
{
public:
    // C++ Core对象
    LumaPlayerCore* m_core;
    // C音频渲染适配器
    LumaPlayerCoreCAudioRenderAdapter* m_audioRender;
    // C视频渲染适配器
    LumaPlayerCoreCVideoRenderAdapter* m_videoRender;
    // Karaoke录音缓存与普通播放状态分开锁定
    std::mutex m_karaokeMutex;
    // 拥有的三路K歌录音时间线，受K歌锁保护
    KaraokeRecordingTracks m_karaokePcmTimeline;
    // 受K歌锁保护的PCM处理暂存缓冲区
    std::vector<int16_t> m_karaokeRecordingScratch;

public:
    /** 构造空句柄上下文，资源由创建接口依次建立
    */
    LumaPlayerCoreCContext();
};