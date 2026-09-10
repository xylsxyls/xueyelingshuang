#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerTest.h"
#include "LumaPlayerCore/PlayerEngineHelper.h"
#include "LumaPlayerCore/MediaClock.h"

#include <climits>
#include <cstring>

#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <stdexcept>

int64_t LumaPlayerTestHelper::nowMs()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

const char* LumaPlayerTestHelper::resultName(LumaPlayerCoreResult result)
{
	switch (result)
	{
	case LumaPlayerCoreResultSuccess:
		return "Success";
	case LumaPlayerCoreResultInvalidParam:
		return "InvalidParam";
	case LumaPlayerCoreResultNotInit:
		return "NotInit";
	case LumaPlayerCoreResultNotOpen:
		return "NotOpen";
	case LumaPlayerCoreResultOpenFailed:
		return "OpenFailed";
	case LumaPlayerCoreResultStateError:
		return "StateError";
	case LumaPlayerCoreResultCanceled:
		return "Canceled";
	case LumaPlayerCoreResultTimeout:
		return "Timeout";
	case LumaPlayerCoreResultInternalError:
		return "InternalError";
	case LumaPlayerCoreResultEnd:
		return "End";
	default:
		return "Unknown";
	}
}

std::string LumaPlayerTestHelper::resultMessage(const char* action, LumaPlayerCoreResult result)
{
	std::ostringstream stream;
	stream << action << " result=" << resultName(result) << "(" << static_cast<int32_t>(result) << ")";
	return stream.str();
}

void LumaPlayerTestHelper::sleepMs(int32_t milliseconds)
{
	if (milliseconds > 0)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	}
}

LumaPlayerTestCRenderState::LumaPlayerTestCRenderState()
		: m_openAudioCount(0),
		  m_closeAudioCount(0),
		  m_flushAudioCount(0),
		  m_pauseAudioCount(0),
		  m_resumeAudioCount(0),
		  m_audioFrameCount(0),
		  m_openVideoCount(0),
		  m_closeVideoCount(0),
		  m_videoFrameCount(0),
		  m_lastVideoStart100ns(0),
		  m_lastVideoEnd100ns(0)
{
}

LumaPlayerTestCaseRecord::LumaPlayerTestCaseRecord()
	: m_passed(true),
	  m_costMs(0)
{
}

LumaPlayerTestReport::LumaPlayerTestReport()
	: m_caseStarted(false),
	  m_caseBeginMs(0)
{
}

void LumaPlayerTestReport::beginCase(const std::string& caseName)
{
    std::cout << "BEGIN " << caseName << std::endl;
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_caseStarted)
	{
		m_currentCase.m_costMs = LumaPlayerTestHelper::nowMs() - m_caseBeginMs;
		m_records.push_back(m_currentCase);
	}
	m_currentCase = LumaPlayerTestCaseRecord();
	m_currentCase.m_name = caseName;
	m_caseBeginMs = LumaPlayerTestHelper::nowMs();
	m_caseStarted = true;
}

void LumaPlayerTestReport::endCase()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (!m_caseStarted)
	{
		return;
	}
	m_currentCase.m_costMs = LumaPlayerTestHelper::nowMs() - m_caseBeginMs;
	m_records.push_back(m_currentCase);
	m_caseStarted = false;
}

void LumaPlayerTestReport::check(bool condition, const std::string& message)
{
    std::cout << (condition ? "PASS " : "FAIL ") << message << std::endl;
	std::ostringstream stream;
	stream << (condition ? "[PASS] " : "[FAIL] ") << message;
	std::lock_guard<std::mutex> lock(m_mutex);
	if (!m_caseStarted)
	{
		return;
	}
	m_currentCase.m_messages.push_back(stream.str());
	if (!condition)
	{
		m_currentCase.m_passed = false;
	}
}

void LumaPlayerTestReport::info(const std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_caseStarted)
	{
		m_currentCase.m_messages.push_back(std::string("[INFO] ") + message);
	}
}

int32_t LumaPlayerTestReport::failedCount() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	int32_t failed = 0;
	for (size_t i = 0; i < m_records.size(); ++i)
	{
		if (!m_records[i].m_passed)
		{
			++failed;
		}
	}
	return failed;
}

void LumaPlayerTestReport::writeReport(const std::string& filePath) const
{
	std::ostringstream stream;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		int32_t failed = 0;
		for (size_t i = 0; i < m_records.size(); ++i)
		{
			if (!m_records[i].m_passed)
			{
				++failed;
			}
		}
		stream << "LumaPlayerCore Test Report" << std::endl;
		stream << "caseCount=" << m_records.size() << ", failedCount=" << failed << std::endl;
		for (size_t i = 0; i < m_records.size(); ++i)
		{
			const LumaPlayerTestCaseRecord& record = m_records[i];
			stream << std::endl;
			stream << (record.m_passed ? "[PASS] " : "[FAIL] ") << record.m_name << ", costMs=" << record.m_costMs << std::endl;
			for (size_t j = 0; j < record.m_messages.size(); ++j)
			{
				stream << "  " << record.m_messages[j] << std::endl;
			}
		}
	}

	std::cout << stream.str();
	std::ofstream file(filePath.c_str(), std::ios::out | std::ios::trunc);
	if (file.is_open())
	{
		file << stream.str();
	}
}

LumaPlayerTestAudioRender::LumaPlayerTestAudioRender()
	: m_openCount(0),
	  m_closeCount(0),
	  m_flushCount(0),
	  m_pauseCount(0),
	  m_resumeCount(0),
	  m_frameCount(0),
	  m_pcmBytes(0)
{
}

bool LumaPlayerTestAudioRender::openAudio(const LumaPlayerAudioFormat& format)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	++m_openCount;
	return format.isValid();
}

void LumaPlayerTestAudioRender::closeAudio()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	++m_closeCount;
}

void LumaPlayerTestAudioRender::flushAudio()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	++m_flushCount;
}

void LumaPlayerTestAudioRender::pauseAudio()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	++m_pauseCount;
}

void LumaPlayerTestAudioRender::resumeAudio()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	++m_resumeCount;
}

bool LumaPlayerTestAudioRender::renderAudio(const LumaPlayerAudioFrame& frame)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	++m_frameCount;
	m_pcmBytes += frame.m_pcmData.size();
	return !frame.empty();
}

int64_t LumaPlayerTestAudioRender::bufferedDuration100ns() const
{
	return 0;
}

LumaPlayerTestAudioStats LumaPlayerTestAudioRender::stats() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	LumaPlayerTestAudioStats result = {};
	result.m_openCount = m_openCount;
	result.m_closeCount = m_closeCount;
	result.m_flushCount = m_flushCount;
	result.m_pauseCount = m_pauseCount;
	result.m_resumeCount = m_resumeCount;
	result.m_frameCount = m_frameCount;
	result.m_pcmBytes = m_pcmBytes;
	return result;
}

LumaPlayerTestVideoRender::LumaPlayerTestVideoRender()
	: m_openCount(0),
	  m_closeCount(0),
	  m_frameCount(0),
	  m_lastTimestamp100ns(0),
	  m_lastEndTime100ns(0),
	  m_lastPixelBytes(0)
{
}

bool LumaPlayerTestVideoRender::openVideo(const LumaPlayerVideoFormat& format)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	++m_openCount;
	return format.isValid();
}

void LumaPlayerTestVideoRender::closeVideo()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	++m_closeCount;
}

bool LumaPlayerTestVideoRender::renderVideo(const LumaPlayerVideoFrame& frame)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	++m_frameCount;
	m_lastTimestamp100ns = frame.m_timestamp100ns;
	m_lastEndTime100ns = frame.endTime100ns();
	m_lastPixelBytes = frame.m_bgraPixels.size();
	try
	{
		m_frameHistory.push_back(frame.m_timestamp100ns);
        m_frameWallTimes.push_back(LumaPlayerTestHelper::nowMs());
        uint32_t color = frame.m_bgraPixels.size() >= 3 ? static_cast<uint32_t>(frame.m_bgraPixels[0]) |
            (static_cast<uint32_t>(frame.m_bgraPixels[1]) << 8) | (static_cast<uint32_t>(frame.m_bgraPixels[2]) << 16) : 0;
        m_frameColors.push_back(color);
	}
	catch (...)
	{
		return false;
	}
	return !frame.empty();
}

void LumaPlayerTestVideoRender::clearFrameHistory()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_frameHistory.clear();
	m_frameWallTimes.clear();
	m_frameColors.clear();
}

std::vector<int64_t> LumaPlayerTestVideoRender::frameHistory() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_frameHistory;
}

std::vector<int64_t> LumaPlayerTestVideoRender::frameWallTimes() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_frameWallTimes;
}

std::vector<uint32_t> LumaPlayerTestVideoRender::frameColors() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_frameColors;
}

bool LumaPlayerTestVideoRender::lastFrameRange(int64_t* frameStart100ns, int64_t* frameEnd100ns) const
{
	if (frameStart100ns == nullptr || frameEnd100ns == nullptr)
	{
		return false;
	}
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_frameCount <= 0 || m_lastEndTime100ns <= m_lastTimestamp100ns)
	{
		return false;
	}
	*frameStart100ns = m_lastTimestamp100ns;
	*frameEnd100ns = m_lastEndTime100ns;
	return true;
}

LumaPlayerTestVideoStats LumaPlayerTestVideoRender::stats() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	LumaPlayerTestVideoStats result = {};
	result.m_openCount = m_openCount;
	result.m_closeCount = m_closeCount;
	result.m_frameCount = m_frameCount;
	result.m_lastTimestamp100ns = m_lastTimestamp100ns;
	result.m_lastEndTime100ns = m_lastEndTime100ns;
	result.m_lastPixelBytes = m_lastPixelBytes;
	return result;
}

static int32_t cOpenAudioCallback(const LumaPlayerCoreCAudioFormat* format, void* userData)
{
	LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
	if (state != nullptr)
	{
		++state->m_openAudioCount;
	}
	return format != nullptr && format->m_sampleRate > 0 ? 1 : 0;
}

static void cCloseAudioCallback(void* userData)
{
	LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
	if (state != nullptr)
	{
		++state->m_closeAudioCount;
	}
}

static void cFlushAudioCallback(void* userData)
{
	LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
	if (state != nullptr)
	{
		++state->m_flushAudioCount;
	}
}

static void cPauseAudioCallback(void* userData)
{
	LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
	if (state != nullptr)
	{
		++state->m_pauseAudioCount;
	}
}

static void cResumeAudioCallback(void* userData)
{
	LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
	if (state != nullptr)
	{
		++state->m_resumeAudioCount;
	}
}

static int32_t cRenderAudioCallback(const LumaPlayerCoreCAudioFrame* frame, void* userData)
{
	LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
	if (state != nullptr)
	{
		++state->m_audioFrameCount;
	}
	return frame != nullptr && frame->m_pcmData != nullptr && frame->m_pcmSize > 0 ? 1 : 0;
}

static int64_t cBufferedAudioCallback(void* userData)
{
	LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
	if (state != nullptr)
	{
		return 0;
	}
	return 0;
}

static int32_t cOpenVideoCallback(const LumaPlayerCoreCVideoFormat* format, void* userData)
{
	LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
	if (state != nullptr)
	{
		++state->m_openVideoCount;
	}
	return format != nullptr && format->m_width > 0 && format->m_height > 0 ? 1 : 0;
}

static void cCloseVideoCallback(void* userData)
{
	LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
	if (state != nullptr)
	{
		++state->m_closeVideoCount;
	}
}

static int32_t cRenderVideoCallback(const LumaPlayerCoreCVideoFrame* frame, void* userData)
{
	bool validFrame = frame != nullptr && frame->m_bgraPixels != nullptr && frame->m_bgraSize > 0;
	LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
	if (state != nullptr && validFrame)
	{
		++state->m_videoFrameCount;
		state->m_lastVideoStart100ns = frame->m_timestamp100ns;
		state->m_lastVideoEnd100ns = frame->m_timestamp100ns + frame->m_duration100ns;
	}
	return validFrame ? 1 : 0;
}

static void runCppLifecycleCase(LumaPlayerTestReport& report)
{
	report.beginCase("C++接口生命周期和错误路径");
	LumaPlayerCore core;
	LumaPlayerTestAudioRender audioRender;
	LumaPlayerTestVideoRender videoRender;
	core.setAudioRender(&audioRender);
	core.setVideoRender(&videoRender);
	core.setLogEnabled(true);

	report.check(!core.isInit(), "构造后Core尚未初始化");
	report.check(core.play() == LumaPlayerCoreResultNotInit, "未初始化时play返回NotInit");
	report.check(core.openMediaAsync("missing.mp4") == LumaPlayerCoreResultNotInit, "未初始化时异步打开返回NotInit");
	LumaPlayerCoreResult result = core.init();
	report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("init", result));
	report.check(core.isInit(), "init后Core进入可用状态");
	report.check(core.stop() == LumaPlayerCoreResultNotOpen, "未打开媒体时stop返回NotOpen");
	report.check(core.seekTo(0, false) == LumaPlayerCoreResultNotOpen, "未打开媒体时seek返回NotOpen");
	report.check(core.previewFrame(0) == LumaPlayerCoreResultNotOpen, "未打开媒体时preview返回NotOpen");
	result = core.setPlaybackRatePermille(50);
	report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("set low rate", result));
	report.check(core.snapshot().m_ratePermille == 100, "低于下限的倍速会裁剪到0.1倍");
	result = core.setPlaybackRatePermille(6000);
	report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("set high rate", result));
	report.check(core.snapshot().m_ratePermille == 3000, "高于上限的倍速会裁剪到3.0倍");
	result = core.openMedia("");
	report.check(result != LumaPlayerCoreResultSuccess, "空路径打开失败且不崩溃");
	core.uninit();
	report.check(!core.isInit(), "uninit后Core退出可用状态");
	report.endCase();
}

static void runCppRepeatedInitCase(LumaPlayerTestReport& report)
{
	report.beginCase("C++接口重复初始化压力");
	for (int32_t i = 0; i < 20; ++i)
	{
		LumaPlayerCore core;
		LumaPlayerCoreResult result = core.init();
		std::ostringstream stream;
		stream << "第" << i << "次init返回" << LumaPlayerTestHelper::resultName(result);
		report.check(result == LumaPlayerCoreResultSuccess, stream.str());
		core.uninit();
		report.check(!core.isInit(), "重复uninit后Core状态正确");
	}
	report.endCase();
}

static void runCApiLifecycleCase(LumaPlayerTestReport& report)
{
	report.beginCase("C接口生命周期和错误路径");
	LumaPlayerCoreHandle handle = lumaPlayerCoreCreate();
	report.check(handle != nullptr, "C接口create返回非空句柄");
	if (handle == nullptr)
	{
		report.endCase();
		return;
	}

	LumaPlayerTestCRenderState renderState;
	LumaPlayerCoreCRenderCallbacks callbacks;
	callbacks.m_openAudio = cOpenAudioCallback;
	callbacks.m_closeAudio = cCloseAudioCallback;
	callbacks.m_flushAudio = cFlushAudioCallback;
	callbacks.m_pauseAudio = cPauseAudioCallback;
	callbacks.m_resumeAudio = cResumeAudioCallback;
	callbacks.m_renderAudio = cRenderAudioCallback;
	callbacks.m_bufferedAudio = cBufferedAudioCallback;
	callbacks.m_openVideo = cOpenVideoCallback;
	callbacks.m_closeVideo = cCloseVideoCallback;
	callbacks.m_renderVideo = cRenderVideoCallback;
	callbacks.m_userData = &renderState;

	lumaPlayerCoreSetLogEnabled(handle, 1);

	lumaPlayerCoreSetRenderCallbacks(handle, &callbacks);
	report.check(lumaPlayerCorePlay(handle) == LumaPlayerCoreCResultNotInit, "C接口未初始化play返回NotInit");
	report.check(lumaPlayerCoreOpenMediaAsync(handle, "missing.mp4") == LumaPlayerCoreCResultNotInit, "C接口未初始化时异步打开返回NotInit");
	report.check(lumaPlayerCoreSetLoopAAtPositionAsync(handle, 0) == LumaPlayerCoreCResultNotInit, "C接口未初始化时异步设置A点返回NotInit");
	report.check(lumaPlayerCoreClearLoopAsync(handle) == LumaPlayerCoreCResultNotInit, "C接口未初始化时异步清空AB点返回NotInit");
	report.check(lumaPlayerCoreMoveLoopPointAsync(handle, 0, 1) == LumaPlayerCoreCResultNotInit, "C接口未初始化时异步逐帧返回NotInit");
	report.check(lumaPlayerCoreSetPlaybackRatePermilleAsync(handle, 1100) == LumaPlayerCoreCResultNotInit, "C接口未初始化时异步倍速返回NotInit");
	int32_t result = lumaPlayerCoreInit(handle);
	report.check(result == LumaPlayerCoreCResultSuccess, "C接口init成功");
	report.check(lumaPlayerCoreStop(handle) == LumaPlayerCoreCResultNotOpen, "C接口未打开媒体stop返回NotOpen");
	result = lumaPlayerCoreSetPlaybackRatePermille(handle, 40);
	report.check(result == LumaPlayerCoreCResultSuccess, "C接口设置倍速成功");
	LumaPlayerCoreCSnapshot snapshot;
	report.check(lumaPlayerCoreGetSnapshot(handle, &snapshot) != 0, "C接口获取快照成功");
	report.check(snapshot.m_ratePermille == 100, "C接口倍速下限裁剪正确");
	result = lumaPlayerCoreOpenMedia(handle, "");
	report.check(result != LumaPlayerCoreCResultSuccess, "C接口空路径打开失败且不崩溃");
	report.check(lumaPlayerCoreOpenMedia(handle, nullptr) == LumaPlayerCoreCResultInvalidParam, "C接口空路径指针返回InvalidParam");
	report.check(lumaPlayerCoreMoveLoopPoint(handle, 2, 1) == LumaPlayerCoreCResultInvalidParam, "C接口拒绝无效循环点类型");
	lumaPlayerCoreUninit(handle);
	lumaPlayerCoreDestroy(handle);
	report.endCase();
}

/** 通过纯C ABI验证真实媒体和异步控制，覆盖手机端或其他语言绑定使用的入口。 */
static void runCApiMediaCase(LumaPlayerTestReport& report, const std::string& mediaPath)
{
	report.beginCase("C接口真实媒体和异步控制");
	if (mediaPath.empty())
	{
		report.info("未传入媒体路径，C接口真实解码用例跳过。");
		report.endCase();
		return;
	}

	LumaPlayerCoreHandle handle = lumaPlayerCoreCreate();
	report.check(handle != nullptr, "C接口为真实媒体创建Core句柄");
	if (handle == nullptr)
	{
		report.endCase();
		return;
	}

	LumaPlayerTestCRenderState renderState;
	LumaPlayerCoreCRenderCallbacks callbacks = {};
	callbacks.m_openAudio = cOpenAudioCallback;
	callbacks.m_closeAudio = cCloseAudioCallback;
	callbacks.m_flushAudio = cFlushAudioCallback;
	callbacks.m_pauseAudio = cPauseAudioCallback;
	callbacks.m_resumeAudio = cResumeAudioCallback;
	callbacks.m_renderAudio = cRenderAudioCallback;
	callbacks.m_bufferedAudio = cBufferedAudioCallback;
	callbacks.m_openVideo = cOpenVideoCallback;
	callbacks.m_closeVideo = cCloseVideoCallback;
	callbacks.m_renderVideo = cRenderVideoCallback;
	callbacks.m_userData = &renderState;
	lumaPlayerCoreSetRenderCallbacks(handle, &callbacks);

	lumaPlayerCoreSetLogEnabled(handle, 1);

	int32_t result = lumaPlayerCoreInit(handle);
	report.check(result == LumaPlayerCoreCResultSuccess, "C接口真实媒体Core初始化成功");
	if (result != LumaPlayerCoreCResultSuccess)
	{
		lumaPlayerCoreDestroy(handle);
		report.endCase();
		return;
	}

	LumaPlayerCoreCSnapshot snapshot = {};
	lumaPlayerCoreGetSnapshot(handle, &snapshot);
	uint64_t mediaSerialBeforeOpen = snapshot.m_mediaSerial;
	result = lumaPlayerCoreOpenMediaAsync(handle, mediaPath.c_str());
	report.check(result == LumaPlayerCoreCResultSuccess, "C接口异步打开命令成功入队");
	for (int32_t waitIndex = 0; waitIndex < 2000 && result == LumaPlayerCoreCResultSuccess; ++waitIndex)
	{
		if (!lumaPlayerCoreGetSnapshot(handle, &snapshot))
		{
			break;
		}
		if (snapshot.m_state == LumaPlayerCoreCStatePaused || snapshot.m_state == LumaPlayerCoreCStateError)
		{
			break;
		}
		LumaPlayerTestHelper::sleepMs(5);
	}
	report.check(snapshot.m_state == LumaPlayerCoreCStatePaused, "C接口异步打开完成后进入Paused状态");
	report.check(snapshot.m_mediaSerial > mediaSerialBeforeOpen, "C接口媒体序号在成功打开后递增");
	report.check(snapshot.m_duration100ns > 0, "C接口快照返回有效媒体时长");
	report.check(snapshot.m_hasAudio != 0 || snapshot.m_hasVideo != 0, "C接口快照至少包含一路媒体流");
	if (snapshot.m_state != LumaPlayerCoreCStatePaused)
	{
		char errorBuffer[1024] = { 0 };
		lumaPlayerCoreGetLastError(handle, errorBuffer, sizeof(errorBuffer));
		report.info(std::string("C API lastError=") + errorBuffer);
		lumaPlayerCoreDestroy(handle);
		report.endCase();
		return;
	}

	if (snapshot.m_hasAudio != 0)
	{
		report.check(renderState.m_openAudioCount.load() > 0, "C接口音频回调收到openAudio");
	}
	if (snapshot.m_hasVideo != 0)
	{
		report.check(renderState.m_openVideoCount.load() > 0, "C接口视频回调收到openVideo");
		report.check(renderState.m_videoFrameCount.load() > 0, "C接口打开媒体时收到首帧");
	}

	int64_t frameDuration100ns = snapshot.m_defaultVideoFrameDuration100ns > 0 ? snapshot.m_defaultVideoFrameDuration100ns : 400000;
	int64_t previewPosition100ns = snapshot.m_duration100ns / 2;
	if (previewPosition100ns >= snapshot.m_duration100ns)
	{
		previewPosition100ns = snapshot.m_duration100ns - 1;
	}
	if (previewPosition100ns < 0)
	{
		previewPosition100ns = 0;
	}
	if (snapshot.m_hasVideo != 0)
	{
		int32_t videoCountBeforePreview = renderState.m_videoFrameCount.load();
		result = lumaPlayerCorePreviewFrame(handle, previewPosition100ns);
		report.check(result == LumaPlayerCoreCResultSuccess, "C接口同步预览目标帧成功");
		int64_t frameStart100ns = renderState.m_lastVideoStart100ns.load();
		int64_t frameEnd100ns = renderState.m_lastVideoEnd100ns.load();
		report.check(renderState.m_videoFrameCount.load() > videoCountBeforePreview, "C接口预览送出新视频帧");
		report.check(frameStart100ns <= previewPosition100ns && previewPosition100ns < frameEnd100ns,
			"C接口预览帧的左闭右开区间覆盖目标时间");
	}

	result = lumaPlayerCoreSetLoopAAtPosition(handle, 0);
	report.check(result == LumaPlayerCoreCResultSuccess, "C接口按位置设置A点成功");
	result = lumaPlayerCoreSetLoopBAtPosition(handle, previewPosition100ns);
	report.check(result == LumaPlayerCoreCResultSuccess, "C接口按位置设置B点成功");
	lumaPlayerCoreGetSnapshot(handle, &snapshot);
	report.check(snapshot.m_hasLoopA != 0 && snapshot.m_hasLoopB != 0, "C接口快照返回完整AB点");
	report.check(snapshot.m_loopAStart100ns < snapshot.m_loopBEnd100ns, "C接口AB点保持左闭右开有效区间");
	if (snapshot.m_hasVideo != 0 && snapshot.m_duration100ns > frameDuration100ns * 4)
	{
		result = lumaPlayerCoreMoveLoopPoint(handle, LumaPlayerCoreCLoopPointB, 1);
		report.check(result == LumaPlayerCoreCResultSuccess, "C接口将B点向右移动一帧");
		if (result == LumaPlayerCoreCResultSuccess)
		{
			result = lumaPlayerCoreMoveLoopPoint(handle, LumaPlayerCoreCLoopPointB, -1);
			report.check(result == LumaPlayerCoreCResultSuccess, "C接口将B点向左移回一帧");
		}
	}

	result = lumaPlayerCoreClearLoop(handle);
	report.check(result == LumaPlayerCoreCResultSuccess, "C接口清除AB点成功");
	result = lumaPlayerCoreSeekTo(handle, 0, 0);
	report.check(result == LumaPlayerCoreCResultSuccess, "C接口播放前seek到起点成功");
	int32_t resumeCountBeforePlay = renderState.m_resumeAudioCount.load();
	result = lumaPlayerCorePlayAsync(handle);
	report.check(result == LumaPlayerCoreCResultSuccess, "C接口异步播放命令成功入队");
	for (int32_t waitIndex = 0; waitIndex < 400; ++waitIndex)
	{
		lumaPlayerCoreGetSnapshot(handle, &snapshot);
		if (snapshot.m_state == LumaPlayerCoreCStatePlaying || snapshot.m_state == LumaPlayerCoreCStateError)
		{
			break;
		}
		LumaPlayerTestHelper::sleepMs(5);
	}
	report.check(snapshot.m_state == LumaPlayerCoreCStatePlaying, "C接口异步播放切换到Playing状态");
	LumaPlayerTestHelper::sleepMs(200);
	int32_t pauseCountBeforePause = renderState.m_pauseAudioCount.load();
	result = lumaPlayerCorePauseAsync(handle);
	report.check(result == LumaPlayerCoreCResultSuccess, "C接口异步暂停命令成功入队");
	for (int32_t waitIndex = 0; waitIndex < 400; ++waitIndex)
	{
		lumaPlayerCoreGetSnapshot(handle, &snapshot);
		if (snapshot.m_state == LumaPlayerCoreCStatePaused || snapshot.m_state == LumaPlayerCoreCStateError)
		{
			break;
		}
		LumaPlayerTestHelper::sleepMs(5);
	}
	report.check(snapshot.m_state == LumaPlayerCoreCStatePaused, "C接口异步暂停切换到Paused状态");
	if (snapshot.m_hasAudio != 0)
	{
		report.check(renderState.m_resumeAudioCount.load() > resumeCountBeforePlay, "C接口播放恢复音频设备");
		report.check(renderState.m_pauseAudioCount.load() > pauseCountBeforePause, "C接口暂停音频设备且保留缓存");
	}

	char errorBuffer[16] = { 0 };
	size_t requiredErrorSize = lumaPlayerCoreGetLastError(handle, errorBuffer, sizeof(errorBuffer));
	report.check(requiredErrorSize >= 1 && errorBuffer[sizeof(errorBuffer) - 1] == '\0', "C接口错误缓冲区契约有效");
	lumaPlayerCoreCloseMedia(handle);
	lumaPlayerCoreUninit(handle);
	lumaPlayerCoreDestroy(handle);
	report.endCase();
}

static void runMediaCase(LumaPlayerTestReport& report, const std::string& mediaPath)
{
	report.beginCase("真实媒体打开、预览、seek和AB循环");
	if (mediaPath.empty())
	{
		report.info("未传入媒体路径，真实解码用例跳过。用法：LumaPlayerTestd.exe test.mp4");
		report.endCase();
		return;
	}

	LumaPlayerCore core;
	LumaPlayerTestAudioRender audioRender;
	LumaPlayerTestVideoRender videoRender;
	core.setAudioRender(&audioRender);
	core.setVideoRender(&videoRender);
	core.setLogEnabled(true);

	LumaPlayerCoreResult result = core.init();
	report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("init", result));
	if (result != LumaPlayerCoreResultSuccess)
	{
		report.endCase();
		return;
	}

	result = core.openMediaAsync(mediaPath);
	report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("openMediaAsync post", result));
	LumaPlayerSnapshot snapshot = core.snapshot();
	if (result == LumaPlayerCoreResultSuccess)
	{
		const int32_t maxWaitCount = 2000;
		for (int32_t waitIndex = 0; waitIndex < maxWaitCount; ++waitIndex)
		{
			snapshot = core.snapshot();
			if (snapshot.m_state == LumaPlayerCoreStatePaused || snapshot.m_state == LumaPlayerCoreStateError)
			{
				break;
			}
			LumaPlayerTestHelper::sleepMs(5);
		}
	}
	report.check(snapshot.m_state == LumaPlayerCoreStatePaused, "异步打开完成后快照进入Paused状态");
	if (result != LumaPlayerCoreResultSuccess || snapshot.m_state != LumaPlayerCoreStatePaused)
	{
		report.info(std::string("lastError=") + core.lastError());
		core.uninit();
		report.endCase();
		return;
	}

	report.check(snapshot.m_mediaInfo.m_duration100ns > 0, "媒体总时长有效");
	report.check(snapshot.m_mediaInfo.m_hasVideo || snapshot.m_mediaInfo.m_hasAudio, "媒体至少有一路可播放流");
	if (snapshot.m_mediaInfo.m_hasVideo)
	{
		report.check(videoRender.stats().m_openCount > 0, "打开媒体时视频渲染器收到openVideo");
		report.check(videoRender.stats().m_frameCount > 0, "打开媒体时预览首帧");
	}

	int64_t frameDuration = snapshot.m_mediaInfo.m_defaultVideoFrameDuration100ns > 0 ? snapshot.m_mediaInfo.m_defaultVideoFrameDuration100ns : 400000;
	int64_t previewPosition = frameDuration * 2;
	if (snapshot.m_mediaInfo.m_duration100ns > frameDuration * 4)
	{
		previewPosition = snapshot.m_mediaInfo.m_duration100ns / 2;
	}
	if (previewPosition >= snapshot.m_mediaInfo.m_duration100ns)
	{
		previewPosition = snapshot.m_mediaInfo.m_duration100ns - 1;
	}
	if (previewPosition < 0)
	{
		previewPosition = 0;
	}

	result = core.previewFrame(0);
	report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("preview 0", result));
	result = core.setLoopAAtPosition(0);
	report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("set A by position", result));
	result = core.previewFrame(previewPosition);
	report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("preview B", result));
	int64_t previewFrameStart100ns = 0;
	int64_t previewFrameEnd100ns = 0;
	bool havePreviewRange = videoRender.lastFrameRange(&previewFrameStart100ns, &previewFrameEnd100ns);
	report.check(havePreviewRange && previewFrameStart100ns <= previewPosition && previewPosition < previewFrameEnd100ns,
		"预览帧的左闭右开时间区间覆盖目标时间，不会选到下一帧" );
	result = core.setLoopBAtPosition(previewPosition);
	report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("set B by position", result));
	report.check(core.snapshot().m_loopRange.isValid(), "AB点形成左闭右开循环区间");
	LumaPlayerSnapshot loopPointSnapshot = core.snapshot();
	int64_t originalBFrameStart100ns = loopPointSnapshot.m_loopRange.m_bPoint.m_frameStart100ns;
	result = core.moveLoopPoint(LumaPlayerCoreLoopPointB, 1);
	report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("move B right one frame", result));
	loopPointSnapshot = core.snapshot();
	report.check(loopPointSnapshot.m_loopRange.m_bPoint.m_frameStart100ns > originalBFrameStart100ns,
		"B点右移一帧会落到实际相邻视频帧");
	result = core.moveLoopPoint(LumaPlayerCoreLoopPointB, -1);
	report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("move B left one frame", result));
	loopPointSnapshot = core.snapshot();
	report.check(loopPointSnapshot.m_loopRange.m_bPoint.m_frameStart100ns == originalBFrameStart100ns,
		"B点向右再向左逐帧移动会回到原视频帧");

	if (snapshot.m_mediaInfo.m_hasVideo && snapshot.m_mediaInfo.m_duration100ns > frameDuration * 8)
	{
		result = core.setLoopAAtPosition(0);
		report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("set short-loop A", result));
		result = core.setLoopBAtPosition(frameDuration * 5);
		report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("set short-loop B", result));
		LumaPlayerSnapshot loopSnapshot = core.snapshot();
		result = core.seekTo(loopSnapshot.m_loopRange.m_aPoint.m_frameStart100ns, false);
		report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("seek short-loop A", result));
		videoRender.clearFrameHistory();
		result = core.play();
		report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("play short loop", result));
		LumaPlayerTestHelper::sleepMs(700);
		core.pause();
		std::vector<int64_t> frameHistory = videoRender.frameHistory();
		bool sawBFrame = false;
		bool sawReplayAfterB = false;
		bool stayedBeforeBEnd = true;
		for (size_t frameIndex = 0; frameIndex < frameHistory.size(); ++frameIndex)
		{
			int64_t frameTimestamp100ns = frameHistory[frameIndex];
			if (frameTimestamp100ns == loopSnapshot.m_loopRange.m_bPoint.m_frameStart100ns)
			{
				sawBFrame = true;
			}
			else if (sawBFrame && frameTimestamp100ns == loopSnapshot.m_loopRange.m_aPoint.m_frameStart100ns)
			{
				sawReplayAfterB = true;
			}
			if (frameTimestamp100ns >= loopSnapshot.m_loopRange.m_bPoint.m_frameEnd100ns)
			{
				stayedBeforeBEnd = false;
			}
		}
		report.check(sawBFrame, "短AB循环会显示B点所在的最后一帧，不会提前数帧回跳");
		report.check(sawReplayAfterB, "显示B点最后一帧后会回到A点继续播放");
		report.check(stayedBeforeBEnd, "左闭右开循环不会显示B点右边界后的帧");
	}
	result = core.clearLoop();
	report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("clear loop before pause test", result));
	result = core.seekTo(0, false);
	report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("seek 0 before pause test", result));
	int32_t resumeCountBeforePlay = audioRender.stats().m_resumeCount;
	result = core.play();
	report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("play", result));
	if (snapshot.m_mediaInfo.m_hasAudio)
	{
		report.check(audioRender.stats().m_resumeCount > resumeCountBeforePlay, "播放命令会恢复音频设备");
	}
	LumaPlayerTestHelper::sleepMs(300);
	int32_t flushCountBeforePause = audioRender.stats().m_flushCount;
	int32_t pauseCountBeforePause = audioRender.stats().m_pauseCount;
	result = core.pause();
	report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("pause", result));
	if (snapshot.m_mediaInfo.m_hasAudio)
	{
		report.check(audioRender.stats().m_pauseCount > pauseCountBeforePause, "暂停命令会暂停音频设备");
		report.check(audioRender.stats().m_flushCount == flushCountBeforePause, "暂停命令保留音频缓存且不会触发flush");
	}
	snapshot = core.snapshot();
	report.check(snapshot.m_state == LumaPlayerCoreStatePaused, "暂停后状态为Paused");
	if (snapshot.m_mediaInfo.m_hasVideo)
	{
		report.check(videoRender.stats().m_frameCount > 0, "短时播放期间收到视频帧或至少首帧");
	}
	if (snapshot.m_mediaInfo.m_hasAudio)
	{
		report.check(audioRender.stats().m_openCount > 0, "打开媒体时音频渲染器收到openAudio");
	}
	core.closeMedia();
	core.uninit();
	report.endCase();
}

/** 验证C与C++配置入口、未初始化保护和统一中文错误描述
@param [in,out] report 当前测试报告
@param [in] path 媒体素材路径，用于验证配置实际生效
*/
static void RunConfigurationCase(LumaPlayerTestReport& report, const std::string& path)
{
    report.beginCase("ConfigurationAndErrors");
    LumaPlayerCore core;
    report.check(core.play() == LumaPlayerCoreResultNotInit, "未初始化时拒绝读取配置和提交播放命令");
    LumaPlayerCoreConfig options;
    options.m_idleWaitMs = 0;
    report.check(core.init(options) == LumaPlayerCoreResultInvalidParam, "拒绝无效等待配置");
    report.check(!core.isInit(), "初始化失败后保持未初始化状态");
    options.m_idleWaitMs = 2;
    options.m_controlCommandTimeoutMs = 1000;
    report.check(core.init(options) == LumaPlayerCoreResultSuccess, "接受外部配置");
    report.check(core.openMedia("Z:/LumaPlayerTest/nonexistent-file.mp4") == LumaPlayerCoreResultMediaInfoOpenFailed, "文件不存在返回明确的媒体信息读取错误");
    report.check(core.lastError() == kLumaPlayerCoreResultDescriptions.at(LumaPlayerCoreResultMediaInfoOpenFailed), "错误描述来自统一中文映射");
    core.uninit();
    options.m_idleWaitMs = 3;
    report.check(core.init(options) == LumaPlayerCoreResultSuccess, "反初始化后允许使用新配置初始化");
    core.uninit();
    LumaPlayerCoreCConfig cOptions;
    report.check(lumaPlayerCoreDefaultConfig(&cOptions) == LumaPlayerCoreCResultSuccess, "C调用方获得默认配置");
    report.check(lumaPlayerCoreDefaultConfig(nullptr) == LumaPlayerCoreCResultInvalidParam, "C默认配置接口检查空指针");
    LumaPlayerCoreHandle handle = lumaPlayerCoreCreate();
    report.check(handle != nullptr, "创建C句柄");
    report.check(lumaPlayerCoreInitWithConfig(handle, nullptr) == LumaPlayerCoreCResultInvalidParam, "C初始化接口检查空配置");
    cOptions.m_controlCommandTimeoutMs = 0;
    report.check(lumaPlayerCoreInitWithConfig(handle, &cOptions) == LumaPlayerCoreCResultInvalidParam, "C和C++保持一致的参数校验");
    cOptions.m_controlCommandTimeoutMs = 1000;
    report.check(lumaPlayerCoreInitWithConfig(handle, &cOptions) == LumaPlayerCoreCResultSuccess, "C接口接受外部配置");
    lumaPlayerCoreUninit(handle);
    LumaPlayerCoreConfig defaults;
    report.check(cOptions.m_defaultFrameDuration100ns == defaults.m_defaultFrameDuration100ns &&
        cOptions.m_defaultRatePermille == 1000 && cOptions.m_minRatePermille == 100 && cOptions.m_maxRatePermille == 3000,
        "C与C++新增配置默认值一致，1000表示1倍速");
    options = defaults;
    options.m_defaultFrameDuration100ns = 0;
    report.check(core.init(options) == LumaPlayerCoreResultInvalidParam, "拒绝零兜底帧时长");
    options = defaults;
    options.m_minRatePermille = 0;
    report.check(core.init(options) == LumaPlayerCoreResultInvalidParam, "拒绝非正最低倍率");
    options = defaults;
    options.m_minRatePermille = 2000;
    options.m_maxRatePermille = 1000;
    report.check(core.init(options) == LumaPlayerCoreResultInvalidParam, "拒绝倒置倍率范围");
    cOptions.m_defaultRatePermille = 4000;
    report.check(lumaPlayerCoreInitWithConfig(handle, &cOptions) == LumaPlayerCoreCResultInvalidParam, "C接口拒绝范围外默认倍率");
    options = defaults;
    options.m_defaultFrameDuration100ns = 700000;
    options.m_defaultRatePermille = 1500;
    options.m_minRatePermille = 50;
    options.m_maxRatePermille = 4000;
    cOptions.m_defaultFrameDuration100ns = 900000;
    cOptions.m_defaultRatePermille = 2000;
    cOptions.m_minRatePermille = 500;
    cOptions.m_maxRatePermille = 5000;
    report.check(core.init(options) == LumaPlayerCoreResultSuccess, "C++接受自定义倍率及帧时长");
    report.check(lumaPlayerCoreInitWithConfig(handle, &cOptions) == LumaPlayerCoreCResultSuccess, "C接受另一套独立配置");
    LumaPlayerCoreCSnapshot cSnapshot = {};
    report.check(lumaPlayerCoreGetSnapshot(handle, &cSnapshot) != 0 && cSnapshot.m_ratePermille == 2000 &&
        cSnapshot.m_defaultVideoFrameDuration100ns == 900000, "C配置复制到实例快照");
    report.check(core.snapshot().m_ratePermille == 1500 && core.snapshot().m_mediaInfo.m_defaultVideoFrameDuration100ns == 700000,
        "C++实例配置不受另一个C实例影响");
    options.m_defaultRatePermille = 3000;
    cOptions.m_defaultRatePermille = 3000;
    report.check(core.snapshot().m_ratePermille == 1500, "初始化后修改调用方配置不改变运行配置");
    if (!path.empty())
    {
        report.check(core.openMedia(path) == LumaPlayerCoreResultSuccess, "自定义配置C++打开媒体");
        report.check(lumaPlayerCoreOpenMedia(handle, path.c_str()) == LumaPlayerCoreCResultSuccess, "自定义配置C打开媒体");
        report.check(core.snapshot().m_ratePermille == 1500, "打开媒体采用复制后的默认倍率");
        report.check(lumaPlayerCoreGetSnapshot(handle, &cSnapshot) != 0 && cSnapshot.m_ratePermille == 2000,
            "C打开媒体采用自身默认倍率");
        report.check(core.setPlaybackRatePermille(INT_MIN) == LumaPlayerCoreResultSuccess && core.snapshot().m_ratePermille == 50,
            "最低倍率可低于旧的100限制");
        report.check(core.setPlaybackRatePermille(INT_MAX) == LumaPlayerCoreResultSuccess && core.snapshot().m_ratePermille == 4000,
            "最高倍率可高于旧的3000限制");
        report.check(lumaPlayerCoreSetPlaybackRatePermille(handle, INT_MAX) == LumaPlayerCoreCResultSuccess &&
            lumaPlayerCoreGetSnapshot(handle, &cSnapshot) != 0 && cSnapshot.m_ratePermille == 5000, "C使用独立倍率上限");
        report.check(core.setPlaybackRatePermille(1500) == LumaPlayerCoreResultSuccess && core.play() == LumaPlayerCoreResultSuccess,
            "非默认初始倍率可实际播放");
        LumaPlayerTestHelper::sleepMs(100);
        report.check(core.pause() == LumaPlayerCoreResultSuccess && core.snapshot().m_position100ns > 0, "自定义倍率时钟实际前进");
        report.check(core.openMedia(path) == LumaPlayerCoreResultSuccess && core.snapshot().m_ratePermille == 1500,
            "重新打开媒体恢复实例默认倍率");
    }
    core.uninit();
    for (std::map<LumaPlayerCoreResult, std::string>::const_iterator it = kLumaPlayerCoreResultDescriptions.begin(); it != kLumaPlayerCoreResultDescriptions.end(); ++it)
    {
        report.check(it->second == lumaPlayerCoreResultDescription(static_cast<int32_t>(it->first)), "C和C++错误中文描述一致");
    }
    lumaPlayerCoreDestroy(handle);
    report.endCase();
}

/** 检查A向左一帧的真实首帧、保持时间及循环接缝
@param [in,out] report 测试报告
@param [in] path 真实媒体或红蓝切镜素材
@param [in] sceneFixture true表示使用25fps红蓝素材，可断言颜色
@param [in] budget 缓存预算，极小预算强制走前缀与预热读取器路径
*/
static void RunLoopStartCase(LumaPlayerTestReport& report, const std::string& path, bool sceneFixture, int64_t budget)
{
    report.beginCase(budget < 1000 ? "LoopStartPrefixBuffer" : "LoopStartCompleteBuffer");
    if (path.empty())
    {
        report.info("没有媒体，跳过实际帧调度检查");
        report.endCase();
        return;
    }
    LumaPlayerCore core;
    LumaPlayerCoreConfig config;
    config.m_loopBufferMaxBytes = budget;
    LumaPlayerTestAudioRender audio;
    LumaPlayerTestVideoRender video;
    core.setAudioRender(&audio);
    core.setVideoRender(&video);
    report.check(core.init(config) == LumaPlayerCoreResultSuccess, "初始化缓存测试");
    LumaPlayerCoreResult result = core.openMedia(path);
    report.check(result == LumaPlayerCoreResultSuccess, "打开缓存测试素材");
    if (result != LumaPlayerCoreResultSuccess || !core.mediaInfo().m_hasVideo)
    {
        core.uninit();
        report.endCase();
        return;
    }
    report.check(core.setLoopAAtPosition(10000000) == LumaPlayerCoreResultSuccess, "切镜之后设置A");
    int64_t originalA = core.snapshot().m_loopRange.start100ns();
    report.check(core.setLoopBAtPosition(11600000) == LumaPlayerCoreResultSuccess, "设置B保留完整末帧");
    report.check(core.moveLoopPoint(LumaPlayerCoreLoopPointA, -1) == LumaPlayerCoreResultSuccess, "A向左移动一个实际展示帧");
    LumaPlayerLoopRange range = core.snapshot().m_loopRange;
    report.check(range.start100ns() < originalA, "A确实移动到了前一帧");
    if (sceneFixture)
    {
        report.check(range.start100ns() == 9600000, "红蓝素材A准确落到0.960秒红色末帧");
    }
    LumaPlayerTestHelper::sleepMs(300);
    video.clearFrameHistory();
    report.check(core.seekTo(range.start100ns(), false) == LumaPlayerCoreResultSuccess, "定位到A准备起播");
    int32_t flushBefore = audio.stats().m_flushCount;
    report.check(core.play() == LumaPlayerCoreResultSuccess, "开始检查连续循环");
    LumaPlayerTestHelper::sleepMs(2600);
    core.pause();
    std::vector<int64_t> frames = video.frameHistory();
    std::vector<int64_t> walls = video.frameWallTimes();
    std::vector<uint32_t> colors = video.frameColors();
    int32_t cycles = 0;
    int64_t minAHoldMs = 100000;
    int64_t maxSeamMs = 0;
    bool exactStart = true;
    bool bounded = true;
    bool sawRedAtEveryA = true;
    for (size_t i = 0; i < frames.size(); ++i)
    {
        bounded = bounded && frames[i] >= range.start100ns() && frames[i] < range.end100ns();
        if (i > 0 && frames[i] < frames[i - 1])
        {
            ++cycles;
            exactStart = exactStart && frames[i] == range.start100ns();
            int64_t seam = walls[i] - walls[i - 1];
            if (seam > maxSeamMs)
            {
                maxSeamMs = seam;
            }
        }
        if (frames[i] == range.start100ns())
        {
            if (sceneFixture)
            {
                sawRedAtEveryA = sawRedAtEveryA && ((colors[i] >> 16) & 255) > 200 && (colors[i] & 255) < 30;
            }
            if (i + 1 < frames.size() && frames[i + 1] > frames[i])
            {
                int64_t held = walls[i + 1] - walls[i];
                if (held < minAHoldMs)
                {
                    minAHoldMs = held;
                }
            }
        }
    }
    std::ostringstream timing;
    timing << "cycles=" << cycles << ", minAHoldMs=" << minAHoldMs << ", maxBToAMs=" << maxSeamMs;
    report.info(timing.str());
    report.check(cycles >= 6, "完成至少6轮循环");
    report.check(exactStart, "每轮第一帧都是设置的A，不跳过切镜前画面");
    report.check(bounded, "没有提交A之前或B排他边界之后的画面");
    report.check(minAHoldMs >= (range.m_aPoint.m_frameEnd100ns - range.start100ns()) / 10000 - 12, "A帧保留其真实展示时长，不瞬间追赶后续帧");
    if (sceneFixture)
    {
        report.check(sawRedAtEveryA, "每轮A帧确实为切镜前的红色画面");
        report.check(maxSeamMs < 100, "B到A间隔小于100毫秒，不再产生数百毫秒停顿");
        report.check(audio.stats().m_flushCount == flushBefore, "正常循环不会重置音频设备");
    }
    if (sceneFixture && budget > 1000)
    {
        report.check(core.setLoopBAtPosition(range.start100ns()) == LumaPlayerCoreResultSuccess, "缩成A和B同一帧");
        LumaPlayerTestHelper::sleepMs(200);
        video.clearFrameHistory();
        core.seekTo(range.start100ns(), false);
        int32_t flushOneFrame = audio.stats().m_flushCount;
        size_t pcmBefore = audio.stats().m_pcmBytes;
        core.play();
        LumaPlayerTestHelper::sleepMs(4300);
        core.pause();
        std::vector<int64_t> oneFrameHistory = video.frameHistory();
        bool onlyA = !oneFrameHistory.empty();
        for (size_t i = 0; i < oneFrameHistory.size(); ++i)
        {
            onlyA = onlyA && oneFrameHistory[i] == range.start100ns();
        }
        report.check(oneFrameHistory.size() >= 100 && onlyA, "一帧循环100次均保持A画面");
        report.check(audio.stats().m_pcmBytes > pcmBefore, "一帧循环持续提交音频");
        report.check(audio.stats().m_flushCount == flushOneFrame, "一帧循环不重置声卡");
    }
    core.uninit();
    report.endCase();
}

/** 检查高频预览只保留最终目标，松手恢复播放，退出收敛
@param [in,out] report 测试报告
@param [in] path 真实媒体路径
*/
static void RunDragPreviewCase(LumaPlayerTestReport& report, const std::string& path)
{
	report.beginCase("DragPreviewLatestAndExit");
	if (path.empty())
	{
		report.info("未提供媒体，跳过拖动检查");
		report.endCase();
		return;
	}
	LumaPlayerCore core;
	LumaPlayerTestAudioRender audio;
	LumaPlayerTestVideoRender video;
	core.setAudioRender(&audio);
	core.setVideoRender(&video);
	report.check(core.init() == LumaPlayerCoreResultSuccess, "初始化预览测试");
	if (core.openMedia(path) != LumaPlayerCoreResultSuccess)
	{
		report.check(false, "打开预览素材");
		core.uninit();
		report.endCase();
		return;
	}
	int64_t duration = core.mediaInfo().m_duration100ns;
	int64_t target = duration / 3;
	int64_t maxSubmitMs = 0;
	bool accepted = true;
	for (int32_t drag = 0; drag < 20; ++drag)
	{
		core.play();
		core.pauseAsync();
		for (int32_t index = 0; index < 500; ++index)
		{
			int64_t before = LumaPlayerTestHelper::nowMs();
			accepted = core.previewFrameAsync((index * 37LL % 1000) * duration / 1000) == LumaPlayerCoreResultSuccess && accepted;
			int64_t elapsed = LumaPlayerTestHelper::nowMs() - before;
			if (elapsed > maxSubmitMs)
			{
				maxSubmitMs = elapsed;
			}
			if (index % 20 == 0)
			{
				LumaPlayerTestHelper::sleepMs(1);
			}
		}
		core.previewFrameAsync(target);
		int64_t deadline = LumaPlayerTestHelper::nowMs() + 3000;
		while (LumaPlayerTestHelper::nowMs() < deadline)
		{
			LumaPlayerTestVideoStats stats = video.stats();
			if (stats.m_lastTimestamp100ns <= target && stats.m_lastEndTime100ns > target)
			{
				break;
			}
			LumaPlayerTestHelper::sleepMs(5);
		}
		LumaPlayerTestHelper::sleepMs(30);
		LumaPlayerTestVideoStats stats = video.stats();
		report.check(stats.m_lastTimestamp100ns <= target && stats.m_lastEndTime100ns > target, "停止拖动后显示最后请求所在帧，无旧请求覆盖");
		core.seekToAsync(target, true);
		deadline = LumaPlayerTestHelper::nowMs() + 3000;
		while (LumaPlayerTestHelper::nowMs() < deadline && core.snapshot().m_state != LumaPlayerCoreStatePlaying)
		{
			LumaPlayerTestHelper::sleepMs(5);
		}
		report.check(core.snapshot().m_state == LumaPlayerCoreStatePlaying, "提交最终位置后恢复播放");
	}
	report.check(accepted, "一万次预览请求均被接受");
	std::ostringstream timing;
	timing << "previewRequests=10000, maxSubmitMs=" << maxSubmitMs;
	report.info(timing.str());
	report.check(maxSubmitMs < 100, "提交预览不等待解码或遍历历史队列");
	core.pause();
	core.previewFrameAsync(duration / 2);
	core.closeMedia();
	report.check(core.openMedia(path) == LumaPlayerCoreResultSuccess, "预览未完成时换文件成功");
	for (int32_t i = 0; i < 500; ++i)
	{
		core.previewFrameAsync((i * 71LL % 1000) * duration / 1000);
	}
	int64_t shutdownBegin = LumaPlayerTestHelper::nowMs();
	core.uninit();
	report.check(LumaPlayerTestHelper::nowMs() - shutdownBegin < 3000, "预览期间退出可及时收敛");
	int32_t renderedAfterExit = video.stats().m_frameCount;
	LumaPlayerTestHelper::sleepMs(50);
	report.check(video.stats().m_frameCount == renderedAfterExit, "退出返回后没有旧渲染回调");
	report.check(core.init() == LumaPlayerCoreResultSuccess, "退出后可重新初始化预览任务");
	core.uninit();
	report.endCase();
}

/** 验证连续AB移动、取消后的稳定性以及切换控制命令
@param [in,out] report 测试报告
@param [in] path 测试素材路径
*/
static void RunLoopMoveTaskCase(LumaPlayerTestReport& report, const std::string& path)
{
    report.beginCase("AB逐帧任务取消和连续移动");
    if (path.empty())
    {
        report.check(false, "需要真实视频素材");
        report.endCase();
        return;
    }
    LumaPlayerTestVideoRender video;
    LumaPlayerTestAudioRender audio;
    LumaPlayerCore core;
    core.setVideoRender(&video);
    core.setAudioRender(&audio);
    report.check(core.init() == LumaPlayerCoreResultSuccess, "初始化");
    if (core.openMedia(path) != LumaPlayerCoreResultSuccess)
    {
        report.check(false, "打开素材");
        core.uninit();
        report.endCase();
        return;
    }
    core.pause();
    core.setLoopAAtPosition(0);
    core.setLoopBAtPosition(core.mediaInfo().m_duration100ns / 4);
    bool forward = true;
    int64_t beforeTime = LumaPlayerTestHelper::nowMs();
    for (int32_t i = 0; i < 20; ++i)
    {
        LumaPlayerSnapshot before = core.snapshot();
        LumaPlayerCoreResult result = core.moveLoopPoint(LumaPlayerCoreLoopPointB, 1);
        LumaPlayerSnapshot after = core.snapshot();
        forward = forward && result == LumaPlayerCoreResultSuccess &&
            after.m_loopRange.m_bPoint.m_frameStart100ns > before.m_loopRange.m_bPoint.m_frameStart100ns &&
            after.m_currentVideoFrame.m_frameStart100ns == after.m_loopRange.m_bPoint.m_frameStart100ns &&
            after.m_state == LumaPlayerCoreStatePaused && after.m_position100ns == after.m_currentVideoFrame.m_frameStart100ns;
    }
    report.check(forward, "连续20次右移均前进且画面与B帧一致");
    std::ostringstream timing;
    timing << "20-step-ms=" << LumaPlayerTestHelper::nowMs() - beforeTime;
    report.info(timing.str());
    bool backward = true;
    for (int32_t i = 0; i < 20; ++i)
    {
        int64_t before = core.snapshot().m_loopRange.m_bPoint.m_frameStart100ns;
        backward = core.moveLoopPoint(LumaPlayerCoreLoopPointB, -1) == LumaPlayerCoreResultSuccess && backward;
        backward = core.snapshot().m_loopRange.m_bPoint.m_frameStart100ns < before && backward;
        backward = backward && core.snapshot().m_position100ns == core.snapshot().m_currentVideoFrame.m_frameStart100ns;
    }
    report.check(backward, "连续20次左移包含缓存耗尽后的定位");
    for (int32_t i = 0; i < 2000; ++i)
    {
        core.moveLoopPointAsync(LumaPlayerCoreLoopPointB, 1);
    }
    core.cancelLoopPointMove();
    // 同线程暂停命令作为屏障，确保在途移动已经退出
    report.check(core.pause() == LumaPlayerCoreResultSuccess, "取消后控制线程及时响应");
    LumaPlayerSnapshot stopped = core.snapshot();
    report.check(stopped.m_position100ns == stopped.m_currentVideoFrame.m_frameStart100ns, "取消不会留下仅进度变化而未显示对应帧的状态");
    int32_t frames = video.stats().m_frameCount;
    LumaPlayerTestHelper::sleepMs(1200);
    report.check(core.snapshot().m_loopRange.m_bPoint.m_frameStart100ns == stopped.m_loopRange.m_bPoint.m_frameStart100ns && video.stats().m_frameCount == frames,
        "取消后1.2秒无历史端点移动和旧画面提交");
    bool editing = true;
    for (int32_t i = 0; i < 4; ++i)
    {
        core.play();
        LumaPlayerTestHelper::sleepMs(100);
        LumaPlayerCoreLoopPointType point = i % 2 == 0 ? LumaPlayerCoreLoopPointA : LumaPlayerCoreLoopPointB;
        editing = core.moveLoopPoint(point, i < 2 ? 1 : -1) == LumaPlayerCoreResultSuccess && editing;
        LumaPlayerSnapshot moved = core.snapshot();
        int64_t selectedStart = point == LumaPlayerCoreLoopPointA ? moved.m_loopRange.m_aPoint.m_frameStart100ns : moved.m_loopRange.m_bPoint.m_frameStart100ns;
        editing = editing && moved.m_state == LumaPlayerCoreStatePaused && moved.m_position100ns == selectedStart &&
            moved.m_currentVideoFrame.m_frameStart100ns == selectedStart;
        int32_t pausedFrames = video.stats().m_frameCount;
        LumaPlayerTestHelper::sleepMs(150);
        editing = editing && video.stats().m_frameCount == pausedFrames && core.snapshot().m_position100ns == selectedStart;
        editing = core.play() == LumaPlayerCoreResultSuccess && editing;
        LumaPlayerTestHelper::sleepMs(200);
        editing = editing && video.stats().m_frameCount > pausedFrames;
    }
    report.check(editing, "播放中A/B左右移动均暂停，画面/进度一致，重新播放可恢复");
    core.pause();
    core.moveLoopPointAsync(LumaPlayerCoreLoopPointB, 1);
    core.cancelLoopPointMove();
    core.pause();
    int32_t pausedFrames = video.stats().m_frameCount;
    LumaPlayerTestHelper::sleepMs(300);
    report.check(video.stats().m_frameCount == pausedFrames, "取消端点移动保持暂停");
    core.moveLoopPointAsync(LumaPlayerCoreLoopPointB, -1);
    core.uninit();
    report.check(core.init() == LumaPlayerCoreResultSuccess, "在途端点任务退出后可重新初始化");
    core.uninit();
    report.endCase();
}

LumaPlayerTestReentryRender::LumaPlayerTestReentryRender(LumaPlayerCore* core) :
m_core(core),
m_result(-1)
{

}

bool LumaPlayerTestReentryRender::renderVideo(const LumaPlayerVideoFrame& frame)
{
	if (m_core != nullptr)
	{
		m_result.store(m_core->pause());
	}
	return LumaPlayerTestVideoRender::renderVideo(frame);
}

int32_t LumaPlayerTestReentryRender::result() const
{
	return m_result.load();
}

/** 验证整数边界、实际PCM倍速长度和源缓存不变
@param [in,out] report 测试报告
*/
static void RunFrameBoundaryAndRateCase(LumaPlayerTestReport& report)
{
	report.beginCase("FrameBoundaryAndAudioRate");
    MediaClock clock;
    clock.start(INT64_MAX - 1, 3000);
    LumaPlayerTestHelper::sleepMs(1);
    report.check(clock.position100ns() == INT64_MAX, "媒体时钟极值饱和，不回绕为负数");
    clock.pause();
	LumaPlayerAudioFormat format;
	format.m_sampleRate = 48000;
	format.m_channels = INT_MAX;
	format.m_bitsPerSample = 32;
	report.check(!format.isValid() && format.bytesPerFrame() == 0, "格式乘法溢出被拒绝");
	LumaPlayerVideoFrame video;
	video.m_width = 8;
	video.m_height = 8;
	video.m_stride = 32;
	video.m_bgraPixels.resize(4);
	report.check(video.empty(), "不足一帧的像素缓冲被拒绝");
	video.m_timestamp100ns = INT64_MAX - 1;
	video.m_duration100ns = 10;
	report.check(video.endTime100ns() == INT64_MAX, "帧结束时间不会有符号溢出");
	LumaPlayerAudioFrame source;
	source.m_format.m_sampleRate = 48000;
	source.m_format.m_channels = 2;
	source.m_format.m_bitsPerSample = 16;
	source.m_timestamp100ns = 10000000;
	source.m_duration100ns = 1000000;
	source.m_pcmData.resize(4800 * 4, 0);
	for (size_t i = 0; i < 4800; ++i)
	{
		const int16_t left = 1234;
		const int16_t right = -4321;
		std::memcpy(&source.m_pcmData[i * 4], &left, 2);
		std::memcpy(&source.m_pcmData[i * 4 + 2], &right, 2);
	}
	const std::vector<unsigned char> original = source.m_pcmData;
	const int32_t rates[] = {50, 100, 500, 1000, 1500, 2000, 3000, 4000, 5000};
	bool correct = true;
	for (size_t i = 0; i < sizeof(rates) / sizeof(rates[0]); ++i)
	{
		LumaPlayerAudioFrame output;
		correct = correct && PlayerEngineHelper::scaleAudioRate(source, rates[i], &output);
		correct = correct && output.m_pcmData.size() == static_cast<size_t>(4800 * 1000 / rates[i] * 4);
		correct = correct && output.m_timestamp100ns == source.m_timestamp100ns && output.m_duration100ns == source.m_duration100ns;
		for (size_t sample = 0; sample + 3 < output.m_pcmData.size(); sample += 4)
		{
			int16_t left = 0;
			int16_t right = 0;
			std::memcpy(&left, &output.m_pcmData[sample], 2);
			std::memcpy(&right, &output.m_pcmData[sample + 2], 2);
			correct = correct && left == 1234 && right == -4321;
		}
	}
	report.check(correct, "0.05至5倍PCM长度正确且声道不串扰，媒体时间不变");
	report.check(source.m_pcmData == original, "倍速不修改循环缓存中的源PCM");
	report.endCase();
}

bool LumaPlayerTestThrowingVideoRender::openVideo(const LumaPlayerVideoFormat& format)
{
    (void)format;
    throw std::runtime_error("test video output failure");
}

/** 验证渲染回调同步重入被拒绝及EOF预览和预算失败
@param [in,out] report 测试报告
@param [in] mediaPath 素材路径
*/
static void RunReentryAndPreviewEndCase(LumaPlayerTestReport& report, const std::string& mediaPath)
{
	report.beginCase("ReentryAndPreviewEnd");
	LumaPlayerCore core;
	LumaPlayerTestReentryRender video(&core);
	core.setVideoRender(&video);
	report.check(core.init() == LumaPlayerCoreResultSuccess, "初始化重入测试");
	report.check(core.setLoopAAtPosition(0) == LumaPlayerCoreResultNotOpen, "未打开媒体不能设置AB");
	if (!mediaPath.empty())
	{
		report.check(core.openMedia(mediaPath) == LumaPlayerCoreResultSuccess, "回调调用同步pause不阻塞open");
		report.check(video.result() == LumaPlayerCoreResultStateError, "控制线程回调拒绝同步重入");
		core.previewFrameAsync(core.mediaInfo().m_duration100ns / 2);
		LumaPlayerTestHelper::sleepMs(150);
		report.check(video.result() == LumaPlayerCoreResultStateError, "预览线程回调拒绝同步重入");
		core.closeMedia();
		const int32_t count = video.stats().m_frameCount;
		LumaPlayerTestHelper::sleepMs(100);
		report.check(video.stats().m_frameCount == count, "close返回后无旧预览发布");
		FFmpegCppPlaybackReader reader;
		FFmpegCppPlaybackOpenOption openOption;
		openOption.decodeAudio = false;
		report.check(reader.open(mediaPath, openOption), "打开末帧预览读取器");
		FFmpegCppPlaybackPreviewOption option;
		option.maxReadFrameCount = 10000;
		FFmpegCppPlaybackVideoFrame frame;
		const bool found = reader.readVideoFrameAtEx(reader.duration100ns(), option, &frame, nullptr);
		std::ostringstream endInfo;
		endInfo << "previewFound=" << found << ", timestamp=" << frame.timestamp100ns << ", frameDuration=" << frame.duration100ns << ", mediaDuration=" << reader.duration100ns() << ", error=" << reader.lastError();
		report.info(endInfo.str());
		report.check(found && frame.timestamp100ns < reader.duration100ns() &&
			frame.timestamp100ns + frame.duration100ns >= reader.duration100ns(), "EOF排空后能够定位末展示帧");
		option.maxReadFrameCount = 1;
		report.check(!reader.readVideoFrameAtEx(reader.duration100ns(), option, &frame, nullptr), "帧预算不足不伪报定位成功");
		reader.close();
        LumaPlayerTestThrowingVideoRender throwingVideo;
        LumaPlayerCore throwingCore;
        throwingCore.setVideoRender(&throwingVideo);
        report.check(throwingCore.init() == LumaPlayerCoreResultSuccess, "初始化异常路径测试");
        report.check(throwingCore.openMedia(mediaPath) == LumaPlayerCoreResultInternalError, "输出异常转换为明确错误");
        report.check(throwingCore.snapshot().m_state == LumaPlayerCoreStateError, "输出异常不遗留Opening状态");
        throwingCore.uninit();
	}
	core.uninit();
	report.endCase();
}

int main(int argc, char* argv[])
{
	LumaPlayerTestReport report;
	const std::string mediaPath = argc > 1 && argv[1] != nullptr ? argv[1] : std::string();
	bool sceneFixture = argc > 2 && std::string(argv[2]) == "scene";
	RunFrameBoundaryAndRateCase(report);
	RunReentryAndPreviewEndCase(report, mediaPath);
	RunLoopStartCase(report, mediaPath, sceneFixture, 64 * 1024 * 1024);
	RunLoopStartCase(report, mediaPath, sceneFixture, 1);
	RunDragPreviewCase(report, mediaPath);
	RunLoopMoveTaskCase(report, mediaPath);
	RunConfigurationCase(report, mediaPath);
	runCppLifecycleCase(report);
	runCppRepeatedInitCase(report);
	runCApiLifecycleCase(report);
	runCApiMediaCase(report, mediaPath);
	runMediaCase(report, mediaPath);
	report.writeReport("LumaPlayerTestReport.txt");
	return report.failedCount() == 0 ? 0 : 1;
}