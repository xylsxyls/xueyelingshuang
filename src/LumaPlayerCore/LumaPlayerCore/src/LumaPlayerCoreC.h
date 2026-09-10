#pragma once
#include <stddef.h>
#include <stdint.h>
#include "LumaPlayerCoreMacro.h"

#ifdef __cplusplus
extern "C" {
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
	// 最高播放倍率，不小于最低倍率，默认3000表示3.0倍率，1000表示1.0倍率
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
	LumaPlayerCoreCStateClosed = 0,
	LumaPlayerCoreCStateStopped = 1,
	LumaPlayerCoreCStatePlaying = 2,
	LumaPlayerCoreCStatePaused = 3,
	LumaPlayerCoreCStateSeeking = 4,
	LumaPlayerCoreCStateEnded = 5,
	LumaPlayerCoreCStateError = 6,
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

#ifdef __cplusplus
}
#endif