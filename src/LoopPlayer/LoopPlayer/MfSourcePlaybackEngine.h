#pragma once
#include "LoopPlayerPlatform.h"
#include "PlaybackEngineTypes.h"

#include <string>
#include <vector>

namespace LoopPlayer
{
    /** 基于Media Foundation SourceReader和waveOut的播放引擎，隐藏解码、时钟、音频缓冲和视频帧绘制细节
    */
    class MfSourcePlaybackEngine
    {
    public:
        /** 构造函数，只初始化成员默认值，不打开底层媒体资源
        */
        MfSourcePlaybackEngine();

        /** 析构函数只兜底释放资源，正常调用方应先主动调用uninit
        */
        ~MfSourcePlaybackEngine();

        /** 打开媒体文件并初始化视频解码、音频输出和播放线程
        @param [in] param 播放初始化参数
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT init(const PlaybackEngineInitParam& param);

        /** 关闭媒体文件并释放所有播放线程和底层句柄
        */
        void uninit();

        /** 从当前位置开始播放
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT play();

        /** 暂停播放并保留当前位置
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT pause();

        /** 停止播放并回到0位置
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT stop();

        /** 跳转到指定逻辑时间，视频流会自动映射到原始时间轴
        @param [in] position 目标逻辑时间，单位100ns
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT seek(REFERENCE_TIME position);

        /** 设置播放倍速，视频时钟立即生效，音频会尝试使用waveOut倍速能力
        @param [in] rate 播放倍速
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT setRate(double rate);

        /** 读取当前逻辑播放位置
        @param [out] position 当前逻辑播放位置，单位100ns
        @return 成功返回true，否则返回false
        */
        bool getPosition(REFERENCE_TIME& position);

        /** 读取当前快照，主要用于日志排查
        @param [out] snapshot 当前播放状态快照
        */
        void snapshot(PlaybackEngineSnapshot& snapshot);

        /** 判断当前是否已经打开媒体
        @return 已打开返回true，否则返回false
        */
        bool isOpen() const;

        /** 读取媒体总时长
        @return 媒体总时长，单位100ns
        */
        REFERENCE_TIME duration() const;

        /** 读取单帧时长
        @return 单帧时长，单位100ns
        */
        REFERENCE_TIME frameDuration() const;

        /** 读取视频原始尺寸
        @param [out] width 视频宽度
        @param [out] height 视频高度
        @return 读取成功返回true，否则返回false
        */
        bool getNativeVideoSize(int& width, int& height) const;

        /** 读取当前播放状态
        @return 当前播放状态
        */
        PlaybackEngineState state();

        /** 读取最近一次错误文本
        @return 最近一次错误文本
        */
        std::wstring lastError() const;

        /** 把最近解码的视频帧绘制到目标区域
        @param [in] hdc 目标DC
        @param [in] rect 绘制区域
        */
        void paintVideo(HDC hdc, const RECT& rect);

    private:
        /** 禁止拷贝构造，避免多个播放引擎同时管理同一组底层线程和音频句柄
        @param [in] other 另一个播放引擎
        */
        MfSourcePlaybackEngine(const MfSourcePlaybackEngine& other);

        /** 禁止赋值，避免底层媒体资源所有权被复制
        @param [in] other 另一个播放引擎
        @return 返回当前对象引用
        */
        MfSourcePlaybackEngine& operator=(const MfSourcePlaybackEngine& other);

        /** 创建SourceReader属性
        @param [out] attributes 创建出的属性对象
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT createReaderAttributes(IMFAttributes** attributes);

        /** 创建并配置视频SourceReader
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT openVideoReader();

        /** 创建并配置音频SourceReader
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT openAudioReader();

        /** 读取媒体文件总时长
        @param [in] reader 可用的SourceReader
        */
        void readSourceDuration(IMFSourceReader* reader);

        /** 从媒体类型读取视频尺寸、帧率和步幅
        @param [in] mediaType 当前视频媒体类型
        */
        void readVideoTypeInfo(IMFMediaType* mediaType);

        /** 从媒体类型读取音频PCM格式并打开waveOut
        @param [in] mediaType 当前音频媒体类型
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT openWaveOut(IMFMediaType* mediaType);

        /** 启动视频和音频工作线程
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT startThreads();

        /** 停止工作线程并等待退出
        */
        void stopThreads();

        /** 视频线程入口
        @param [in] param 播放引擎指针
        @return 线程退出码
        */
        static DWORD WINAPI videoThreadProc(LPVOID param);

        /** 音频线程入口
        @param [in] param 播放引擎指针
        @return 线程退出码
        */
        static DWORD WINAPI audioThreadProc(LPVOID param);

        /** 视频线程主循环
        */
        void videoThreadMain();

        /** 音频线程主循环
        */
        void audioThreadMain();

        /** 等待进入播放状态或收到退出请求
        @return 可以继续解码返回true，需要退出返回false
        */
        bool waitForPlayableState();

        /** 从视频SourceReader读取下一帧，内部会按时间戳重排B帧
        @param [out] frame 输出的视频帧
        @param [out] endOfStream 是否到达视频流末尾
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT readNextVideoFrame(PlaybackVideoFrame& frame, bool& endOfStream);

        /** 从视频SourceReader原始读取一个解码样本，调用前必须持有m_readerLock
        @param [out] frame 输出的视频帧
        @param [out] endOfStream 是否到达视频流末尾
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT readNextVideoFrameFromSourceLocked(PlaybackVideoFrame& frame, bool& endOfStream);

        /** 填充视频重排队列，调用前必须持有m_readerLock
        @param [in] minCount 期望队列内至少保留的帧数
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT fillVideoReorderFramesLocked(size_t minCount);

        /** 按显示时间从视频重排队列取出下一帧，调用前必须持有m_readerLock
        @param [out] frame 输出的视频帧
        @return 取到帧返回true，否则返回false
        */
        bool popNextVideoFrameLocked(PlaybackVideoFrame& frame);

        /** 清空视频重排队列，调用前必须持有m_readerLock
        */
        void clearVideoReorderFramesLocked();

        /** 在当前位置读取一帧视频并立刻发布，供seek后预览使用
        @param [in] position 逻辑时间，单位100ns
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT previewVideoFrameAt(REFERENCE_TIME position);

        /** 把视频样本复制成可绘制的BGRA像素
        @param [in] sample 视频样本
        @param [in] sampleTime 样本逻辑时间
        @param [in] sampleDuration 样本持续时间
        @param [out] frame 输出的视频帧
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT copyVideoSample(IMFSample* sample, REFERENCE_TIME sampleTime, REFERENCE_TIME sampleDuration, PlaybackVideoFrame& frame);

        /** 发布视频帧并通知窗口重绘
        @param [in] frame 已解码的视频帧
        */
        void publishVideoFrame(const PlaybackVideoFrame& frame);

        /** 从音频SourceReader读取并写入一个PCM缓冲
        @param [out] endOfStream 是否到达音频流末尾
        @return 成功返回S_OK，失败返回底层HRESULT
        */
        HRESULT readAndQueueAudio(bool& endOfStream);

        /** 查找可复用的waveOut音频缓冲
        @return 可用缓冲下标，找不到返回-1
        */
        int findReusableAudioBuffer();

        /** 释放单个waveOut缓冲头
        @param [in] index 缓冲下标
        */
        void unprepareAudioBuffer(size_t index);

        /** 重置waveOut并释放所有已Prepare的缓冲
        */
        void resetWaveBuffers();

        /** 关闭waveOut音频设备
        */
        void closeWaveOut();

        /** 按当前位置重置播放时钟
        @param [in] position 逻辑时间，单位100ns
        */
        void resetClockLocked(REFERENCE_TIME position);

        /** 计算当前逻辑播放位置，调用前必须持有m_stateLock
        @return 当前逻辑播放位置
        */
        REFERENCE_TIME calculatePositionLocked() const;

        /** 修改播放状态并保存最近一次HRESULT
        @param [in] state 新状态
        @param [in] result 最近一次HRESULT
        */
        void setStateLocked(PlaybackEngineState state, HRESULT result);

        /** 投递播放事件到窗口线程
        @param [in] eventType 播放事件类型
        @param [in] result 事件对应HRESULT
        */
        void postEvent(PlaybackEngineEvent eventType, HRESULT result);

        /** 标记某个流已经播放结束，并在音视频都结束后投递结束事件
        @param [in] videoEnded 是否标记视频流结束
        @param [in] audioEnded 是否标记音频流结束
        */
        void markStreamEnded(bool videoEnded, bool audioEnded);

        /** 读取当前seek序号，用于视频线程丢弃seek前已经读出的旧帧
        @return 当前seek序号
        */
        DWORD readSeekSerial();

        /** 判断播放线程是否应该退出
        @return 需要退出返回true，否则返回false
        */
        bool shouldExit() const;

    private:
        // 保护播放状态、时钟和结束标记
        CRITICAL_SECTION m_stateLock;
        // 保护SourceReader同步读取和seek
        CRITICAL_SECTION m_readerLock;
        // 保护最近视频帧
        CRITICAL_SECTION m_frameLock;
        // 保护waveOut句柄和缓冲
        CRITICAL_SECTION m_waveLock;
        // 媒体文件完整路径
        std::wstring m_filePath;
        // 最近一次错误文本
        std::wstring m_lastError;
        // 视频绘制目标窗口
        HWND m_videoWindow;
        // 播放事件目标窗口
        HWND m_eventWindow;
        // 播放事件消息编号
        UINT m_eventMessage;
        // 视频SourceReader
        IMFSourceReader* m_videoReader;
        // 音频SourceReader
        IMFSourceReader* m_audioReader;
        // 视频线程句柄
        HANDLE m_videoThread;
        // 音频线程句柄
        HANDLE m_audioThread;
        // 唤醒播放线程的事件
        HANDLE m_wakeupEvent;
        // 请求线程退出的事件
        HANDLE m_exitEvent;
        // waveOut音频输出句柄
        HWAVEOUT m_waveOut;
        // waveOut使用的PCM格式
        WAVEFORMATEX m_waveFormat;
        // waveOut循环使用的音频缓冲
        std::vector<PlaybackAudioBuffer> m_audioBuffers;
        // 下一次优先检查的音频缓冲下标
        size_t m_nextAudioBuffer;
        // 最近发布的视频帧
        PlaybackVideoFrame m_currentFrame;
        // 视频帧重排队列，解决B帧时间戳和读取顺序不一致导致的画面倒跳
        std::vector<PlaybackVideoFrame> m_videoReorderFrames;
        // GDI绘制最近视频帧时使用的位图信息
        BITMAPINFO m_videoBitmapInfo;
        // 是否已经完成init
        bool m_isInit;
        // 是否正在退出
        bool m_isExit;
        // 是否存在可播放视频流
        bool m_videoAvailable;
        // 是否存在可播放音频流
        bool m_audioAvailable;
        // 视频流是否已经读到末尾
        bool m_videoEnded;
        // 音频流是否已经读到末尾
        bool m_audioEnded;
        // 视频SourceReader是否已经读到末尾，重排队列排空后才算真正播放结束
        bool m_videoSourceEnded;
        // 当前播放状态
        PlaybackEngineState m_state;
        // 最近一次底层HRESULT
        HRESULT m_lastResult;
        // seek序号，每次seek递增，用于丢弃seek前遗留的视频帧
        DWORD m_seekSerial;
        // 媒体总时长
        REFERENCE_TIME m_duration;
        // 当前暂停或停止时的逻辑播放位置
        REFERENCE_TIME m_clockBasePosition;
        // 单帧时长
        REFERENCE_TIME m_frameDuration;
        // 原始视频时间戳偏移量
        REFERENCE_TIME m_videoTimelineOffset;
        // 当前播放倍速
        double m_playbackRate;
        // 性能计数器频率
        LARGE_INTEGER m_counterFrequency;
        // 当前播放时钟起点
        LARGE_INTEGER m_clockBaseCounter;
        // 视频宽度
        int m_videoWidth;
        // 视频高度
        int m_videoHeight;
        // 视频源步幅
        LONG m_videoStride;
        // 已记录的视频样本数量
        DWORD m_videoSampleLogCount;
        // 已记录的音频样本数量
        DWORD m_audioSampleLogCount;
        // 已记录的视频重排异常数量，避免异常文件刷屏
        DWORD m_videoReorderLogCount;
        // 最近发布的视频帧时间，用于日志发现画面倒跳
        REFERENCE_TIME m_lastPublishedVideoTime;
    };
}