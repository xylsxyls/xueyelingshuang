#pragma once
#ifndef LUMAPLAYERCORE_USE_C_API
#define LUMAPLAYERCORE_USE_C_API
#endif
#include "LumaPlayerCore/LumaPlayerCoreAPI.h"

#include <stdint.h>
#include <string>

class LumaPlayerAudioRender;
class LumaPlayerVideoRender;

/** Qt播放器到Core C接口的桥接层，隔离不同运行库之间的C++ ABI
*/
class LumaPlayerCoreBridge
{
public:
    /** 同步给当前媒体安装匹配来源的伴奏
    @param [in] source 原媒体来源或PCM存储，借用期覆盖整个调用
    @param [in] backing 伴奏句柄或共享数据，空值选择原唱，非空时借用至调用返回
    @return 零表示同步操作成功，非零为具体C错误码
    */
    int32_t applyKaraokeBacking(const std::string& source, LumaPlayerCoreCBacking backing);

    /** 同步导出当前录制的完整歌曲到临时文件
    @param [in] source 原媒体来源或PCM存储，借用期覆盖整个调用
    @param [in] temporaryPath 临时输出路径，调用期间保持有效
    @param [in] mix 是否混合背景音
    @param [in] video 是否包含原视频画面
    @param [in] pitch 背景音升降调，-7至7半音
    @param [in] gain 背景音增益百分比，0至500
    @param [in] backing 伴奏句柄或共享数据，空值选择原唱，非空时借用至调用返回
    @param [in] callback 同步回调，可为空，进度回调返回零表示取消
    @param [in] user 借用的回调上下文，同步调用返回或异步完成前保持有效
    @param [out] error 可为空的诊断输出缓冲区或对象
    @param [in] capacity error缓冲区字节数，包含终止符
    @return 零表示同步操作成功，非零为具体C错误码
    */
    int32_t exportKaraoke(const char* source, const char* temporaryPath, bool mix, bool video,
        int pitch, int gain, LumaPlayerCoreCBacking backing, LumaPlayerCoreCSeparationProgress callback,
        void* user, char* error, size_t capacity);

    /** 提交携带身份和真实完成回报的C请求
    @param [in] request 参数副本，字符串仅提交期间借用
    @param [in] callback 短小结果回调
    @param [in] userData 借用上下文，存活到uninit结束
    @return 提交结果，非零时不回调
    */
    int32_t submitAsyncEx(const LumaPlayerCoreCRequest& request,
        LumaPlayerCoreCCompletionCallback callback, void* userData);

    /** 创建Core句柄并记录外部渲染器
    @param [in] audioRender Qt音频渲染器，由窗口管理生命周期
    @param [in] videoRender Qt视频渲染器，由窗口管理生命周期
    */
    LumaPlayerCoreBridge(LumaPlayerAudioRender* audioRender, LumaPlayerVideoRender* videoRender);

    /** 停止Core线程并销毁由Core DLL创建的句柄
    */
    ~LumaPlayerCoreBridge();

    /** 注册渲染回调、开启业务日志并注入配置初始化Core
    @param [in] debugEnabled 保留的debug诊断标记，不控制基础日志开关
    @return 返回LumaPlayerCoreCResult数值
    */
    int32_t init(bool debugEnabled);

    /** 停止Core线程和媒体处理，重复调用安全
    */
    void uninit();

    /** 获取供界面读取的播放快照
    @return 返回当前快照；句柄不可用时返回全零快照
    */
    LumaPlayerCoreCSnapshot snapshot() const;

    /** 获取Core最后一次错误文本
    @return 返回UTF-8文本，过长内容会被截断到本地诊断缓冲区
    */
    std::string lastError() const;

    /** 异步打开UTF-8媒体路径，返回命令投递结果
    @param [in] filePath 媒体UTF-8路径，请求提交期间有效
    @return 零表示请求已接受，不代表执行完成；非零为提交错误
    */
    int32_t openMediaAsync(const std::string& filePath);

    /** 异步开始或恢复播放，返回命令投递结果
    @return 零表示请求已接受，不代表执行完成；非零为提交错误
    */
    int32_t playAsync();

    /** 异步暂停播放，返回命令投递结果
    @return 零表示请求已接受，不代表执行完成；非零为提交错误
    */
    int32_t pauseAsync();

    /** 异步跳转到目标时间，keepPlayState决定完成后是否播放
    @param [in] position100ns 媒体位置，100纳秒单位
    @param [in] keepPlayState 是否保持原播放意图
    @return 零表示请求已接受，不代表执行完成；非零为提交错误
    */
    int32_t seekToAsync(int64_t position100ns, bool keepPlayState);

    /** 异步解码目标位置预览帧，不改变正式播放时钟
    @param [in] position100ns 媒体位置，100纳秒单位
    @return 零表示请求已接受，不代表执行完成；非零为提交错误
    */
    int32_t previewFrameAsync(int64_t position100ns);

    /** 按目标位置实际显示帧异步设置A点
    @param [in] position100ns 媒体位置，100纳秒单位
    @return 零表示请求已接受，不代表执行完成；非零为提交错误
    */
    int32_t setLoopAAtPositionAsync(int64_t position100ns);

    /** 按目标位置实际显示帧异步设置B点
    @param [in] position100ns 媒体位置，100纳秒单位
    @return 零表示请求已接受，不代表执行完成；非零为提交错误
    */
    int32_t setLoopBAtPositionAsync(int64_t position100ns);

    /** 异步取消AB循环
    @return 零表示请求已接受，不代表执行完成；非零为提交错误
    */
    int32_t clearLoopAsync();

    /** 异步按帧移动循环点；pointType为0表示A点，1表示B点
    @param [in] pointType AB点类型
    @param [in] frameOffset 帧步进偏移，可为负
    @return 零表示请求已接受，不代表执行完成；非零为提交错误
    */
    int32_t moveLoopPointAsync(int32_t pointType, int32_t frameOffset);

    /** 取消未完成的AB移动
    */
    void cancelLoopPointMove();

    /** 异步设置千分比倍速，1000表示1.0倍
    @param [in] ratePermille 播放速率千分值，1000为1倍，范围由配置校验
    @return 零表示请求已接受，不代表执行完成；非零为提交错误
    */
    int32_t setPlaybackRatePermilleAsync(int32_t ratePermille);

    /** 初始化本次K歌录制时间线
    @param [in] sampleRate 输入PCM采样率
    @param [in] channels 输入PCM声道数
    @param [in] maxPcmBytes 录音内存上限
    @return 零表示同步操作成功，非零为具体C错误码
    */
    int32_t initKaraokeRecording(int32_t sampleRate, int32_t channels,
        size_t maxPcmBytes);

    /** 把已聚合的麦克风PCM送入Core，暂停时仍可监听但不会录入
    @param [in] mediaStart100ns 播放中本块媒体起点
    @param [in] mediaEnd100ns 播放中本块媒体终点
    @param [in] ratePermille 本块播放倍率
    @param [in] inputPcm 借用的交错S16 PCM
    @param [in] frameCount 每声道帧数
    @param [in] gainPercent 实际PCM麦克风增益，范围0至5000；UI显示值在调用前换算
    @param [in] microphoneMuted 是否静音麦克风
    @param [in] monitoringEnabled 是否把人声送往监听输出
    @param [in] recordingEnabled 是否录制
    @param [in] playbackRunning 当前是否正在播放
    @param [out] monitorPcm 接收监听PCM
    @param [in] microphone 麦克风槽位索引，0至2
    @return 零表示同步操作成功，非零为具体C错误码
    */
    int32_t processKaraokeMicrophone(int64_t mediaStart100ns,
        int64_t mediaEnd100ns, int32_t ratePermille, const int16_t* inputPcm,
        size_t frameCount, int32_t gainPercent, bool microphoneMuted,
        bool monitoringEnabled, bool recordingEnabled, bool playbackRunning,
        int16_t* monitorPcm, int32_t microphone = 0);

    /** 把本次播放位置推进到录音时间线；应在工作线程周期调用以覆盖EOF边界
    @param [in] mediaPosition100ns 当前媒体位置
    @return 零表示同步操作成功，非零为具体C错误码
    */
    int32_t advanceKaraokeRecording(int64_t mediaPosition100ns);

    /** 查询录音成品帧数
    @param [out] frameCount 接收帧数
    @return 零表示同步操作成功，非零为具体C错误码
    */
    int32_t karaokeOutputFrameCount(uint64_t* frameCount) const;

    /** Query retained capture frames without counting generated silence
    @param [out] frameCount 不可为空，接收PCM帧数
    @return 零表示同步操作成功，非零为具体C错误码
    */
    int32_t karaokeCapturedFrameCount(uint64_t* frameCount) const;

    /** 按帧分块读取录音成品PCM
    @param [in] startFrame 起始帧
    @param [in] frameCount 帧数
    @param [out] outputPcm 接收PCM
    @return 零表示同步操作成功，非零为具体C错误码
    */
    int32_t readKaraokePcm(uint64_t startFrame, size_t frameCount,
        int16_t* outputPcm) const;

    /** 清空当前录音数据并保留其PCM格式
    @return 零表示同步操作成功，非零为具体C错误码
    */
    int32_t clearKaraokeRecording();

private:
    /** 禁止复制拥有Core句柄的桥接器
    @param [in] other 另一桥接器
    */
    LumaPlayerCoreBridge(const LumaPlayerCoreBridge& other) = delete;

    /** 禁止复制Core句柄所有权
    @param [in] other 另一桥接器
    @return 当前对象
    */
    LumaPlayerCoreBridge& operator=(const LumaPlayerCoreBridge& other) = delete;

    /** 把Core的音频打开请求转发到Qt音频渲染器
    @param [in] format 外部输出设备需要的格式
    @param [in] userData 借用的渲染器上下文，Core回调全部退出前保持有效
    @return 零表示同步操作成功，非零为具体C错误码
    */
    static int32_t openAudioCallback(const LumaPlayerCoreCAudioFormat* format, void* userData);

    /** 把Core的音频关闭请求转发到Qt音频渲染器
    @param [in] userData 借用的渲染器上下文，Core回调全部退出前保持有效
    */
    static void closeAudioCallback(void* userData);

    /** 把Core的音频清空请求转发到Qt音频渲染器
    @param [in] userData 借用的渲染器上下文，Core回调全部退出前保持有效
    */
    static void flushAudioCallback(void* userData);

    /** 把Core的音频暂停请求转发到Qt音频渲染器
    @param [in] userData 借用的渲染器上下文，Core回调全部退出前保持有效
    */
    static void pauseAudioCallback(void* userData);

    /** 把Core的音频恢复请求转发到Qt音频渲染器
    @param [in] userData 借用的渲染器上下文，Core回调全部退出前保持有效
    */
    static void resumeAudioCallback(void* userData);

    /** 在回调有效期内把PCM复制到Qt音频渲染器
    @param [in] frame 回调期间借用的媒体帧
    @param [in] userData 借用的渲染器上下文，Core回调全部退出前保持有效
    @return 零表示同步操作成功，非零为具体C错误码
    */
    static int32_t renderAudioCallback(const LumaPlayerCoreCAudioFrame* frame, void* userData);

    /** 查询Qt侧尚未消费的PCM时长
    @param [in] userData 借用的渲染器上下文，Core回调全部退出前保持有效
    @return 尚未消费的PCM时长，100纳秒单位
    */
    static int64_t bufferedAudioCallback(void* userData);

    /** 把Core的视频打开请求转发到Qt视频渲染器
    @param [in] format 外部输出设备需要的格式
    @param [in] userData 借用的渲染器上下文，Core回调全部退出前保持有效
    @return 零表示同步操作成功，非零为具体C错误码
    */
    static int32_t openVideoCallback(const LumaPlayerCoreCVideoFormat* format, void* userData);

    /** 把Core的视频关闭请求转发到Qt视频渲染器
    @param [in] userData 借用的渲染器上下文，Core回调全部退出前保持有效
    */
    static void closeVideoCallback(void* userData);

    /** 在回调有效期内把BGRA帧复制到Qt视频渲染器
    @param [in] frame 回调期间借用的媒体帧
    @param [in] userData 借用的渲染器上下文，Core回调全部退出前保持有效
    @return 零表示同步操作成功，非零为具体C错误码
    */
    static int32_t renderVideoCallback(const LumaPlayerCoreCVideoFrame* frame, void* userData);

private:
    // Core DLL创建并销毁的不透明句柄
    LumaPlayerCoreHandle m_handle;
    // 外部拥有的Qt音频渲染器，Core停止前必须保持有效
    LumaPlayerAudioRender* m_audioRender;
    // 外部拥有的Qt视频渲染器，Core停止前必须保持有效
    LumaPlayerVideoRender* m_videoRender;
    // 防止重复执行Core释放流程
    bool m_initialized;
};