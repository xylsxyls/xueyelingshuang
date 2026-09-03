#pragma once

#include "LoopPlayerPlatform.h"

namespace LoopPlayer
{
    /** 视频流时间轴探测结果，用于识别首帧时间戳偏移和视频真实结束位置
    */
    struct VideoTimelineInfo
    {
        /** 构造默认时间轴探测结果
        */
        VideoTimelineInfo();

        // 是否得到有效视频样本时间轴
        bool isValid;
        // 读取到的视频样本数量
        DWORD sampleCount;
        // 探测过程最后一次HRESULT
        HRESULT result;
        // 首帧原始时间戳
        REFERENCE_TIME firstTime;
        // 第二帧原始时间戳
        REFERENCE_TIME secondTime;
        // 最后一帧原始时间戳
        REFERENCE_TIME lastTime;
        // 最后一帧结束时间戳
        REFERENCE_TIME lastEndTime;
        // 首帧持续时间
        REFERENCE_TIME firstDuration;
        // Media Foundation报告的源媒体总时长
        REFERENCE_TIME sourceDuration;
        // 视频样本之间检测到的最大时间戳间隔
        REFERENCE_TIME maxGap;
        // 最大间隔起点时间戳
        REFERENCE_TIME maxGapStart;
        // 最大间隔终点时间戳
        REFERENCE_TIME maxGapEnd;
    };

    /** 视频时间轴探测器，只负责读取样本时间戳，不参与实际播放
    */
    class VideoTimelineProbe
    {
    public:
        /** 探测视频流样本时间轴
        @param [in] path 媒体文件完整路径
        @param [in] fallbackFrameDuration 无法读取样本持续时间时使用的帧时长
        @param [out] info 输出的视频时间轴信息
        @return 探测到有效视频样本返回true，否则返回false
        */
        static bool Probe(const wchar_t* path, REFERENCE_TIME fallbackFrameDuration, VideoTimelineInfo& info);
    };
}