#pragma once
#include "LumaPlayerCore/LumaPlayerCoreAPI.h"
#include "LumaPlayerCore/LumaPlayerCoreC.h"

#include <stdint.h>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

/** 单个测试用例的执行结果。 */
struct LumaPlayerTestCaseRecord
{
public:
	/** 用例名称，用于报告定位。 */
	std::string m_name;
	/** true表示用例全部检查通过。 */
	bool m_passed;
	/** 用例耗时，单位毫秒。 */
	int64_t m_costMs;
	/** 用例检查细节和失败原因。 */
	std::vector<std::string> m_messages;

public:
	/** 构造一个空测试结果。 */
	LumaPlayerTestCaseRecord();
};

/** 测试报告收集器，负责记录检查点并输出文本报告。 */
class LumaPlayerTestReport
{
public:
	/** 构造一个没有当前用例的报告对象。 */
	LumaPlayerTestReport();

	/**
	* 开始记录一个测试用例。
	* @param [in] caseName 测试用例名称。
	*/
	void beginCase(const std::string& caseName);

	/** 结束当前测试用例并写入结果列表。 */
	void endCase();

	/**
	* 记录一个布尔检查点。
	* @param [in] condition true表示检查通过。
	* @param [in] message 检查点说明或失败原因。
	*/
	void check(bool condition, const std::string& message);

	/**
	* 写入一条普通信息。
	* @param [in] message 信息文本。
	*/
	void info(const std::string& message);

	/**
	* 获取失败用例数量。
	* @return 返回失败用例数量。
	*/
	int32_t failedCount() const;

	/**
	* 输出报告到控制台和文件。
	* @param [in] filePath 报告文件路径。
	*/
	void writeReport(const std::string& filePath) const;

private:
	/** 保护测试用例和日志消息，Core日志可能从后台线程写入报告。 */
	mutable std::mutex m_mutex;
	/** 当前用例是否已经开始。 */
	bool m_caseStarted;
	/** 当前用例开始时间，使用毫秒时间戳保存。 */
	int64_t m_caseBeginMs;
	/** 当前正在收集的用例结果。 */
	LumaPlayerTestCaseRecord m_currentCase;
	/** 所有已完成测试用例结果。 */
	std::vector<LumaPlayerTestCaseRecord> m_records;
};

/** 测试音频渲染器的线程安全计数快照。 */
struct LumaPlayerTestAudioStats
{
	/** 音频打开次数。 */
	int32_t m_openCount;
	/** 音频关闭次数。 */
	int32_t m_closeCount;
	/** 音频清空次数。 */
	int32_t m_flushCount;
	/** 音频暂停次数。 */
	int32_t m_pauseCount;
	/** 音频恢复次数。 */
	int32_t m_resumeCount;
	/** 已收到音频帧数量。 */
	int32_t m_frameCount;
	/** 已收到PCM总字节数。 */
	size_t m_pcmBytes;
};

/** 测试用音频渲染器，只计数不播放声音，避免测试依赖设备。 */
class LumaPlayerTestAudioRender : public IAudioRender
{
public:
	/** 构造计数清零的音频渲染桩。 */
	LumaPlayerTestAudioRender();

	/** @see IAudioRender::openAudio */
	virtual bool openAudio(const LumaPlayerAudioFormat& format) override;
	/** @see IAudioRender::closeAudio */
	virtual void closeAudio() override;
	/** @see IAudioRender::flushAudio */
	virtual void flushAudio() override;
	/** @see IAudioRender::pauseAudio */
	virtual void pauseAudio() override;
	/** @see IAudioRender::resumeAudio */
	virtual void resumeAudio() override;
	/** @see IAudioRender::renderAudio */
	virtual bool renderAudio(const LumaPlayerAudioFrame& frame) override;
	/** @see IAudioRender::bufferedDuration100ns */
	virtual int64_t bufferedDuration100ns() const override;

	/**
	* 获取全部音频计数的一致快照。
	* @return 返回加锁复制的计数，调用方不会读取正在变化的成员。
	*/
	LumaPlayerTestAudioStats stats() const;

private:
	/** 保护渲染计数，Core可能在工作线程回调渲染器。 */
	mutable std::mutex m_mutex;
	/** 当前音频打开次数。 */
	int32_t m_openCount;
	/** 当前音频关闭次数。 */
	int32_t m_closeCount;
	/** 当前音频清空次数。 */
	int32_t m_flushCount;
	/** 当前音频暂停次数。 */
	int32_t m_pauseCount;
	/** 当前音频恢复次数。 */
	int32_t m_resumeCount;
	/** 已收到音频帧数量。 */
	int32_t m_frameCount;
	/** 已收到PCM总字节数。 */
	size_t m_pcmBytes;
};

/** 测试视频渲染器的线程安全计数快照。 */
struct LumaPlayerTestVideoStats
{
	/** 视频打开次数。 */
	int32_t m_openCount;
	/** 视频关闭次数。 */
	int32_t m_closeCount;
	/** 已收到视频帧数量。 */
	int32_t m_frameCount;
	/** 最近一帧视频时间戳，单位100纳秒。 */
	int64_t m_lastTimestamp100ns;
	/** 最近一帧结束边界，单位100纳秒。 */
	int64_t m_lastEndTime100ns;
	/** 最近一帧像素数据字节数。 */
	size_t m_lastPixelBytes;
};

/** 测试用视频渲染器，只计数不显示窗口，避免测试依赖界面。 */
class LumaPlayerTestVideoRender : public IVideoRender
{
public:
	/** 构造计数清零的视频渲染桩。 */
	LumaPlayerTestVideoRender();

	/** @see IVideoRender::openVideo */
	virtual bool openVideo(const LumaPlayerVideoFormat& format) override;
	/** @see IVideoRender::closeVideo */
	virtual void closeVideo() override;
	/** @see IVideoRender::renderVideo */
	virtual bool renderVideo(const LumaPlayerVideoFrame& frame) override;

	/** 清空已记录的视频帧时间戳，用于开始一段独立的播放边界测试。 */
	void clearFrameHistory();

	/**
	* 获取视频帧时间戳历史的线程安全副本。
	* @return 返回按渲染先后排列的时间戳，单位100纳秒。
	*/
	std::vector<int64_t> frameHistory() const;

    /** 获取每帧实际提交的单调毫秒时间
    @return 与frameHistory顺序相同的时间数组，暂停后联合读取
    */
    std::vector<int64_t> frameWallTimes() const;

    /** 获取每帧首像素的BGR颜色，用于确定性切镜素材断言
    @return 与frameHistory顺序相同的颜色数组，暂停后联合读取
    */
    std::vector<uint32_t> frameColors() const;

	/** 获取最近渲染帧覆盖的左闭右开时间区间。 */
	bool lastFrameRange(int64_t* frameStart100ns, int64_t* frameEnd100ns) const;

	/**
	* 获取全部视频计数的一致快照。
	* @return 返回加锁复制的计数，调用方不会读取正在变化的成员。
	*/
	LumaPlayerTestVideoStats stats() const;

private:
	/** 保护渲染计数，Core可能在工作线程回调渲染器。 */
	mutable std::mutex m_mutex;
	/** 当前视频打开次数。 */
	int32_t m_openCount;
	/** 当前视频关闭次数。 */
	int32_t m_closeCount;
	/** 已收到视频帧数量。 */
	int32_t m_frameCount;
	/** 最近一帧视频时间戳，单位100纳秒。 */
	int64_t m_lastTimestamp100ns;
	/** 最近一帧结束边界，单位100纳秒。 */
	int64_t m_lastEndTime100ns;
	/** 最近一帧像素数据字节数。 */
	size_t m_lastPixelBytes;
	/** 按渲染顺序记录的视频时间戳，只用于测试断言。 */
	std::vector<int64_t> m_frameHistory;
    // 每帧提交时的单调毫秒时间
    std::vector<int64_t> m_frameWallTimes;
    // 每帧首像素的BGR颜色
    std::vector<uint32_t> m_frameColors;
};

/** 验证渲染回调内同步控制不会重入或等待自身
*/
class LumaPlayerTestReentryRender : public LumaPlayerTestVideoRender
{
public:
	/** 构造借用Core的回调测试器
	@param [in] core 待测Core，必须活到回调结束
	*/
	LumaPlayerTestReentryRender(LumaPlayerCore* core);

	/** 回调中尝试同步暂停并记录结果
	@param [in] frame 借用的视频帧
	@return 基类缓存是否成功
	*/
	virtual bool renderVideo(const LumaPlayerVideoFrame& frame) override;

	/** 获取最后一次重入调用结果
	@return Core结果枚举
	*/
	int32_t result() const;

private:
	// 借用Core，测试结束前不释放
	LumaPlayerCore* m_core;
	// 回调线程发布的同步调用结果
	std::atomic<int32_t> m_result;
};

/** 测试进程内部工具函数集合。 */
class LumaPlayerTestHelper
{
public:
	/**
	* 获取单调递增毫秒时间戳。
	* @return 返回当前steady_clock换算出的毫秒数。
	*/
	static int64_t nowMs();

	/**
	* 将Core返回值转换成报告文本。
	* @param [in] result Core返回值。
	* @return 返回便于阅读的结果名称。
	*/
	static const char* resultName(LumaPlayerCoreResult result);

	/**
	* 格式化Core操作结果。
	* @param [in] action 操作名称。
	* @param [in] result 操作结果。
	* @return 返回报告文本。
	*/
	static std::string resultMessage(const char* action, LumaPlayerCoreResult result);

	/**
	* 安全等待一小段时间，让异步播放线程有机会送出帧。
	* @param [in] milliseconds 等待时间，单位毫秒。
	*/
	static void sleepMs(int32_t milliseconds);
};

/** C API测试使用的渲染回调计数。 */
struct LumaPlayerTestCRenderState
{
public:
	/** 音频打开次数。 */
	std::atomic<int32_t> m_openAudioCount;
	/** 音频关闭次数。 */
	std::atomic<int32_t> m_closeAudioCount;
	/** 音频清空次数。 */
	std::atomic<int32_t> m_flushAudioCount;
	/** 音频暂停次数。 */
	std::atomic<int32_t> m_pauseAudioCount;
	/** 音频恢复次数。 */
	std::atomic<int32_t> m_resumeAudioCount;
	/** 音频帧数量。 */
	std::atomic<int32_t> m_audioFrameCount;
	/** 视频打开次数。 */
	std::atomic<int32_t> m_openVideoCount;
	/** 视频关闭次数。 */
	std::atomic<int32_t> m_closeVideoCount;
	/** 视频帧数量。 */
	std::atomic<int32_t> m_videoFrameCount;
	/** 最近一帧起始时间，单位100纳秒。 */
	std::atomic<int64_t> m_lastVideoStart100ns;
	/** 最近一帧结束边界，单位100纳秒。 */
	std::atomic<int64_t> m_lastVideoEnd100ns;

public:
	/** 构造一个清零的C回调状态。 */
	LumaPlayerTestCRenderState();
};

/** 验证外部输出异常不会让Core停留在打开中 */
class LumaPlayerTestThrowingVideoRender : public LumaPlayerTestVideoRender
{
public:
    /** 模拟视频输出打开失败的异常
    @param [in] format 视频格式
    @return 本测试实现固定抛出异常
    */
    virtual bool openVideo(const LumaPlayerVideoFormat& format);
};

/** 迁移的12组测试入口，参数和断言保留原语义
@param [in] caseId 原12组映射的稳定数字ID
@param [in] mediaPath 必须提供有效媒体
@param [in] sceneFixture 是否25fps红蓝切镜素材
@param [in] reportPath UTF8报告路径
@return 0通过，1失败，2缺素材
*/
int RunLegacyTest(int32_t caseId, const std::string& mediaPath, bool sceneFixture, const std::string& reportPath);