#pragma once
#include "LumaPlayerCoreResult.h"
#include "LumaPlayerCoreMacro.h"

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

/** 播放核心当前状态
*/
enum LumaPlayerCoreState
{
	// 没有打开媒体
	LumaPlayerCoreStateClosed = 0,
	// 媒体已打开，但播放位置在开始或被停止
	LumaPlayerCoreStateStopped = 1,
	// 正在播放
	LumaPlayerCoreStatePlaying = 2,
	// 已暂停，播放位置保持在当前帧
	LumaPlayerCoreStatePaused = 3,
	// 正在执行seek或预览
	LumaPlayerCoreStateSeeking = 4,
	// 播放到文件末尾，后续会根据循环设置决定是否重播
	LumaPlayerCoreStateEnded = 5,
	// 播放核心发生错误
	LumaPlayerCoreStateError = 6,
	// 正在工作线程打开媒体，调用线程可以继续处理界面事件
	LumaPlayerCoreStateOpening = 7
};

/** AB循环点类型
*/
enum LumaPlayerCoreLoopPointType
{
	// A点表示循环左边界，语义为当前显示帧起始时间
	LumaPlayerCoreLoopPointA = 0,
	// B点表示循环右边界，语义为当前显示帧结束时间且不播放右边界帧
	LumaPlayerCoreLoopPointB = 1
};

/** 音频PCM格式
*/
struct LumaPlayerCoreAPI LumaPlayerAudioFormat
{
public:
	// PCM采样率
	int32_t m_sampleRate;
	// PCM声道数
	int32_t m_channels;
	// 单个采样点位深，目前主要使用16位
	int32_t m_bitsPerSample;

public:
	/** 构造一个空音频格式
	*/
	LumaPlayerAudioFormat();

	/** 判断格式是否能用于播放
	@return true表示采样率、声道数和位深都有效
	*/
	bool isValid() const;

	/** 获取单声道单采样占用字节数
	@return 返回采样字节数，无效格式返回0
	*/
	int32_t bytesPerSample() const;

	/** 获取一帧多声道PCM占用字节数
	@return 返回一组多声道采样的字节数，无效格式返回0
	*/
	int32_t bytesPerFrame() const;
};

/** 视频图像格式
*/
struct LumaPlayerCoreAPI LumaPlayerVideoFormat
{
public:
	// 视频原始宽度
	int32_t m_width;
	// 视频原始高度
	int32_t m_height;

public:
	/** 构造一个空视频格式
	*/
	LumaPlayerVideoFormat();

	/** 判断格式是否能用于显示
	@return true表示宽高都大于0
	*/
	bool isValid() const;
};

/** Core向外部视频渲染器送出的BGRA视频帧
*/
struct LumaPlayerCoreAPI LumaPlayerVideoFrame
{
public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	// BGRA像素数据，按m_stride逐行排列
	std::vector<unsigned char> m_bgraPixels;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	// 帧宽度
	int32_t m_width;
	// 帧高度
	int32_t m_height;
	// 每行字节数
	int32_t m_stride;
	// 当前帧起始时间，单位100纳秒
	int64_t m_timestamp100ns;
	// 当前帧显示时长，单位100纳秒
	int64_t m_duration100ns;
	// 当前帧索引，未知时为-1
	int64_t m_frameIndex;
	// 是否关键帧
	bool m_keyFrame;

public:
	/** 构造空视频帧
	*/
	LumaPlayerVideoFrame();

	/** 清空像素数据并恢复默认时间信息
	*/
	void reset();

	/** 判断当前帧是否没有可显示内容
	@return true表示没有像素或宽高无效
	*/
	bool empty() const;

	/** 获取当前帧结束边界
	@return 返回timestamp + duration，duration无效时使用timestamp
	*/
	int64_t endTime100ns() const;
};

/** Core向外部音频渲染器送出的交错PCM音频帧
*/
struct LumaPlayerCoreAPI LumaPlayerAudioFrame
{
public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	// PCM数据，格式由m_format描述
	std::vector<unsigned char> m_pcmData;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	// PCM格式
	LumaPlayerAudioFormat m_format;
	// 音频片段起始时间，单位100纳秒
	int64_t m_timestamp100ns;
	// 音频片段时长，单位100纳秒
	int64_t m_duration100ns;

public:
	/** 构造空音频帧
	*/
	LumaPlayerAudioFrame();

	/** 清空PCM数据并恢复默认时间信息
	*/
	void reset();

	/** 判断当前音频帧是否没有可播放数据
	@return true表示PCM为空或格式无效
	*/
	bool empty() const;

	/** 获取当前音频片段结束边界
	@return 返回timestamp + duration，duration无效时使用timestamp
	*/
	int64_t endTime100ns() const;
};

/** 媒体文件基础信息
*/
struct LumaPlayerCoreAPI LumaPlayerMediaInfo
{
public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	// 当前打开的UTF-8文件路径
	std::string m_filePath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	// 总时长，单位100纳秒
	int64_t m_duration100ns;
	// 默认视频帧时长，单位100纳秒
	int64_t m_defaultVideoFrameDuration100ns;
	// 视频格式
	LumaPlayerVideoFormat m_videoFormat;
	// 音频格式
	LumaPlayerAudioFormat m_audioFormat;
	// 是否存在视频流
	bool m_hasVideo;
	// 是否存在音频流
	bool m_hasAudio;

public:
	/** 构造空媒体信息
	*/
	LumaPlayerMediaInfo();

	/** 清空媒体信息
	*/
	void reset();
};

/** 一个AB点的内部边界信息
*/
struct LumaPlayerCoreAPI LumaPlayerLoopPointInfo
{
public:
	// 该循环点是否已经设置
	bool m_isSet;
	// 当前点所在帧的起始时间，单位100纳秒
	int64_t m_frameStart100ns;
	// 当前点所在帧的结束边界，单位100纳秒
	int64_t m_frameEnd100ns;
	// 当前点所在帧索引，未知时为-1
	int64_t m_frameIndex;

public:
	/** 构造一个未设置的循环点
	*/
	LumaPlayerLoopPointInfo();

	/** 清空循环点
	*/
	void reset();
};

/** AB循环范围，播放语义为[A.frameStart, B.frameEnd)
*/
struct LumaPlayerCoreAPI LumaPlayerLoopRange
{
public:
	// A点信息
	LumaPlayerLoopPointInfo m_aPoint;
	// B点信息
	LumaPlayerLoopPointInfo m_bPoint;

public:
	/** 构造一个空循环范围
	*/
	LumaPlayerLoopRange();

	/** 清空A点和B点
	*/
	void reset();

	/** 判断当前AB点是否形成有效左闭右开区间
	@return true表示A和B都已设置，且B结束边界大于A起始边界
	*/
	bool isValid() const;

	/** 获取实际循环起点
	@return A点帧起始时间，未设置时返回0
	*/
	int64_t start100ns() const;

	/** 获取实际循环右边界
	@return B点帧结束边界，未设置时返回0
	*/
	int64_t end100ns() const;
};

/** 播放核心快照，用于界面刷新和测试校验
*/
struct LumaPlayerCoreAPI LumaPlayerSnapshot
{
public:
	// 最近完成的操作或后台播放错误码
	LumaPlayerCoreResult m_result;
	// 当前播放状态
	LumaPlayerCoreState m_state;
	// 成功打开媒体后递增的序号，用于异步调用方识别新媒体已经就绪
	uint64_t m_mediaSerial;
	// 当前媒体信息
	LumaPlayerMediaInfo m_mediaInfo;
	// 当前播放位置，单位100纳秒
	int64_t m_position100ns;
	// 当前播放倍速，1000表示1.0倍速
	int32_t m_ratePermille;
	// 当前AB循环范围
	LumaPlayerLoopRange m_loopRange;
	// 最近一次显示的视频帧
	LumaPlayerLoopPointInfo m_currentVideoFrame;
	// 是否启用文件播放结束后自动循环
	bool m_enableAutoReplay;

public:
	/** 构造空播放快照
	*/
	LumaPlayerSnapshot();

	/** 清空快照到初始状态
	*/
	void reset();
};