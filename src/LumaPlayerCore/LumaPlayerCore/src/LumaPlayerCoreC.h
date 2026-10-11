#pragma once
#include <stddef.h>
#include <stdint.h>
#include "LumaPlayerCoreMacro.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** 不透明的独立Core句柄，由创建接口分配，调用方不得解引用或自行释放
    */
    typedef struct LumaPlayerCoreCContext* LumaPlayerCoreHandle;

    /** C接口运行配置，先调用默认配置函数，再按需要修改字段
    */
    typedef struct LumaPlayerCoreCConfig
    {
        // 同步控制命令等待上限，单位毫秒，必须大于0
        int32_t m_controlCommandTimeoutMs;
        // 打开媒体等待上限，单位毫秒，必须大于0
        int32_t m_openCommandTimeoutMs;
        // 空闲任务等待间隔，单位毫秒，必须大于0
        int32_t m_idleWaitMs;
        // 视频提前呈现容差，单位100纳秒，必须大于等于0
        int64_t m_videoEarlyTolerance100ns;
        // 音频预写缓冲范围，单位100纳秒，必须大于等于0
        int64_t m_audioLead100ns;
        // 单份循环缓存预算，单位字节，范围1到1073741824，至少保存一帧
        int64_t m_loopBufferMaxBytes;
        // 缺少有效媒体帧时长时的兜底值，单位100纳秒，必须大于0，默认400000对应25fps
        int64_t m_defaultFrameDuration100ns;
        // 初始化及成功打开媒体时的默认播放倍率，默认1000表示1.0倍率，必须位于配置范围内
        int32_t m_defaultRatePermille;
        // 最低播放倍率，必须大于0，默认100表示0.1倍率，1000表示1.0倍率
        int32_t m_minRatePermille;
        // 最高播放倍率，不小于最低倍率，默认5000表示5.0倍率，1000表示1.0倍率
        int32_t m_maxRatePermille;
    } LumaPlayerCoreCConfig;

    /** 获取C接口默认配置
    @param [out] config 配置对象，不能为空，init时复制后不再借用
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreDefaultConfig(LumaPlayerCoreCConfig* config);

    /** 复制外部配置并初始化Core
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] config 配置对象，不能为空，init时复制后不再借用
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreInitWithConfig(LumaPlayerCoreHandle handle, const LumaPlayerCoreCConfig* config);

    /** C接口返回值，与C++接口的LumaPlayerCoreResult数值保持一致
    */
    typedef enum LumaPlayerCoreCResult
    {
        // 操作成功
        LumaPlayerCoreCResultSuccess = 0,
        // 参数不符合接口要求，请检查空指针、文件路径和配置范围
        LumaPlayerCoreCResultInvalidParam = 1,
        // 播放器尚未初始化或已经反初始化，请先成功调用init
        LumaPlayerCoreCResultNotInit = 2,
        // 当前没有打开可播放的媒体文件
        LumaPlayerCoreCResultNotOpen = 3,
        // 媒体打开失败，请检查文件是否存在、访问权限以及媒体格式
        LumaPlayerCoreCResultOpenFailed = 4,
        // 当前播放状态或AB端点范围不允许执行该操作
        LumaPlayerCoreCResultStateError = 5,
        // 请求已被更新的预览请求替代或调用方取消
        LumaPlayerCoreCResultCanceled = 6,
        // 等待工作线程完成命令超时，命令可能仍在执行
        LumaPlayerCoreCResultTimeout = 7,
        // 播放器内部发生异常，无法完成当前操作
        LumaPlayerCoreCResultInternalError = 8,
        // 已到媒体流结尾，没有下一帧
        LumaPlayerCoreCResultEnd = 9,
        // 创建播放或预览工作线程时发生异常
        LumaPlayerCoreCResultThreadInitException = 10,
        // 任务管理器未能创建播放或预览工作线程
        LumaPlayerCoreCResultThreadInitFailed = 11,
        // 创建播放工作任务失败，无法分配内存
        LumaPlayerCoreCResultWorkerAllocationFailed = 12,
        // 为播放工作任务分配共享管理对象失败
        LumaPlayerCoreCResultWorkerOwnershipFailed = 13,
        // 工作线程初始化后无法获取对应任务线程对象
        LumaPlayerCoreCResultThreadUnavailable = 14,
        // 创建同步命令结果失败，无法分配内存
        LumaPlayerCoreCResultResultAllocationFailed = 15,
        // 为同步命令结果分配共享管理对象失败
        LumaPlayerCoreCResultResultOwnershipFailed = 16,
        // 命令队列入队时发生异常，控制请求未被接受
        LumaPlayerCoreCResultCommandQueueFailed = 17,
        // 预览任务线程不存在或已经停止，无法提交预览
        LumaPlayerCoreCResultPreviewThreadUnavailable = 18,
        // 创建预览任务失败，无法分配内存
        LumaPlayerCoreCResultPreviewAllocationFailed = 19,
        // 为预览任务分配共享管理对象失败
        LumaPlayerCoreCResultPreviewOwnershipFailed = 20,
        // 设置A点失败，无法确定目标画面的真实展示区间
        LumaPlayerCoreCResultLoopAFrameUnavailable = 21,
        // A点帧起点必须早于B点帧的排他结束边界，原端点保持不变
        LumaPlayerCoreCResultLoopAOutOfRange = 22,
        // 设置B点失败，无法确定目标画面的真实结束边界
        LumaPlayerCoreCResultLoopBFrameUnavailable = 23,
        // 设置B点时无法定位媒体起点画面，自动建立A点失败
        LumaPlayerCoreCResultLoopAutoAUnavailable = 24,
        // B点帧的排他结束边界必须晚于A点帧起点，原端点保持不变
        LumaPlayerCoreCResultLoopBOutOfRange = 25,
        // 要移动的AB端点尚未设置，请先设置该端点
        LumaPlayerCoreCResultLoopPointNotSet = 26,
        // 读取媒体信息失败，请检查文件路径、权限和封装格式
        LumaPlayerCoreCResultMediaInfoOpenFailed = 27,
        // 创建视频或预览解码器失败，无法分配内存
        LumaPlayerCoreCResultVideoAllocationFailed = 28,
        // 视频解码器打开失败，视频流可能损坏或编码不受支持
        LumaPlayerCoreCResultVideoOpenFailed = 29,
        // 预览解码器打开失败，无法执行定位和逐帧预览
        LumaPlayerCoreCResultPreviewOpenFailed = 30,
        // 创建音频解码器失败，无法分配内存
        LumaPlayerCoreCResultAudioAllocationFailed = 31,
        // 视频定位失败，无法跳转到请求的媒体位置
        LumaPlayerCoreCResultVideoSeekFailed = 32,
        // 音频定位失败，无法与目标视频位置重新同步
        LumaPlayerCoreCResultAudioSeekFailed = 33,
        // 目标位置预览解码失败，未能生成有效画面
        LumaPlayerCoreCResultPreviewDecodeFailed = 34,
        // 预览解码器尚未打开，无法读取相邻展示帧
        LumaPlayerCoreCResultPreviewReaderUnavailable = 35,
        // 逐帧定位失败，无法回退到可解码的视频位置
        LumaPlayerCoreCResultAdjacentSeekFailed = 36,
        // 下一展示帧解码失败，原AB端点保持不变
        LumaPlayerCoreCResultNextFrameDecodeFailed = 37,
        // 未找到下一展示帧，可能已到媒体末帧
        LumaPlayerCoreCResultNextFrameUnavailable = 38,
        // 上一展示帧解码失败，原AB端点保持不变
        LumaPlayerCoreCResultPreviousFrameDecodeFailed = 39,
        // 未找到上一展示帧，可能已到媒体首帧或超出回溯范围
        LumaPlayerCoreCResultPreviousFrameUnavailable = 40,
        // 播放视频读取或解码失败，媒体数据可能损坏
        LumaPlayerCoreCResultVideoReadFailed = 41,
        // 播放音频读取或解码失败，媒体数据可能损坏
        LumaPlayerCoreCResultAudioReadFailed = 42,
        // 音频解码器打开失败，无法播放媒体声音
        LumaPlayerCoreCResultAudioOpenFailed = 43,
        // 视频输出设备打开失败，无法呈现画面
        LumaPlayerCoreCResultVideoRenderOpenFailed = 44,
        // 音频输出设备打开失败，无法播放声音
        LumaPlayerCoreCResultAudioRenderOpenFailed = 45,
        // 视频输出回调拒绝当前画面
        LumaPlayerCoreCResultVideoRenderFailed = 46,
        // 音频输出回调拒绝当前音频片段
        LumaPlayerCoreCResultAudioRenderFailed = 47
    } LumaPlayerCoreCResult;

    /** C接口播放状态，与C++接口的LumaPlayerCoreState数值保持一致
    */
typedef enum LumaPlayerCoreCState
    {
        // 没有打开媒体
        LumaPlayerCoreCStateClosed = 0,
        // 媒体已打开，但播放位置在开始或被停止
        LumaPlayerCoreCStateStopped = 1,
        // 正在播放
        LumaPlayerCoreCStatePlaying = 2,
        // 已暂停，播放位置保持在当前帧
        LumaPlayerCoreCStatePaused = 3,
        // 正在执行seek或预览
        LumaPlayerCoreCStateSeeking = 4,
        // 播放到文件末尾，后续会根据循环设置决定是否重播
        LumaPlayerCoreCStateEnded = 5,
        // 播放核心发生错误
        LumaPlayerCoreCStateError = 6,
        // 正在工作线程打开媒体，调用线程可以继续处理界面事件
        LumaPlayerCoreCStateOpening = 7
    } LumaPlayerCoreCState;

    /** C接口循环点类型，与C++接口的LumaPlayerCoreLoopPointType数值保持一致
    */
    typedef enum LumaPlayerCoreCLoopPointType
    {
        // A点表示循环左边界
        LumaPlayerCoreCLoopPointA = 0,
        // B点表示循环右边界
        LumaPlayerCoreCLoopPointB = 1
    } LumaPlayerCoreCLoopPointType;

    /** C接口音频格式
    */
    typedef struct LumaPlayerCoreCAudioFormat
    {
        // PCM采样率
        int32_t m_sampleRate;
        // PCM声道数
        int32_t m_channels;
        // 单个采样点位深
        int32_t m_bitsPerSample;
    } LumaPlayerCoreCAudioFormat;

    /** C接口视频格式
    */
    typedef struct LumaPlayerCoreCVideoFormat
    {
        // 视频宽度
        int32_t m_width;
        // 视频高度
        int32_t m_height;
    } LumaPlayerCoreCVideoFormat;

    /** C接口视频帧，回调返回后数据指针立即失效
    */
    typedef struct LumaPlayerCoreCVideoFrame
    {
        // BGRA像素数据指针
        const unsigned char* m_bgraPixels;
        // BGRA像素数据字节数
        size_t m_bgraSize;
        // 帧宽度
        int32_t m_width;
        // 帧高度
        int32_t m_height;
        // 每行字节数
        int32_t m_stride;
        // 帧起始时间，单位100纳秒
        int64_t m_timestamp100ns;
        // 帧显示时长，单位100纳秒
        int64_t m_duration100ns;
        // Core侧帧索引，未知时为-1
        int64_t m_frameIndex;
        // 是否关键帧
        int32_t m_keyFrame;
    } LumaPlayerCoreCVideoFrame;

    /** C接口音频帧，回调返回后数据指针立即失效
    */
    typedef struct LumaPlayerCoreCAudioFrame
    {
        // PCM数据指针
        const unsigned char* m_pcmData;
        // PCM数据字节数
        size_t m_pcmSize;
        // PCM格式
        LumaPlayerCoreCAudioFormat m_format;
        // 音频片段起始时间，单位100纳秒
        int64_t m_timestamp100ns;
        // 音频片段时长，单位100纳秒
        int64_t m_duration100ns;
    } LumaPlayerCoreCAudioFrame;

    /** C接口播放快照，保留界面和测试常用字段
    */
    typedef struct LumaPlayerCoreCSnapshot
    {
        // 当前播放状态
        int32_t m_state;
        // 成功打开媒体后递增的序号，用于识别异步打开完成
        uint64_t m_mediaSerial;
        // 媒体总时长，单位100纳秒
        int64_t m_duration100ns;
        // 当前播放位置，单位100纳秒
        int64_t m_position100ns;
        // 视频默认帧时长，单位100纳秒，仅用于界面移动预估
        int64_t m_defaultVideoFrameDuration100ns;
        // 播放倍速，1000表示1.0倍速
        int32_t m_ratePermille;
        // 是否存在视频流
        int32_t m_hasVideo;
        // 是否存在音频流
        int32_t m_hasAudio;
        // 视频宽度
        int32_t m_videoWidth;
        // 视频高度
        int32_t m_videoHeight;
        // A点是否设置
        int32_t m_hasLoopA;
        // B点是否设置
        int32_t m_hasLoopB;
        // A点帧起始时间，单位100纳秒
        int64_t m_loopAStart100ns;
        // B点帧结束边界，单位100纳秒
        int64_t m_loopBEnd100ns;
        // 最近完成的操作或后台播放错误码
        int32_t m_result;
        /* Recording clock and discontinuity; callers must rebuild with this ABI */
        // 录制状态代次，用于识别新的录制会话
        uint64_t m_recordingSerial;
        // 最近录制区间媒体位置，100纳秒单位
        int64_t m_recordingPosition100ns;
    } LumaPlayerCoreCSnapshot;

    /** C接口打开音频回调
    */
    typedef int32_t (*LumaPlayerCoreCOpenAudioCallback)(const LumaPlayerCoreCAudioFormat* format, void* userData);

    /** C接口关闭音频回调
    */
    typedef void (*LumaPlayerCoreCCloseAudioCallback)(void* userData);

    /** C接口清空音频缓存回调
    */
    typedef void (*LumaPlayerCoreCFlushAudioCallback)(void* userData);

    /** C接口暂停音频设备并保留缓存回调
    */
    typedef void (*LumaPlayerCoreCPauseAudioCallback)(void* userData);

    /** C接口恢复音频设备回调
    */
    typedef void (*LumaPlayerCoreCResumeAudioCallback)(void* userData);

    /** C接口写音频回调
    */
    typedef int32_t (*LumaPlayerCoreCRenderAudioCallback)(const LumaPlayerCoreCAudioFrame* frame, void* userData);

    /** C接口查询音频缓存时长回调
    */
    typedef int64_t (*LumaPlayerCoreCBufferedAudioCallback)(void* userData);

    /** C接口打开视频回调
    */
    typedef int32_t (*LumaPlayerCoreCOpenVideoCallback)(const LumaPlayerCoreCVideoFormat* format, void* userData);

    /** C接口关闭视频回调
    */
    typedef void (*LumaPlayerCoreCCloseVideoCallback)(void* userData);

    /** C接口显示视频帧回调
    */
    typedef int32_t (*LumaPlayerCoreCRenderVideoCallback)(const LumaPlayerCoreCVideoFrame* frame, void* userData);

    /** C接口渲染回调集合，回调只可提交异步控制，不得同步等待或执行init/uninit/destroy
    */
    typedef struct LumaPlayerCoreCRenderCallbacks
    {
        // 音频打开回调，可以为空
        LumaPlayerCoreCOpenAudioCallback m_openAudio;
        // 音频关闭回调，可以为空
        LumaPlayerCoreCCloseAudioCallback m_closeAudio;
        // 音频清空回调，可以为空
        LumaPlayerCoreCFlushAudioCallback m_flushAudio;
        // 音频暂停回调，可以为空
        LumaPlayerCoreCPauseAudioCallback m_pauseAudio;
        // 音频恢复回调，可以为空
        LumaPlayerCoreCResumeAudioCallback m_resumeAudio;
        // 音频写入回调，可以为空
        LumaPlayerCoreCRenderAudioCallback m_renderAudio;
        // 音频缓存查询回调，可以为空
        LumaPlayerCoreCBufferedAudioCallback m_bufferedAudio;
        // 视频打开回调，可以为空
        LumaPlayerCoreCOpenVideoCallback m_openVideo;
        // 视频关闭回调，可以为空
        LumaPlayerCoreCCloseVideoCallback m_closeVideo;
        // 视频显示回调，可以为空
        LumaPlayerCoreCRenderVideoCallback m_renderVideo;
        // 调用方用户数据，Core只透传不释放
        void* m_userData;
    } LumaPlayerCoreCRenderCallbacks;

    /** 创建未初始化的Core句柄，必须由Destroy释放
    @return 新句柄，创建失败返回空指针
    */
    LumaPlayerCoreAPI LumaPlayerCoreHandle lumaPlayerCoreCreate(void);

    /** 停止Core并释放句柄，返回前等待内部任务结束，之后句柄失效
    @param [in] handle 库创建的Core句柄，必须保持有效
    */
    LumaPlayerCoreAPI void lumaPlayerCoreDestroy(LumaPlayerCoreHandle handle);

    /** 初始化Core侧K歌PCM录音时间线；输入回调线程应先经有界队列聚合，不要直接调用该分配型接口
    @param [in] handle 有效Core句柄
    @param [in] sampleRate PCM采样率，单位Hz
    @param [in] channels PCM声道数
    @param [in] maxPcmBytes 非0为PCM驻内存上限；0为临时磁盘分块模式，写入失败保留已提交录音
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreKaraokeInit(LumaPlayerCoreHandle handle,
        int32_t sampleRate, int32_t channels, size_t maxPcmBytes);

    /** 清空录音样本并保留当前格式及容量配置
    @param [in] handle 有效Core句柄
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreKaraokeClear(LumaPlayerCoreHandle handle);

    /** 将媒体时间线推进到目标位置；未录区间在输出中生成零PCM
    @param [in] handle 有效Core句柄
    @param [in] mediaPosition100ns 媒体目标位置，单位100纳秒
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreKaraokeAdvanceTo(LumaPlayerCoreHandle handle,
        int64_t mediaPosition100ns);

    /** 复制一段交错S16 PCM并替换媒体时间线上相交的旧录音样本
    @param [in] handle 有效Core句柄
    @param [in] mediaStart100ns 新片段媒体起点，包含
    @param [in] mediaEnd100ns 新片段媒体终点，排他
    @param [in] ratePermille 采集时播放倍率，1000表示1倍
    @param [in] interleavedPcm 交错有符号16位PCM样本，数据仅在调用中借用
    @param [in] frameCount 每声道采样帧数
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreKaraokeWritePcm(LumaPlayerCoreHandle handle,
        int64_t mediaStart100ns, int64_t mediaEnd100ns, int32_t ratePermille,
        const int16_t* interleavedPcm, size_t frameCount);

    /** Process one microphone PCM block for monitoring and optional recording
    @param [in] handle 有效Core句柄
    @param [in] mediaStart100ns 播放中本块对应媒体起点；暂停时忽略
    @param [in] mediaEnd100ns 播放中本块对应媒体终点；暂停时忽略
    @param [in] ratePermille 本块播放倍率，1000表示1.0倍
    @param [in] inputPcm 麦克风交错S16 PCM，数据仅在调用中借用
    @param [in] frameCount 每声道采样帧数，必须大于0
    @param [in] gainPercent 实际PCM麦克风增益，范围0至5000
    @param [in] microphoneMuted 非0时录音和监听均静音
    @param [in] monitoringEnabled 非0时产生监听PCM
    @param [in] recordingEnabled 非0时请求录入时间线
    @param [in] playbackRunning 非0时推进媒体时间并允许录音；暂停时仍可监听但不录音
    @param [out] monitorPcm 接收监听交错S16 PCM
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreKaraokeProcessMicrophone(
        LumaPlayerCoreHandle handle, int64_t mediaStart100ns, int64_t mediaEnd100ns,
        int32_t ratePermille, const int16_t* inputPcm, size_t frameCount,
        int32_t gainPercent, int32_t microphoneMuted, int32_t monitoringEnabled,
        int32_t recordingEnabled, int32_t playbackRunning, int16_t* monitorPcm);

    /** Process an independent microphone lane (0..2); export mixes all lanes
    @param [in] handle 有效Core会话句柄，调用期间不得销毁
    @param [in] microphone 麦克风槽位索引，0至2
    @param [in] mediaStart100ns 原媒体区间起点，100纳秒单位
    @param [in] mediaEnd100ns 原媒体区间终点，100纳秒单位，须大于起点
    @param [in] ratePermille 播放速率千分值，1000为1倍，范围由配置校验
    @param [in] inputPcm 借用的S16输入，长度为frameCount乘声道数
    @param [in] frameCount PCM帧数
    @param [in] gainPercent 麦克风软件增益百分比，0至5000，界面显示值与此不同
    @param [in] microphoneMuted 是否静音麦克风
    @param [in] monitoringEnabled 是否启用监听输出
    @param [in] recordingEnabled 是否启用录音
    @param [in] playbackRunning 是否处于播放状态，暂停时不写入录音
    @param [out] monitorPcm 接收监听S16样本的缓冲区，长度为frameCount乘声道数
    @return 零表示同步操作成功，非零为具体C错误码
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreKaraokeProcessMicrophoneSlot(
        LumaPlayerCoreHandle handle, int32_t microphone, int64_t mediaStart100ns,
        int64_t mediaEnd100ns, int32_t ratePermille, const int16_t* inputPcm,
        size_t frameCount, int32_t gainPercent, int32_t microphoneMuted,
        int32_t monitoringEnabled, int32_t recordingEnabled, int32_t playbackRunning,
        int16_t* monitorPcm);

    /** 查询完整输出PCM的每声道采样帧数
    @param [in] handle 有效Core句柄
    @param [out] frameCount 接收帧数的指针
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreKaraokeOutputFrameCount(
        LumaPlayerCoreHandle handle, uint64_t* frameCount);

    /** Return retained capture frames, excluding generated silence
    @param [in] handle 有效Core会话句柄，调用期间不得销毁
    @param [out] frameCount 不可为空，接收PCM帧数
    @return 零表示同步操作成功，非零为具体C错误码
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreKaraokeCapturedFrameCount(LumaPlayerCoreHandle handle, uint64_t* frameCount);

    /** 按输出帧位置分块读取PCM，未录制的媒体区间返回零样本
    @param [in] handle 有效Core句柄
    @param [in] startFrame 输出起始帧序号
    @param [in] frameCount 每声道要读取的帧数
    @param [out] interleavedOutput 接收交错S16 PCM样本的缓冲区
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreKaraokeReadPcm(LumaPlayerCoreHandle handle,
        uint64_t startFrame, size_t frameCount, int16_t* interleavedOutput);

    /** 按默认配置初始化Core工作任务
    @param [in] handle 库创建的Core句柄，必须保持有效
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreInit(LumaPlayerCoreHandle handle);

    /** 释放媒体和工作任务，保留句柄以便重新初始化
    @param [in] handle 库创建的Core句柄，必须保持有效
    */
    LumaPlayerCoreAPI void lumaPlayerCoreUninit(LumaPlayerCoreHandle handle);

    /** 设置业务Core的LogManager日志开关，Engine不写日志
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] enabled 非0开启日志，0关闭日志，默认关闭
    */
    LumaPlayerCoreAPI void lumaPlayerCoreSetLogEnabled(LumaPlayerCoreHandle handle, int32_t enabled);

    /** 仅在init前或uninit后复制音视频回调表，用户数据必须覆盖Core回调的生命周期
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] callbacks 回调表，允许为空以解除注册；注册时复制，userData不转移所有权
    */
    LumaPlayerCoreAPI void lumaPlayerCoreSetRenderCallbacks(LumaPlayerCoreHandle handle, const LumaPlayerCoreCRenderCallbacks* callbacks);

    /** 同步打开媒体并准备首帧
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] filePath 以0结尾的UTF-8媒体路径，不能为空或空字符串
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreOpenMedia(LumaPlayerCoreHandle handle, const char* filePath);

    /** 提交打开媒体请求，完成状态和错误码通过快照读取
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] filePath 以0结尾的UTF-8媒体路径，不能为空或空字符串
    @return 返回命令提交结果，0表示已接收，最终结果读取快照m_result
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreOpenMediaAsync(LumaPlayerCoreHandle handle, const char* filePath);

    /** 关闭当前媒体并释放解码资源
    @param [in] handle 库创建的Core句柄，必须保持有效
    */
    LumaPlayerCoreAPI void lumaPlayerCoreCloseMedia(LumaPlayerCoreHandle handle);

    /** 同步开始或继续播放
    @param [in] handle 库创建的Core句柄，必须保持有效
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCorePlay(LumaPlayerCoreHandle handle);

    /** 提交开始或继续播放请求
    @param [in] handle 库创建的Core句柄，必须保持有效
    @return 返回命令提交结果，0表示已接收，最终结果读取快照m_result
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCorePlayAsync(LumaPlayerCoreHandle handle);

    /** 同步暂停播放并保留音频缓冲
    @param [in] handle 库创建的Core句柄，必须保持有效
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCorePause(LumaPlayerCoreHandle handle);

    /** 提交暂停播放请求
    @param [in] handle 库创建的Core句柄，必须保持有效
    @return 返回命令提交结果，0表示已接收，最终结果读取快照m_result
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCorePauseAsync(LumaPlayerCoreHandle handle);

    /** 停止播放并回到媒体起点
    @param [in] handle 库创建的Core句柄，必须保持有效
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreStop(LumaPlayerCoreHandle handle);

    /** 同步定位到请求的媒体位置
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] position100ns 媒体位置，单位100纳秒，将裁剪到有效时长内
    @param [in] keepPlayState 非0表示定位完成后播放，0表示暂停
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreSeekTo(LumaPlayerCoreHandle handle, int64_t position100ns, int32_t keepPlayState);

    /** 提交定位请求，更新请求会替代旧预览
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] position100ns 媒体位置，单位100纳秒，将裁剪到有效时长内
    @param [in] keepPlayState 非0表示定位完成后播放，0表示暂停
    @return 返回命令提交结果，0表示已接收，最终结果读取快照m_result
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreSeekToAsync(LumaPlayerCoreHandle handle, int64_t position100ns, int32_t keepPlayState);

    /** 同步预览请求位置的画面
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] position100ns 媒体位置，单位100纳秒，将裁剪到有效时长内
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCorePreviewFrame(LumaPlayerCoreHandle handle, int64_t position100ns);

    /** 提交可取消的画面预览请求
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] position100ns 媒体位置，单位100纳秒，将裁剪到有效时长内
    @return 返回命令提交结果，0表示已接收，最终结果读取快照m_result
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCorePreviewFrameAsync(LumaPlayerCoreHandle handle, int64_t position100ns);

    /** 按当前显示画面设置循环A点
    @param [in] handle 库创建的Core句柄，必须保持有效
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreSetLoopAByCurrentFrame(LumaPlayerCoreHandle handle);

    /** 同步按请求位置的实际展示帧设置A点
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] position100ns 媒体位置，单位100纳秒，将裁剪到有效时长内
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreSetLoopAAtPosition(LumaPlayerCoreHandle handle, int64_t position100ns);

    /** 提交按请求位置设置A点的命令
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] position100ns 媒体位置，单位100纳秒，将裁剪到有效时长内
    @return 返回命令提交结果，0表示已接收，最终结果读取快照m_result
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreSetLoopAAtPositionAsync(LumaPlayerCoreHandle handle, int64_t position100ns);

    /** 按当前显示画面的完整结束边界设置B点，未设置A时自动设置媒体起点
    @param [in] handle 库创建的Core句柄，必须保持有效
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreSetLoopBByCurrentFrame(LumaPlayerCoreHandle handle);

    /** 同步按请求位置的实际展示帧结束边界设置B点
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] position100ns 媒体位置，单位100纳秒，将裁剪到有效时长内
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreSetLoopBAtPosition(LumaPlayerCoreHandle handle, int64_t position100ns);

    /** 提交按请求位置设置B点的命令
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] position100ns 媒体位置，单位100纳秒，将裁剪到有效时长内
    @return 返回命令提交结果，0表示已接收，最终结果读取快照m_result
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreSetLoopBAtPositionAsync(LumaPlayerCoreHandle handle, int64_t position100ns);

    /** 同步清除A点和B点
    @param [in] handle 库创建的Core句柄，必须保持有效
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreClearLoop(LumaPlayerCoreHandle handle);

    /** 提交清除AB循环的命令
    @param [in] handle 库创建的Core句柄，必须保持有效
    @return 返回命令提交结果，0表示已接收，最终结果读取快照m_result
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreClearLoopAsync(LumaPlayerCoreHandle handle);

    /** 同步逐帧移动指定端点，先暂停并定位到该帧，越界保留原端点
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] pointType 循环点枚举，只允许A或B
    @param [in] frameOffset 要移动的展示帧数，负数向前、正数向后
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreMoveLoopPoint(LumaPlayerCoreHandle handle, int32_t pointType, int32_t frameOffset);

    /** 提交逐帧移动端点命令，执行时先暂停并同步画面和播放位置
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] pointType 循环点枚举，只允许A或B
    @param [in] frameOffset 要移动的展示帧数，负数向前、正数向后
    @return 返回命令提交结果，0表示已接收，最终结果读取快照m_result
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreMoveLoopPointAsync(LumaPlayerCoreHandle handle, int32_t pointType, int32_t frameOffset);

    /** 取消未完成的AB端点移动
    @param [in] handle 已初始化的Core句柄
    @return 操作结果
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreCancelLoopPointMove(LumaPlayerCoreHandle handle);

    /** 同步设置千分播放倍率
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] ratePermille 千分播放倍率，1000表示正常速度，按播放器范围裁剪
    @return 返回0表示成功，其他值为具体的C结果枚举
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreSetPlaybackRatePermille(LumaPlayerCoreHandle handle, int32_t ratePermille);

    /** 提交设置千分播放倍率的命令
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [in] ratePermille 千分播放倍率，1000表示正常速度，按播放器范围裁剪
    @return 返回命令提交结果，0表示已接收，最终结果读取快照m_result
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreSetPlaybackRatePermilleAsync(LumaPlayerCoreHandle handle, int32_t ratePermille);

    /** 复制当前播放快照及最近操作结果
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [out] snapshot 接收播放快照的对象，不能为空
    @return 成功返回1，句柄或输出指针无效、异常时返回0
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreGetSnapshot(LumaPlayerCoreHandle handle, LumaPlayerCoreCSnapshot* snapshot);

    /** 复制最近一次错误的UTF-8中文描述
    @param [in] handle 库创建的Core句柄，必须保持有效
    @param [out] buffer 调用方输出缓冲区，允许为空以查询所需大小
    @param [in] bufferSize 输出缓冲区字节数，包含字符串结束符空间
    @return 所需字节数，包含结束符；句柄无效或异常返回0，缓冲不足时截断并补0
    */
    LumaPlayerCoreAPI size_t lumaPlayerCoreGetLastError(LumaPlayerCoreHandle handle, char* buffer, size_t bufferSize);

    /** 获取枚举返回码对应的统一中文描述
    @param [in] result C接口返回码
    @return 库持有的UTF-8只读中文文本，调用方不得修改或释放
    */
    LumaPlayerCoreAPI const char* lumaPlayerCoreResultDescription(int32_t result);

    /** 可选异步操作，与C++操作值一致
    */
typedef enum LumaPlayerCoreCOperation
    {
        // 打开请求路径指定的媒体
        LumaPlayerCoreCOperationOpen = 1,
        // 关闭当前媒体并释放播放资源
        LumaPlayerCoreCOperationClose = 2,
        // 开始或继续播放当前媒体
        LumaPlayerCoreCOperationPlay = 3,
        // 暂停当前媒体，保留播放位置
        LumaPlayerCoreCOperationPause = 4,
        // 停止播放并回到媒体起点
        LumaPlayerCoreCOperationStop = 5,
        // 定位到m_position100ns，完成后按m_playAfterSeek决定是否播放
        LumaPlayerCoreCOperationSeek = 6,
        // 预览m_position100ns对应的视频帧，不恢复播放
        LumaPlayerCoreCOperationPreview = 7,
        // 以指定位置的真实帧起始时间设置A点
        LumaPlayerCoreCOperationSetA = 8,
        // 以指定位置的真实帧排他结束边界设置B点，必要时自动建立A点
        LumaPlayerCoreCOperationSetB = 9,
        // 清除AB端点及循环范围
        LumaPlayerCoreCOperationClearLoop = 10,
        // 按m_frameOffset逐帧移动m_point指定的端点
        LumaPlayerCoreCOperationMoveLoop = 11,
        // 设置播放倍速，m_ratePermille为千分倍率，1000表示1倍
        LumaPlayerCoreCOperationRate = 12,
        // 只预解析位置的真实帧边界，不呈现画面或改变循环、播放状态
        LumaPlayerCoreCOperationPrepareLoopPoint = 13,
        // 设置伴奏半音偏移，m_ratePermille传-7至+7，不改变播放倍速
        LumaPlayerCoreCOperationKaraokePitch = 14,
        // 切换录制模式，m_ratePermille为0或1；录制时禁用循环但保留AB标记
        LumaPlayerCoreCOperationKaraokeRecording = 15
    } LumaPlayerCoreCOperation;

    /** 请求数据，提交时复制；不改变任何旧结构布局
    */
    typedef struct LumaPlayerCoreCRequest
    {
        // 非零宿主请求ID
        uint64_t m_requestId;
        // 宿主管理的媒体代次，原样回报
        uint64_t m_mediaGeneration;
        // LumaPlayerCoreCOperation值
        int32_t m_operation;
        // UTF-8打开路径，仅提交期间借用，非打开操作可空
        const char* m_filePath;
        // 时间，100纳秒
        int64_t m_position100ns;
        // 定位完成后是否播放
        int32_t m_playAfterSeek;
        // 倍率，1000表示1倍；KaraokePitch操作时传半音数，范围-7至+7
        int32_t m_ratePermille;
        // A/B点枚举值
        int32_t m_point;
        // 帧偏移
        int32_t m_frameOffset;
    } LumaPlayerCoreCRequest;

    /** 完成数据仅在回调期间借用，宿主需复制保存
    */
    typedef struct LumaPlayerCoreCCompletion
    {
        // 原始请求ID
        uint64_t m_requestId;
        // 宿主媒体代次
        uint64_t m_mediaGeneration;
        // 操作枚举
        int32_t m_operation;
        // 真实完成结果，取消也必须回报
        int32_t m_result;
        // 完成时状态，尚未执行而取消时为默认值
        LumaPlayerCoreCSnapshot m_snapshot;
    } LumaPlayerCoreCCompletion;

    /** 独立结果线程通知，只复制并投递，不阻塞、不在回调内销毁Core
    @param [in] completion 借用的本次结果，回调返回后失效
    @param [in] userData 宿主上下文，必须活到uninit返回
    */
    typedef void (*LumaPlayerCoreCCompletionCallback)(const LumaPlayerCoreCCompletion* completion, void* userData);

    /** 提交带完成通知的可选异步请求
    @param [in] handle 有效Core句柄
    @param [in] request 不能为空，参数提交时复制
    @param [in] callback 不能为空，接受后恰有一次终态回报
    @param [in] userData 回调上下文，可空，借用到uninit结束
    @return 0表示已接受；其他值表示拒绝且不会回调
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreSubmitAsyncEx(LumaPlayerCoreHandle handle,
        const LumaPlayerCoreCRequest* request, LumaPlayerCoreCCompletionCallback callback, void* userData);

    /** Native microphone owned by Core, independent of a video handle
    */
    typedef void* LumaPlayerCoreCMicrophone;

    /** UTF-8 name is borrowed only during enumeration callback; do not throw
    @param [in] index 当前设备枚举下标，从零开始
    @param [in] name 借用的UTF-8设备名称，回调返回后不可保留指针
    @param [in] userData 本次同步枚举的宿主上下文
    */
    typedef void (*LumaPlayerCoreCMicrophoneNameCallback)(int32_t index, const char* name, void* userData);

    /** Atomic native device observations; counters are mono 48kHz frames, not audible latency
    */
    typedef struct LumaPlayerCoreCMicrophoneStats
    {
        // 设备回调累计收到的PCM帧数
        uint64_t m_receivedFrames;
        // 直接送到监听输出的累计PCM帧数
        uint64_t m_monitoredFrames;
        // 原生设备运行标记
        int32_t m_running;
        // 采集队列是否丢弃过输入帧
        int32_t m_overflow;
        // 最近S16输入峰值，最大32767
        int32_t m_peak;
        // 最近原生采集C错误码，零表示无错误
        int32_t m_error;
        // 最近设备回调的PCM帧数
        int32_t m_callbackFrames;
    } LumaPlayerCoreCMicrophoneStats;

    /** 分离结果的不透明拥有句柄，内容不可变，读取或安装调用结束后由BackingDestroy释放
    */
    typedef void* LumaPlayerCoreCBacking;

    /** 同步分离或导出进度回调，不得阻塞或抛异常
    @param [in] percent 实际已完成工作的百分比，0至100
    @param [in] userData 本次同步操作的宿主上下文
    @return 非零继续处理，零请求取消，不会交付半成品结果
    */
    typedef int (*LumaPlayerCoreCSeparationProgress)(int percent, void* userData);

    /** maxDecodedBytes limits the decoded PCM working block (at least 261120*8 bytes)
    Full-length PCM is temporary disk storage; ORT/model memory is separate.
    @param [in] sourceUtf8 原视频UTF-8路径，借用至返回
    @param [in] runtimeUtf8 运行时UTF-8路径，借用至返回
    @param [in] modelUtf8 模型UTF-8路径，借用至返回
    @param [in] maxDecodedBytes 工作内存预算，字节，按分离入口校验
    @param [in] callback 同步回调，可为空，进度回调返回零表示取消
    @param [in] userData 借用的回调上下文，在回调可发生期间保持有效
    @param [out] error 可为空的诊断输出缓冲区或对象
    @param [in] errorCapacity 错误缓冲区字节数，包含终止符
    @return 伴奏句柄，失败或取消为空，成功由调用方销毁
    */
    LumaPlayerCoreAPI LumaPlayerCoreCBacking lumaPlayerCoreSeparateVocals(const char* sourceUtf8,
        const char* runtimeUtf8, const char* modelUtf8, size_t maxDecodedBytes,
        LumaPlayerCoreCSeparationProgress callback, void* userData, char* error, size_t errorCapacity);

    /** 释放伴奏句柄，空句柄可安全忽略
    @param [in] backing 要销毁的拥有句柄，空值忽略，不得并发读取或重复销毁
    */
    LumaPlayerCoreAPI void lumaPlayerCoreBackingDestroy(LumaPlayerCoreCBacking backing);

    /** MDX-HQ5 settings, stereo 44100 Hz. One frame contains one sample per channel
    */
    typedef struct LumaPlayerCoreCSeparationOptions
    {
        // 结构字节数，调用方填写sizeof当前结构用于兼容检查
        int32_t m_size;
        // 模型单次推理窗口PCM帧数
        int32_t m_windowFrames;
        // 相邻推理窗口重叠PCM帧数
        int32_t m_overlapFrames;
    } LumaPlayerCoreCSeparationOptions;

    /** Null options uses 261120-frame windows and 130560-frame overlap (50%)
    @param [in] sourceUtf8 原视频UTF-8路径，借用至返回
    @param [in] runtimeUtf8 运行时UTF-8路径，借用至返回
    @param [in] modelUtf8 模型UTF-8路径，借用至返回
    @param [in] maxDecodedBytes 工作内存预算，字节，按分离入口校验
    @param [in] options 可为空，空时使用默认分离窗口，非空时m_size须匹配结构大小
    @param [in] callback 同步回调，可为空，进度回调返回零表示取消
    @param [in] userData 借用的回调上下文，在回调可发生期间保持有效
    @param [out] error 可为空的诊断输出缓冲区或对象
    @param [in] errorCapacity 错误缓冲区字节数，包含终止符
    @return 伴奏句柄，失败或取消为空，成功由调用方销毁
    */
    LumaPlayerCoreAPI LumaPlayerCoreCBacking lumaPlayerCoreSeparateVocalsEx(const char* sourceUtf8,
        const char* runtimeUtf8, const char* modelUtf8, size_t maxDecodedBytes,
        const LumaPlayerCoreCSeparationOptions* options, LumaPlayerCoreCSeparationProgress callback,
        void* userData, char* error, size_t errorCapacity);

    /** Null backing restores original audio; source must match current media, not a stale job
    @param [in] handle 有效Core会话句柄，调用期间不得销毁
    @param [in] sourceUtf8 原视频UTF-8路径，借用至返回
    @param [in] backing 伴奏句柄或共享数据，空值选择原唱，非空时借用至调用返回
    @return 零表示同步操作成功，非零为具体C错误码
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreBackingApply(LumaPlayerCoreHandle handle,
        const char* sourceUtf8, LumaPlayerCoreCBacking backing);

    /** Read interleaved 44.1kHz stereo float samples without changing playback
    @param [in] backing 借用的伴奏句柄，读取期间不得销毁，空值返回零帧
    @param [in] offsetFrames 伴奏PCM起始帧下标
    @param [out] output 输出缓冲区或结果对象，非空时容量须满足声明，借用至返回
    @param [in] frameCapacity 输出可容纳的双声道帧数
    @return 实际读取帧数，EOF或无效参数返回零
    */
    LumaPlayerCoreAPI size_t lumaPlayerCoreBackingRead(LumaPlayerCoreCBacking backing,
        uint64_t offsetFrames, float* output, size_t frameCapacity);

    /** Worker-only export to a NEW temporary path. Caller owns final atomic rename and cleanup
    Captures a recording snapshot; source/backing and controls must remain alive for this call
    Output is 48k stereo MP3 or H.264/AAC MP4 through FFmpegCpp; target codecs must be available
    @param [in] handle 有效Core会话句柄，调用期间不得销毁
    @param [in] sourceUtf8 原视频UTF-8路径，借用至返回
    @param [in] temporaryPathUtf8 输出临时文件UTF-8路径，借用至返回
    @param [in] includeBacking 非零时混入背景音
    @param [in] includeVideo 非零时输出视频画面
    @param [in] pitchSemitones 背景音升降调，-7至7半音
    @param [in] backingGainPercent 背景音增益百分比，0至500
    @param [in] backing 伴奏句柄或共享数据，空值选择原唱，非空时借用至调用返回
    @param [in] callback 同步回调，可为空，进度回调返回零表示取消
    @param [in] userData 借用的回调上下文，在回调可发生期间保持有效
    @param [out] error 可为空的诊断输出缓冲区或对象
    @param [in] errorCapacity 错误缓冲区字节数，包含终止符
    @return 零表示同步操作成功，非零为具体C错误码
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreKaraokeExport(LumaPlayerCoreHandle handle,
        const char* sourceUtf8, const char* temporaryPathUtf8, int32_t includeBacking,
        int32_t includeVideo, int32_t pitchSemitones, int32_t backingGainPercent,
        LumaPlayerCoreCBacking backing, LumaPlayerCoreCSeparationProgress callback, void* userData,
        char* error, size_t errorCapacity);

    /** Allocate an initially stopped native device; destroy on the owning lifecycle worker
    @return 停止状态句柄，失败为空，由Destroy释放
    */
    LumaPlayerCoreAPI LumaPlayerCoreCMicrophone lumaPlayerCoreMicrophoneCreate(void);

    /** Stop/join native callbacks and release; null is allowed, never call inside callback
    @param [in] microphone 原生设备句柄，调用期间保持存活，空值按入口返回或忽略
    */
    LumaPlayerCoreAPI void lumaPlayerCoreMicrophoneDestroy(LumaPlayerCoreCMicrophone microphone);

    /** Enumerate current UTF-8 device names; returns 0 on success
    @param [in] callback 不可为空，设备名仅在同步枚举回调期间有效
    @param [in] userData 借用的回调上下文，在回调可发生期间保持有效
    @return 零表示同步操作成功，非零为具体C错误码
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreMicrophoneEnumerate(LumaPlayerCoreCMicrophoneNameCallback callback, void* userData);

    /** Prepare while stopped; queueMs 200..10000, native period hint 5..200ms; mono S16/48kHz
    @param [in] microphone 原生设备句柄，调用期间保持存活，空值按入口返回或忽略
    @param [in] index 从零起的设备或槽位索引
    @param [in] queueMs 采集队列容量时长，毫秒
    @param [in] periodMs 请求的设备周期，毫秒，不代表端到端延迟
    @return 零表示同步操作成功，非零为具体C错误码
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreMicrophonePrepare(LumaPlayerCoreCMicrophone microphone,
        int32_t index, int32_t queueMs, int32_t periodMs);

    /** Open/start prepared device on lifecycle worker; 0 means actual start succeeded
    @param [in] microphone 原生设备句柄，调用期间保持存活，空值按入口返回或忽略
    @return 零表示同步操作成功，非零为具体C错误码
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreMicrophoneStart(LumaPlayerCoreCMicrophone microphone);

    /** Stop/join callbacks on lifecycle worker; repeated calls are safe
    @param [in] microphone 原生设备句柄，调用期间保持存活，空值按入口返回或忽略
    */
    LumaPlayerCoreAPI void lumaPlayerCoreMicrophoneStop(LumaPlayerCoreCMicrophone microphone);

    /** Atomically set actual PCM gain 0..5000 and independent mute/monitor switches
    @param [in] microphone 原生设备句柄，调用期间保持存活，空值按入口返回或忽略
    @param [in] gain 实际麦克风PCM增益百分比，0至5000
    @param [in] muted 麦克风静音标记，非零表示静音
    @param [in] monitoring 是否把麦克风PCM直接送入输出设备
    @return 零表示同步操作成功，非零为具体C错误码
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreMicrophoneControls(LumaPlayerCoreCMicrophone microphone,
        int32_t gain, int32_t muted, int32_t monitoring);

    /** Single consumer only; copies up to capacity mono frames, remainingMs optional
    @param [in] microphone 原生设备句柄，调用期间保持存活，空值按入口返回或忽略
    @param [out] output 输出缓冲区或结果对象，非空时容量须满足声明，借用至返回
    @param [in] capacity output可容纳的单声道S16采样帧数
    @param [out] remainingMs 可为空，接收剩余采集队列时长，毫秒
    @return 实际读取帧数，未启动或无可读帧返回零
    */
    LumaPlayerCoreAPI size_t lumaPlayerCoreMicrophoneRead(LumaPlayerCoreCMicrophone microphone,
        int16_t* output, size_t capacity, int32_t* remainingMs);

    /** Query stats concurrently while handle remains alive; value must not be null
    @param [in] microphone 原生设备句柄，调用期间保持存活，空值按入口返回或忽略
    @param [out] value 不可为空，接收设备状态和音频计数的值副本
    @return 零表示同步操作成功，非零为具体C错误码
    */
    LumaPlayerCoreAPI int32_t lumaPlayerCoreMicrophoneStats(LumaPlayerCoreCMicrophone microphone,
        LumaPlayerCoreCMicrophoneStats* value);

#ifdef __cplusplus
}
#endif