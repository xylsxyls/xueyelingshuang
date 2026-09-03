#pragma once
#include "LoopPlayerPlatform.h"
#include "VideoTimelineProbe.h"

#include <string>
#include <vector>

namespace LoopPlayer
{
    /** 播放引擎状态，用于窗口层判断播放、暂停、结束和错误
    */
    enum PlaybackEngineState
    {
        // 引擎尚未打开媒体
        PlaybackEngineStateClosed = 0,
        // 媒体已打开但处于停止位置
        PlaybackEngineStateStopped,
        // 媒体已打开并暂停在当前位置
        PlaybackEngineStatePaused,
        // 媒体正在按当前时钟播放
        PlaybackEngineStatePlaying,
        // 音视频流均已播放到末尾
        PlaybackEngineStateEnded,
        // 引擎遇到不可继续播放的错误
        PlaybackEngineStateError
    };

    /** 播放引擎异步事件，统一通过窗口消息投递给UI线程
    */
    enum PlaybackEngineEvent
    {
        // 空事件，占位使用
        PlaybackEngineEventNone = 0,
        // seek已经落到目标位置
        PlaybackEngineEventPositionSet,
        // 媒体已经播放结束
        PlaybackEngineEventEnded,
        // 播放引擎出现错误
        PlaybackEngineEventError
    };

    /** 播放引擎初始化参数，集中描述媒体路径、窗口消息和时间轴修正信息
    */
    struct PlaybackEngineInitParam
    {
    public:
        // 待播放文件的完整路径
        std::wstring m_filePath;
        // 视频帧绘制目标窗口
        HWND m_videoWindow;
        // 播放状态事件投递目标窗口
        HWND m_eventWindow;
        // 播放状态事件使用的窗口消息编号
        UINT m_eventMessage;
        // 探测得到的原始视频时间轴，用于修正异常MP4的首帧偏移
        VideoTimelineInfo m_videoTimeline;
        // 原始视频时间戳到播放器逻辑时间戳的偏移量
        REFERENCE_TIME m_videoTimelineOffset;
        // 无法从媒体类型读取帧率时使用的单帧时长
        REFERENCE_TIME m_fallbackFrameDuration;

    public:
        /** 构造默认初始化参数
        */
        PlaybackEngineInitParam();
    };

    /** 解码后的视频帧，保存为GDI可以直接绘制的BGRA像素
    */
    struct PlaybackVideoFrame
    {
    public:
        // BGRA像素数据，按自上而下的行顺序保存
        std::vector<BYTE> m_pixels;
        // 帧宽度
        int m_width;
        // 帧高度
        int m_height;
        // 每行字节数
        int m_stride;
        // 逻辑播放时间戳
        REFERENCE_TIME m_time;
        // 样本持续时间
        REFERENCE_TIME m_duration;

    public:
        /** 构造空视频帧
        */
        PlaybackVideoFrame();
    };

    /** waveOut音频缓冲，封装PCM数据和对应WAVEHDR生命周期
    */
    struct PlaybackAudioBuffer
    {
    public:
        // PCM音频数据，必须在waveOut播放完成前保持有效
        std::vector<BYTE> m_data;
        // waveOut使用的缓冲头
        WAVEHDR m_header;
        // 当前缓冲头是否已经Prepare
        bool m_prepared;

    public:
        /** 构造空音频缓冲
        */
        PlaybackAudioBuffer();
    };

    /** 播放引擎快照，用于窗口层状态同步和详细日志排查
    */
    struct PlaybackEngineSnapshot
    {
    public:
        // 当前播放状态
        PlaybackEngineState m_state;
        // 最近一次底层HRESULT
        HRESULT m_lastResult;
        // 当前逻辑播放位置
        REFERENCE_TIME m_position;
        // 媒体总时长
        REFERENCE_TIME m_duration;
        // 单帧时长
        REFERENCE_TIME m_frameDuration;
        // 当前播放倍速
        double m_playbackRate;
        // 是否打开了视频流
        bool m_videoAvailable;
        // 是否打开了音频流
        bool m_audioAvailable;
        // 视频流是否到达末尾
        bool m_videoEnded;
        // 音频流是否到达末尾
        bool m_audioEnded;

    public:
        /** 构造默认快照
        */
        PlaybackEngineSnapshot();
    };

    /** 把播放状态转换成可写入日志的文本
    @param [in] state 播放引擎状态
    @return 返回状态名称
    */
    const wchar_t* PlaybackEngineStateName(PlaybackEngineState state);

    /** 把播放事件转换成可写入日志的文本
    @param [in] eventType 播放事件类型
    @return 返回事件名称
    */
    const wchar_t* PlaybackEngineEventName(PlaybackEngineEvent eventType);
}