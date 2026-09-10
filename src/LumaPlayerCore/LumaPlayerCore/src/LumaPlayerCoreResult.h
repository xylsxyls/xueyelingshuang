#pragma once
#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include <map>
#include <string>

/** 播放器操作结果，0表示成功，其余枚举值表示具体失败或终止原因
*/
enum LumaPlayerCoreResult
{
	// 操作成功
	LumaPlayerCoreResultSuccess = 0,
	// 参数不符合接口要求，请检查空指针、文件路径和配置范围
	LumaPlayerCoreResultInvalidParam = 1,
	// 播放器尚未初始化或已经反初始化，请先成功调用init
	LumaPlayerCoreResultNotInit = 2,
	// 当前没有打开可播放的媒体文件
	LumaPlayerCoreResultNotOpen = 3,
	// 媒体打开失败，请检查文件是否存在、访问权限以及媒体格式
	LumaPlayerCoreResultOpenFailed = 4,
	// 当前播放状态或AB端点范围不允许执行该操作
	LumaPlayerCoreResultStateError = 5,
	// 请求已被更新的预览请求替代或调用方取消
	LumaPlayerCoreResultCanceled = 6,
	// 等待工作线程完成命令超时，命令可能仍在执行
	LumaPlayerCoreResultTimeout = 7,
	// 播放器内部发生异常，无法完成当前操作
	LumaPlayerCoreResultInternalError = 8,
	// 已到媒体流结尾，没有下一帧
	LumaPlayerCoreResultEnd = 9,
	// 创建播放或预览工作线程时发生异常
	LumaPlayerCoreResultThreadInitException = 10,
	// 任务管理器未能创建播放或预览工作线程
	LumaPlayerCoreResultThreadInitFailed = 11,
	// 创建播放工作任务失败，无法分配内存
	LumaPlayerCoreResultWorkerAllocationFailed = 12,
	// 为播放工作任务分配共享管理对象失败
	LumaPlayerCoreResultWorkerOwnershipFailed = 13,
	// 工作线程初始化后无法获取对应任务线程对象
	LumaPlayerCoreResultThreadUnavailable = 14,
	// 创建同步命令结果失败，无法分配内存
	LumaPlayerCoreResultResultAllocationFailed = 15,
	// 为同步命令结果分配共享管理对象失败
	LumaPlayerCoreResultResultOwnershipFailed = 16,
	// 命令队列入队时发生异常，控制请求未被接受
	LumaPlayerCoreResultCommandQueueFailed = 17,
	// 预览任务线程不存在或已经停止，无法提交预览
	LumaPlayerCoreResultPreviewThreadUnavailable = 18,
	// 创建预览任务失败，无法分配内存
	LumaPlayerCoreResultPreviewAllocationFailed = 19,
	// 为预览任务分配共享管理对象失败
	LumaPlayerCoreResultPreviewOwnershipFailed = 20,
	// 设置A点失败，无法确定目标画面的真实展示区间
	LumaPlayerCoreResultLoopAFrameUnavailable = 21,
	// A点帧起点必须早于B点帧的排他结束边界，原端点保持不变
	LumaPlayerCoreResultLoopAOutOfRange = 22,
	// 设置B点失败，无法确定目标画面的真实结束边界
	LumaPlayerCoreResultLoopBFrameUnavailable = 23,
	// 设置B点时无法定位媒体起点画面，自动建立A点失败
	LumaPlayerCoreResultLoopAutoAUnavailable = 24,
	// B点帧的排他结束边界必须晚于A点帧起点，原端点保持不变
	LumaPlayerCoreResultLoopBOutOfRange = 25,
	// 要移动的AB端点尚未设置，请先设置该端点
	LumaPlayerCoreResultLoopPointNotSet = 26,
	// 读取媒体信息失败，请检查文件路径、权限和封装格式
	LumaPlayerCoreResultMediaInfoOpenFailed = 27,
	// 创建视频或预览解码器失败，无法分配内存
	LumaPlayerCoreResultVideoAllocationFailed = 28,
	// 视频解码器打开失败，视频流可能损坏或编码不受支持
	LumaPlayerCoreResultVideoOpenFailed = 29,
	// 预览解码器打开失败，无法执行定位和逐帧预览
	LumaPlayerCoreResultPreviewOpenFailed = 30,
	// 创建音频解码器失败，无法分配内存
	LumaPlayerCoreResultAudioAllocationFailed = 31,
	// 视频定位失败，无法跳转到请求的媒体位置
	LumaPlayerCoreResultVideoSeekFailed = 32,
	// 音频定位失败，无法与目标视频位置重新同步
	LumaPlayerCoreResultAudioSeekFailed = 33,
	// 目标位置预览解码失败，未能生成有效画面
	LumaPlayerCoreResultPreviewDecodeFailed = 34,
	// 预览解码器尚未打开，无法读取相邻展示帧
	LumaPlayerCoreResultPreviewReaderUnavailable = 35,
	// 逐帧定位失败，无法回退到可解码的视频位置
	LumaPlayerCoreResultAdjacentSeekFailed = 36,
	// 下一展示帧解码失败，原AB端点保持不变
	LumaPlayerCoreResultNextFrameDecodeFailed = 37,
	// 未找到下一展示帧，可能已到媒体末帧
	LumaPlayerCoreResultNextFrameUnavailable = 38,
	// 上一展示帧解码失败，原AB端点保持不变
	LumaPlayerCoreResultPreviousFrameDecodeFailed = 39,
	// 未找到上一展示帧，可能已到媒体首帧或超出回溯范围
	LumaPlayerCoreResultPreviousFrameUnavailable = 40,
	// 播放视频读取或解码失败，媒体数据可能损坏
	LumaPlayerCoreResultVideoReadFailed = 41,
	// 播放音频读取或解码失败，媒体数据可能损坏
	LumaPlayerCoreResultAudioReadFailed = 42,
	// 音频解码器打开失败，无法播放媒体声音
	LumaPlayerCoreResultAudioOpenFailed = 43,
	// 视频输出设备打开失败，无法呈现画面
	LumaPlayerCoreResultVideoRenderOpenFailed = 44,
	// 音频输出设备打开失败，无法播放声音
	LumaPlayerCoreResultAudioRenderOpenFailed = 45,
	// 视频输出回调拒绝当前画面
	LumaPlayerCoreResultVideoRenderFailed = 46,
	// 音频输出回调拒绝当前音频片段
	LumaPlayerCoreResultAudioRenderFailed = 47
};

// 统一的中文结果描述，业务日志和错误提示使用同一份文本
const std::map<LumaPlayerCoreResult, std::string> kLumaPlayerCoreResultDescriptions =
{
	{ LumaPlayerCoreResultSuccess, "操作成功" },
	{ LumaPlayerCoreResultInvalidParam, "参数不符合接口要求，请检查空指针、文件路径和配置范围" },
	{ LumaPlayerCoreResultNotInit, "播放器尚未初始化或已经反初始化，请先成功调用init" },
	{ LumaPlayerCoreResultNotOpen, "当前没有打开可播放的媒体文件" },
	{ LumaPlayerCoreResultOpenFailed, "媒体打开失败，请检查文件是否存在、访问权限以及媒体格式" },
	{ LumaPlayerCoreResultStateError, "当前播放状态或AB端点范围不允许执行该操作" },
	{ LumaPlayerCoreResultCanceled, "请求已被更新的预览请求替代或调用方取消" },
	{ LumaPlayerCoreResultTimeout, "等待工作线程完成命令超时，命令可能仍在执行" },
	{ LumaPlayerCoreResultInternalError, "播放器内部发生异常，无法完成当前操作" },
	{ LumaPlayerCoreResultEnd, "已到媒体流结尾，没有下一帧" },
	{ LumaPlayerCoreResultThreadInitException, "创建播放或预览工作线程时发生异常" },
	{ LumaPlayerCoreResultThreadInitFailed, "任务管理器未能创建播放或预览工作线程" },
	{ LumaPlayerCoreResultWorkerAllocationFailed, "创建播放工作任务失败，无法分配内存" },
	{ LumaPlayerCoreResultWorkerOwnershipFailed, "为播放工作任务分配共享管理对象失败" },
	{ LumaPlayerCoreResultThreadUnavailable, "工作线程初始化后无法获取对应任务线程对象" },
	{ LumaPlayerCoreResultResultAllocationFailed, "创建同步命令结果失败，无法分配内存" },
	{ LumaPlayerCoreResultResultOwnershipFailed, "为同步命令结果分配共享管理对象失败" },
	{ LumaPlayerCoreResultCommandQueueFailed, "命令队列入队时发生异常，控制请求未被接受" },
	{ LumaPlayerCoreResultPreviewThreadUnavailable, "预览任务线程不存在或已经停止，无法提交预览" },
	{ LumaPlayerCoreResultPreviewAllocationFailed, "创建预览任务失败，无法分配内存" },
	{ LumaPlayerCoreResultPreviewOwnershipFailed, "为预览任务分配共享管理对象失败" },
	{ LumaPlayerCoreResultLoopAFrameUnavailable, "设置A点失败，无法确定目标画面的真实展示区间" },
	{ LumaPlayerCoreResultLoopAOutOfRange, "A点帧起点必须早于B点帧的排他结束边界，原端点保持不变" },
	{ LumaPlayerCoreResultLoopBFrameUnavailable, "设置B点失败，无法确定目标画面的真实结束边界" },
	{ LumaPlayerCoreResultLoopAutoAUnavailable, "设置B点时无法定位媒体起点画面，自动建立A点失败" },
	{ LumaPlayerCoreResultLoopBOutOfRange, "B点帧的排他结束边界必须晚于A点帧起点，原端点保持不变" },
	{ LumaPlayerCoreResultLoopPointNotSet, "要移动的AB端点尚未设置，请先设置该端点" },
	{ LumaPlayerCoreResultMediaInfoOpenFailed, "读取媒体信息失败，请检查文件路径、权限和封装格式" },
	{ LumaPlayerCoreResultVideoAllocationFailed, "创建视频或预览解码器失败，无法分配内存" },
	{ LumaPlayerCoreResultVideoOpenFailed, "视频解码器打开失败，视频流可能损坏或编码不受支持" },
	{ LumaPlayerCoreResultPreviewOpenFailed, "预览解码器打开失败，无法执行定位和逐帧预览" },
	{ LumaPlayerCoreResultAudioAllocationFailed, "创建音频解码器失败，无法分配内存" },
	{ LumaPlayerCoreResultVideoSeekFailed, "视频定位失败，无法跳转到请求的媒体位置" },
	{ LumaPlayerCoreResultAudioSeekFailed, "音频定位失败，无法与目标视频位置重新同步" },
	{ LumaPlayerCoreResultPreviewDecodeFailed, "目标位置预览解码失败，未能生成有效画面" },
	{ LumaPlayerCoreResultPreviewReaderUnavailable, "预览解码器尚未打开，无法读取相邻展示帧" },
	{ LumaPlayerCoreResultAdjacentSeekFailed, "逐帧定位失败，无法回退到可解码的视频位置" },
	{ LumaPlayerCoreResultNextFrameDecodeFailed, "下一展示帧解码失败，原AB端点保持不变" },
	{ LumaPlayerCoreResultNextFrameUnavailable, "未找到下一展示帧，可能已到媒体末帧" },
	{ LumaPlayerCoreResultPreviousFrameDecodeFailed, "上一展示帧解码失败，原AB端点保持不变" },
	{ LumaPlayerCoreResultPreviousFrameUnavailable, "未找到上一展示帧，可能已到媒体首帧或超出回溯范围" },
	{ LumaPlayerCoreResultVideoReadFailed, "播放视频读取或解码失败，媒体数据可能损坏" },
	{ LumaPlayerCoreResultAudioReadFailed, "播放音频读取或解码失败，媒体数据可能损坏" },
	{ LumaPlayerCoreResultAudioOpenFailed, "音频解码器打开失败，无法播放媒体声音" },
	{ LumaPlayerCoreResultVideoRenderOpenFailed, "视频输出设备打开失败，无法呈现画面" },
	{ LumaPlayerCoreResultAudioRenderOpenFailed, "音频输出设备打开失败，无法播放声音" },
	{ LumaPlayerCoreResultVideoRenderFailed, "视频输出回调拒绝当前画面" },
	{ LumaPlayerCoreResultAudioRenderFailed, "音频输出回调拒绝当前音频片段" }
};