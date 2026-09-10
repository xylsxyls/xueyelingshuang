#pragma once
#include "LumaPlayerCoreTypes.h"
#include "FFmpegCpp/FFmpegCppAPI.h"
#include <chrono>

/** 播放引擎复用的数值裁剪、解码帧转换与读取器释放工具
	*/
class PlayerEngineHelper
{
public:
	/** 将整数限制在闭区间内
	@param [in] value 待裁剪的值
	@param [in] minValue 允许的最小值
	@param [in] maxValue 允许的最大值
	@return 裁剪后的值
	*/
	static int64_t clampInt64(int64_t value, int64_t minValue, int64_t maxValue);

	/** 比较两个整数并返回较小值，避开平台min宏
	@param [in] left 左侧数值
	@param [in] right 右侧数值
	@return 两个值中的较小值
	*/
	static int64_t minInt64(int64_t left, int64_t right);

	/** 比较两个整数并返回较大值，避开平台max宏
	@param [in] left 左侧数值
	@param [in] right 右侧数值
	@return 两个值中的较大值
	*/
	static int64_t maxInt64(int64_t left, int64_t right);

	/** 将千分倍率限制到播放器允许范围
	@param [in] ratePermille 输入倍率，1000表示正常速度
	@param [in] minRatePermille 配置的最小倍率，必须大于0
	@param [in] maxRatePermille 配置的最大倍率，不小于最小倍率
	@return 范围内的千分倍率
	*/
	static int32_t clampRate(int32_t ratePermille, int32_t minRatePermille, int32_t maxRatePermille);

	/** 释放读取器并将调用方指针清空
	@param [in,out] reader 读取器指针地址，允许为空
	*/
	static void deleteReader(FFmpegCppPlaybackReader** reader);

	/** 将解码器视频帧转换为Core帧并转移像素缓存
	@param [in,out] source 解码器视频帧，转换后像素缓存被移出
	@param [in] frameIndex 展示帧索引，未知时为-1
	@param [out] target 接收转换结果的Core视频帧
	*/
	static void convertVideoFrame(FFmpegCppPlaybackVideoFrame* source, int64_t frameIndex, LumaPlayerVideoFrame* target);

	/** 将解码器音频帧转换为Core帧并转移PCM缓存
	@param [in,out] source 解码器音频帧，转换后PCM缓存被移出
	@param [out] target 接收转换结果的Core音频帧
	*/
	static void convertAudioFrame(FFmpegCppPlaybackAudioFrame* source, LumaPlayerAudioFrame* target);

	/** 将16位交错PCM按倍率重采样，保持媒体时间戳与AB边界不变
	@param [in] source 原始媒体PCM，不修改缓存
	@param [in] ratePermille 正整数千分倍率，1000表示1.0倍率
	@param [out] target 输出设备按原采样率播放的PCM
	@return true表示转换成功，false表示参数或格式无效
	*/
	static bool scaleAudioRate(const LumaPlayerAudioFrame& source, int32_t ratePermille, LumaPlayerAudioFrame* target);
};