#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LegacyTests.h"
#include "TestCaseRegistry.h"
#include "FFmpegCpp/FFmpegCppAPI.h"
#include "LumaPlayerCore/PlayerEngineHelper.h"
#include "LumaPlayerCore/MediaClock.h"
#include "../../LumaPlayerCore/LumaPlayerCore/src/KaraokeTimeline.h"
#include "../../LumaPlayerCore/LumaPlayerCore/src/KaraokePcmTimeline.h"
#include "../../LumaPlayerCore/LumaPlayerCore/src/KaraokePcmProcessor.h"
#include "CSystem/CSystemAPI.h"
#include <QString>
#include <QCoreApplication>

#include <climits>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <stdexcept>
#include <memory>

int64_t LumaPlayerTestHelper::nowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

const char* LumaPlayerTestHelper::resultName(LumaPlayerCoreResult result)
{
    switch (result)
    {
    case LumaPlayerCoreResultSuccess:
    {
        return "Success";
    }
    case LumaPlayerCoreResultInvalidParam:
    {
        return "InvalidParam";
    }
    case LumaPlayerCoreResultNotInit:
    {
        return "NotInit";
    }
    case LumaPlayerCoreResultNotOpen:
    {
        return "NotOpen";
    }
    case LumaPlayerCoreResultOpenFailed:
    {
        return "OpenFailed";
    }
    case LumaPlayerCoreResultStateError:
    {
        return "StateError";
    }
    case LumaPlayerCoreResultCanceled:
    {
        return "Canceled";
    }
    case LumaPlayerCoreResultTimeout:
    {
        return "Timeout";
    }
    case LumaPlayerCoreResultInternalError:
    {
        return "InternalError";
    }
    case LumaPlayerCoreResultEnd:
    {
        return "End";
    }
    default:
    {
        return "Unknown";
    }
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

LumaPlayerTestCRenderState::LumaPlayerTestCRenderState() :
m_openAudioCount(0),
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

LumaPlayerTestCaseRecord::LumaPlayerTestCaseRecord() :
m_passed(true),
m_costMs(0)
{

}

LumaPlayerTestReport::LumaPlayerTestReport() :
m_mutex(new std::mutex),
m_caseStarted(false),
m_caseBeginMs(0)
{

}

void LumaPlayerTestReport::beginCase(const std::string& caseName)
{
    std::cout << "BEGIN " << caseName << std::endl;
    std::lock_guard<std::mutex> lock(*m_mutex);
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
    std::lock_guard<std::mutex> lock(*m_mutex);
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
    std::lock_guard<std::mutex> lock(*m_mutex);
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
    std::lock_guard<std::mutex> lock(*m_mutex);
    if (m_caseStarted)
    {
        m_currentCase.m_messages.push_back(std::string("[INFO] ") + message);
    }
}

int32_t LumaPlayerTestReport::failedCount() const
{
    std::lock_guard<std::mutex> lock(*m_mutex);
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

bool LumaPlayerTestReport::writeReport(const std::string& filePath) const
{
    std::ostringstream stream;
    {
        std::lock_guard<std::mutex> lock(*m_mutex);
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

    const std::string text = stream.str();
    std::cout << text;
    FILE* file = CSystem::openBinaryOutputFile(QString::fromUtf8(filePath.c_str()).toStdWString());
    if (file == nullptr)
    {
        return false;
    }
    const bool written = CSystem::writeBinaryOutputFile(file, text.data(), text.size());
    const bool closed = CSystem::closeBinaryOutputFile(file);
    return written && closed;
}

LumaPlayerTestAudioRender::LumaPlayerTestAudioRender() :
m_mutex(new std::mutex),
m_openCount(0),
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
    std::lock_guard<std::mutex> lock(*m_mutex);
    ++m_openCount;
    return format.isValid();
}

void LumaPlayerTestAudioRender::closeAudio()
{
    std::lock_guard<std::mutex> lock(*m_mutex);
    ++m_closeCount;
}

void LumaPlayerTestAudioRender::flushAudio()
{
    std::lock_guard<std::mutex> lock(*m_mutex);
    ++m_flushCount;
}

void LumaPlayerTestAudioRender::pauseAudio()
{
    std::lock_guard<std::mutex> lock(*m_mutex);
    ++m_pauseCount;
}

void LumaPlayerTestAudioRender::resumeAudio()
{
    std::lock_guard<std::mutex> lock(*m_mutex);
    ++m_resumeCount;
}

bool LumaPlayerTestAudioRender::renderAudio(const LumaPlayerAudioFrame& frame)
{
    std::lock_guard<std::mutex> lock(*m_mutex);
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
    std::lock_guard<std::mutex> lock(*m_mutex);
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

LumaPlayerTestVideoRender::LumaPlayerTestVideoRender() :
m_mutex(new std::mutex),
m_openCount(0),
m_closeCount(0),
m_frameCount(0),
m_lastTimestamp100ns(0),
m_lastEndTime100ns(0),
m_lastPixelBytes(0)
{

}

bool LumaPlayerTestVideoRender::openVideo(const LumaPlayerVideoFormat& format)
{
    std::lock_guard<std::mutex> lock(*m_mutex);
    ++m_openCount;
    return format.isValid();
}

void LumaPlayerTestVideoRender::closeVideo()
{
    std::lock_guard<std::mutex> lock(*m_mutex);
    ++m_closeCount;
}

bool LumaPlayerTestVideoRender::renderVideo(const LumaPlayerVideoFrame& frame)
{
    std::lock_guard<std::mutex> lock(*m_mutex);
    ++m_frameCount;
    m_lastTimestamp100ns = frame.m_timestamp100ns;
    m_lastEndTime100ns = frame.endTime100ns();
    m_lastPixelBytes = frame.m_bgraPixels.size();
    try
    {
        m_frameHistory.push_back(frame.m_timestamp100ns);
        m_frameWallTimes.push_back(LumaPlayerTestHelper::nowMs());
        uint32_t color = frame.m_bgraPixels.size() >= 3 ? static_cast<uint32_t>(frame.m_bgraPixels[0]) |
                (static_cast<uint32_t>(frame.m_bgraPixels[1]) << 8) | (static_cast<uint32_t>(frame.m_bgraPixels[2]) << 16)
                                                        : 0;
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
    std::lock_guard<std::mutex> lock(*m_mutex);
    m_frameHistory.clear();
    m_frameWallTimes.clear();
    m_frameColors.clear();
}

std::vector<int64_t> LumaPlayerTestVideoRender::frameHistory() const
{
    std::lock_guard<std::mutex> lock(*m_mutex);
    return m_frameHistory;
}

std::vector<int64_t> LumaPlayerTestVideoRender::frameWallTimes() const
{
    std::lock_guard<std::mutex> lock(*m_mutex);
    return m_frameWallTimes;
}

std::vector<uint32_t> LumaPlayerTestVideoRender::frameColors() const
{
    std::lock_guard<std::mutex> lock(*m_mutex);
    return m_frameColors;
}

bool LumaPlayerTestVideoRender::lastFrameRange(int64_t* frameStart100ns, int64_t* frameEnd100ns) const
{
    if (frameStart100ns == nullptr || frameEnd100ns == nullptr)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(*m_mutex);
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
    std::lock_guard<std::mutex> lock(*m_mutex);
    LumaPlayerTestVideoStats result = {};
    result.m_openCount = m_openCount;
    result.m_closeCount = m_closeCount;
    result.m_frameCount = m_frameCount;
    result.m_lastTimestamp100ns = m_lastTimestamp100ns;
    result.m_lastEndTime100ns = m_lastEndTime100ns;
    result.m_lastPixelBytes = m_lastPixelBytes;
    return result;
}

/** 检查C音频格式并累计打开次数
@param [in] format 借用的音视频格式，仅调用期间有效
@param [in] userData 借用的回调上下文，在回调可发生期间保持有效
@return 非零表示格式可接受，零表示拒绝
*/
static int32_t COpenAudioCallback(const LumaPlayerCoreCAudioFormat* format, void* userData)
{
    LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
    if (state != nullptr)
    {
        ++state->m_openAudioCount;
    }
    return format != nullptr && format->m_sampleRate > 0 ? 1 : 0;
}

/** 累计C音频关闭次数
@param [in] userData 借用的回调上下文，在回调可发生期间保持有效
*/
static void CCloseAudioCallback(void* userData)
{
    LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
    if (state != nullptr)
    {
        ++state->m_closeAudioCount;
    }
}

/** 累计C音频清空次数
@param [in] userData 借用的回调上下文，在回调可发生期间保持有效
*/
static void CFlushAudioCallback(void* userData)
{
    LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
    if (state != nullptr)
    {
        ++state->m_flushAudioCount;
    }
}

/** 累计C音频暂停次数
@param [in] userData 借用的回调上下文，在回调可发生期间保持有效
*/
static void CPauseAudioCallback(void* userData)
{
    LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
    if (state != nullptr)
    {
        ++state->m_pauseAudioCount;
    }
}

/** 累计C音频恢复次数
@param [in] userData 借用的回调上下文，在回调可发生期间保持有效
*/
static void CResumeAudioCallback(void* userData)
{
    LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
    if (state != nullptr)
    {
        ++state->m_resumeAudioCount;
    }
}

/** 累计C音频帧回调次数，不播放声音
@param [in] frame 借用的音视频帧，仅渲染调用期间有效
@param [in] userData 借用的回调上下文，在回调可发生期间保持有效
@return 非零表示收到有效PCM，零表示拒绝
*/
static int32_t CRenderAudioCallback(const LumaPlayerCoreCAudioFrame* frame, void* userData)
{
    LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
    if (state != nullptr)
    {
        ++state->m_audioFrameCount;
    }
    return frame != nullptr && frame->m_pcmData != nullptr && frame->m_pcmSize > 0 ? 1 : 0;
}

/** 为无设备渲染桩返回零缓冲时长
@param [in] userData 借用的回调上下文，在回调可发生期间保持有效
@return 零，测试桩不提供设备队列
*/
static int64_t CBufferedAudioCallback(void* userData)
{
    LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
    if (state != nullptr)
    {
        return 0;
    }
    return 0;
}

/** 检查C视频格式并累计打开次数
@param [in] format 借用的音视频格式，仅调用期间有效
@param [in] userData 借用的回调上下文，在回调可发生期间保持有效
@return 非零表示格式可接受，零表示拒绝
*/
static int32_t COpenVideoCallback(const LumaPlayerCoreCVideoFormat* format, void* userData)
{
    LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
    if (state != nullptr)
    {
        ++state->m_openVideoCount;
    }
    return format != nullptr && format->m_width > 0 && format->m_height > 0 ? 1 : 0;
}

/** 累计C视频关闭次数
@param [in] userData 借用的回调上下文，在回调可发生期间保持有效
*/
static void CCloseVideoCallback(void* userData)
{
    LumaPlayerTestCRenderState* state = static_cast<LumaPlayerTestCRenderState*>(userData);
    if (state != nullptr)
    {
        ++state->m_closeVideoCount;
    }
}

/** 记录C视频帧区间并累计渲染次数
@param [in] frame 借用的音视频帧，仅渲染调用期间有效
@param [in] userData 借用的回调上下文，在回调可发生期间保持有效
@return 非零表示收到有效像素，零表示拒绝
*/
static int32_t CRenderVideoCallback(const LumaPlayerCoreCVideoFrame* frame, void* userData)
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

/** 检查C++生命周期的正向及反向状态边界
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
*/
static void RunCppLifecycleCase(LumaPlayerTestReport& report)
{
    report.beginCase("C++接口生命周期和错误路径");
    LumaPlayerTestAudioRender audioRender;
    LumaPlayerTestVideoRender videoRender;
    LumaPlayerCore core;
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
    report.check(core.snapshot().m_ratePermille == 5000, "高于上限的倍速会裁剪到5.0倍");
    result = core.openMedia("");
    report.check(result != LumaPlayerCoreResultSuccess, "空路径打开失败且不崩溃");
    core.uninit();
    report.check(!core.isInit(), "uninit后Core退出可用状态");
    report.endCase();
}

/** 检查重复初始化及清理的幂等边界
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
*/
static void RunCppRepeatedInitCase(LumaPlayerTestReport& report)
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

/** 检查C接口生命周期及空参数边界
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
*/
static void RunCApiLifecycleCase(LumaPlayerTestReport& report)
{
    report.beginCase("C接口生命周期和错误路径");
    LumaPlayerTestCRenderState renderState;
    LumaPlayerCoreHandle handle = lumaPlayerCoreCreate();
    std::unique_ptr<LumaPlayerCoreCContext, decltype(&lumaPlayerCoreDestroy)> lifetime(handle, &lumaPlayerCoreDestroy);
    report.check(handle != nullptr, "C接口create返回非空句柄");
    if (handle == nullptr)
    {
        report.endCase();
        return;
    }

    LumaPlayerCoreCRenderCallbacks callbacks;
    callbacks.m_openAudio = COpenAudioCallback;
    callbacks.m_closeAudio = CCloseAudioCallback;
    callbacks.m_flushAudio = CFlushAudioCallback;
    callbacks.m_pauseAudio = CPauseAudioCallback;
    callbacks.m_resumeAudio = CResumeAudioCallback;
    callbacks.m_renderAudio = CRenderAudioCallback;
    callbacks.m_bufferedAudio = CBufferedAudioCallback;
    callbacks.m_openVideo = COpenVideoCallback;
    callbacks.m_closeVideo = CCloseVideoCallback;
    callbacks.m_renderVideo = CRenderVideoCallback;
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
    const std::string expected = kLumaPlayerCoreResultDescriptions.at(static_cast<LumaPlayerCoreResult>(result));
    const size_t required = expected.size() + 1;
    char one[] = {'x', 'z'};
    char zero[] = {'x'};
    char truncated[3] = {'x', 'x', 'z'};
    std::vector<char> full(required + 1, 'z');
    report.check(lumaPlayerCoreGetLastError(handle, nullptr, 0) == required &&
            lumaPlayerCoreGetLastError(handle, zero, 0) == required && zero[0] == 'x',
        "查询错误长度不写零容量缓冲");
    report.check(lumaPlayerCoreGetLastError(handle, one, 1) == required && one[0] == '\0' && one[1] == 'z',
        "单字节缓冲只写终止符");
    report.check(lumaPlayerCoreGetLastError(handle, truncated, 2) == required && truncated[0] == expected[0] &&
            truncated[1] == '\0' && truncated[2] == 'z',
        "截断不越界且保留终止符");
    report.check(lumaPlayerCoreGetLastError(handle, &full[0], required) == required &&
            std::string(&full[0]) == expected && full[required] == 'z',
        "完整错误文本与独立错误映射一致");
    report.check(lumaPlayerCoreGetLastError(nullptr, one, 1) == 0 && one[0] == '\0', "空句柄清空有效输出");
    report.check(lumaPlayerCoreOpenMedia(handle, nullptr) == LumaPlayerCoreCResultInvalidParam, "C接口空路径指针返回InvalidParam");
    report.check(lumaPlayerCoreMoveLoopPoint(handle, 2, 1) == LumaPlayerCoreCResultInvalidParam, "C接口拒绝无效循环点类型");
    lumaPlayerCoreUninit(handle);
    lifetime.reset();
    report.endCase();
}

/** 检查C接口媒体播放、跳转及AB循环
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
@param [in] mediaPath 测试自产媒体路径，调用期间有效
*/
static void RunCApiMediaCase(LumaPlayerTestReport& report, const std::string& mediaPath)
{
    report.beginCase("C接口真实媒体和异步控制");
    if (mediaPath.empty())
    {
        report.info("未传入媒体路径，C接口真实解码用例跳过。");
        report.endCase();
        return;
    }

    LumaPlayerTestCRenderState renderState;
    LumaPlayerCoreHandle handle = lumaPlayerCoreCreate();
    std::unique_ptr<LumaPlayerCoreCContext, decltype(&lumaPlayerCoreDestroy)> lifetime(handle, &lumaPlayerCoreDestroy);
    report.check(handle != nullptr, "C接口为真实媒体创建Core句柄");
    if (handle == nullptr)
    {
        report.endCase();
        return;
    }

    LumaPlayerCoreCRenderCallbacks callbacks = {};
    callbacks.m_openAudio = COpenAudioCallback;
    callbacks.m_closeAudio = CCloseAudioCallback;
    callbacks.m_flushAudio = CFlushAudioCallback;
    callbacks.m_pauseAudio = CPauseAudioCallback;
    callbacks.m_resumeAudio = CResumeAudioCallback;
    callbacks.m_renderAudio = CRenderAudioCallback;
    callbacks.m_bufferedAudio = CBufferedAudioCallback;
    callbacks.m_openVideo = COpenVideoCallback;
    callbacks.m_closeVideo = CCloseVideoCallback;
    callbacks.m_renderVideo = CRenderVideoCallback;
    callbacks.m_userData = &renderState;
    lumaPlayerCoreSetRenderCallbacks(handle, &callbacks);

    lumaPlayerCoreSetLogEnabled(handle, 1);

    int32_t result = lumaPlayerCoreInit(handle);
    report.check(result == LumaPlayerCoreCResultSuccess, "C接口真实媒体Core初始化成功");
    if (result != LumaPlayerCoreCResultSuccess)
    {
        lifetime.reset();
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
        char errorBuffer[1024] = {0};
        lumaPlayerCoreGetLastError(handle, errorBuffer, sizeof(errorBuffer));
        report.info(std::string("C API lastError=") + errorBuffer);
        lifetime.reset();
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

    char errorBuffer[16] = {0};
    size_t requiredErrorSize = lumaPlayerCoreGetLastError(handle, errorBuffer, sizeof(errorBuffer));
    report.check(requiredErrorSize >= 1 && errorBuffer[sizeof(errorBuffer) - 1] == '\0', "C接口错误缓冲区契约有效");
    lumaPlayerCoreCloseMedia(handle);
    lumaPlayerCoreUninit(handle);
    lifetime.reset();
    report.endCase();
}

/** 检查C++接口媒体播放、跳转及AB循环
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
@param [in] mediaPath 测试自产媒体路径，调用期间有效
*/
static void RunMediaCase(LumaPlayerTestReport& report, const std::string& mediaPath)
{
    report.beginCase("真实媒体打开、预览、seek和AB循环");
    if (mediaPath.empty())
    {
        report.info("未传入媒体路径，真实解码用例跳过。用法：LumaPlayerTestd.exe test.mp4");
        report.endCase();
        return;
    }

    LumaPlayerTestAudioRender audioRender;
    LumaPlayerTestVideoRender videoRender;
    LumaPlayerCore core;
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
        "预览帧的左闭右开时间区间覆盖目标时间，不会选到下一帧");
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
    result = core.setPlaybackRatePermille(5000);
    report.check(result == LumaPlayerCoreResultSuccess && core.snapshot().m_ratePermille == 5000,
        "暂停状态可以设置到5.0倍且回执倍率正确");
    int64_t resumeBeginMs = LumaPlayerTestHelper::nowMs();
    result = core.play();
    int64_t resumeElapsedMs = LumaPlayerTestHelper::nowMs() - resumeBeginMs;
    report.check(result == LumaPlayerCoreResultSuccess, LumaPlayerTestHelper::resultMessage("pause-rate play", result));
    report.check(resumeElapsedMs < 500, "暂停调速后恢复播放不触发长时间重定位");
    core.pause();
    if (snapshot.m_mediaInfo.m_hasAudio)
    {
        report.check(audioRender.stats().m_openCount > 0, "打开媒体时音频渲染器收到openAudio");
    }
    core.closeMedia();
    core.uninit();
    report.endCase();
}

/** 检查显式配置、默认值及非法边界
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
@param [in] path 测试自产素材路径，调用期间有效
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
    std::unique_ptr<LumaPlayerCoreCContext, decltype(&lumaPlayerCoreDestroy)> lifetime(handle, &lumaPlayerCoreDestroy);
    report.check(handle != nullptr, "创建C句柄");
    report.check(lumaPlayerCoreInitWithConfig(handle, nullptr) == LumaPlayerCoreCResultInvalidParam, "C初始化接口检查空配置");
    cOptions.m_controlCommandTimeoutMs = 0;
    report.check(lumaPlayerCoreInitWithConfig(handle, &cOptions) == LumaPlayerCoreCResultInvalidParam, "C和C++保持一致的参数校验");
    cOptions.m_controlCommandTimeoutMs = 1000;
    report.check(lumaPlayerCoreInitWithConfig(handle, &cOptions) == LumaPlayerCoreCResultSuccess, "C接口接受外部配置");
    lumaPlayerCoreUninit(handle);
    LumaPlayerCoreConfig defaults;
    report.check(cOptions.m_defaultFrameDuration100ns == defaults.m_defaultFrameDuration100ns &&
            cOptions.m_defaultRatePermille == 1000 && cOptions.m_minRatePermille == 100 && cOptions.m_maxRatePermille == 5000,
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
    cOptions.m_defaultRatePermille = 6000;
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
            cSnapshot.m_defaultVideoFrameDuration100ns == 900000,
        "C配置复制到实例快照");
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
                lumaPlayerCoreGetSnapshot(handle, &cSnapshot) != 0 && cSnapshot.m_ratePermille == 5000,
            "C使用独立倍率上限");
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
    lifetime.reset();
    if (!path.empty())
    {
        LumaPlayerTestVideoRender video;
        LumaPlayerCore extreme;
        LumaPlayerCoreConfig bounds;
        bounds.m_maxRatePermille = INT_MAX;
        bounds.m_audioLead100ns = INT64_MAX;
        extreme.setVideoRender(&video);
        const bool opened = extreme.init(bounds) == LumaPlayerCoreResultSuccess &&
            extreme.openMedia(path) == LumaPlayerCoreResultSuccess;
        report.check(opened && extreme.mediaInfo().m_hasAudio, "打开带音频的极值配置素材");
        if (opened && extreme.mediaInfo().m_hasAudio)
        {
            const LumaPlayerSnapshot before = extreme.snapshot();
            report.check(extreme.setPlaybackRatePermille(INT_MAX) == LumaPlayerCoreResultAudioRenderFailed &&
                    extreme.snapshot().m_ratePermille == before.m_ratePermille &&
                    extreme.snapshot().m_state == before.m_state,
                "音频拒绝倍率后不改变原时钟倍率和暂停状态");
            const int32_t frames = video.stats().m_frameCount;
            report.check(extreme.setPlaybackRatePermille(5000) == LumaPlayerCoreResultSuccess &&
                    extreme.play() == LumaPlayerCoreResultSuccess,
                "超大预写时长仍可正常起播");
            const int64_t deadline = LumaPlayerTestHelper::nowMs() + 1000;
            while (video.stats().m_frameCount == frames && LumaPlayerTestHelper::nowMs() < deadline)
            {
                LumaPlayerTestHelper::sleepMs(5);
            }
            report.check(video.stats().m_frameCount > frames, "极值配置未阻止真实视频帧输出");
        }
        extreme.uninit();
    }
    report.endCase();
}

/** 验证AB循环首帧和播放连续性
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
@param [in] path 测试自产素材路径，调用期间有效
@param [in] sceneFixture 是否使用确定性的非零场景素材
@param [in] budget 用例延迟预算，毫秒
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
    LumaPlayerCoreConfig config;
    config.m_loopBufferMaxBytes = budget;
    LumaPlayerTestAudioRender audio;
    LumaPlayerTestVideoRender video;
    LumaPlayerCore core;
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
        // 按实际帧数结束，100次断言不变；系统调度粒度不能当作播放帧数。
        const int64_t oneFrameDeadline = LumaPlayerTestHelper::nowMs() + 8000;
        while (video.frameHistory().size() < 100 && LumaPlayerTestHelper::nowMs() < oneFrameDeadline)
        {
            LumaPlayerTestHelper::sleepMs(20);
        }
        core.pause();
        std::vector<int64_t> oneFrameHistory = video.frameHistory();
        bool onlyA = !oneFrameHistory.empty();
        for (size_t i = 0; i < oneFrameHistory.size(); ++i)
        {
            onlyA = onlyA && oneFrameHistory[i] == range.start100ns();
        }
        report.info(std::string("singleFrameCount=") + std::to_string(oneFrameHistory.size()) + ", onlyA=" + (onlyA ? "true" : "false"));
        report.check(oneFrameHistory.size() >= 100 && onlyA, "一帧循环100次均保持A画面");
        report.check(audio.stats().m_pcmBytes > pcmBefore, "一帧循环持续提交音频");
        report.check(audio.stats().m_flushCount == flushOneFrame, "一帧循环不重置声卡");
    }
    core.uninit();
    report.endCase();
}

/** 验证拖动预览、最新请求替代与播放意图保持
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
@param [in] path 测试自产素材路径，调用期间有效
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
    LumaPlayerTestAudioRender audio;
    LumaPlayerTestVideoRender video;
    LumaPlayerCore core;
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

/** 验证AB移动任务的步进、完成和取消边界
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
@param [in] path 测试自产素材路径，调用期间有效
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

void LumaPlayerTestReentryRender::setCore(LumaPlayerCore* core)
{
    m_core = core;
}

int32_t LumaPlayerTestReentryRender::result() const
{
    return m_result.load();
}

/** 验证帧边界换算和速率范围
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
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
    source.m_duration100ns = 10000000;
    source.m_pcmData.resize(48000 * 4, 0);
    for (size_t i = 0; i < 48000; ++i)
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
        const bool converted = PlayerEngineHelper::scaleAudioRate(source, rates[i], &output);
        report.check(converted, "PCM转换倍率=" + std::to_string(rates[i]) + ", bytes=" + std::to_string(output.m_pcmData.size()));
        correct = correct && converted;
        // 流式音高周期在段尾允许不足一个周期的舍入，不再要求每个输入包独立精确缩放。
        const int64_t expectedFrames = static_cast<int64_t>(48000) * 1000 / rates[i];
        const int64_t actualFrames = static_cast<int64_t>(output.m_pcmData.size() / 4);
        correct = correct && std::abs(actualFrames - expectedFrames) <= 240;
        correct = correct && output.m_timestamp100ns == source.m_timestamp100ns && output.m_duration100ns == source.m_duration100ns;
        int32_t maxChannelError = 0, maxGainError = 0;
        for (size_t sample = 0; sample + 3 < output.m_pcmData.size(); sample += 4)
        {
            int16_t left = 0;
            int16_t right = 0;
            std::memcpy(&left, &output.m_pcmData[sample], 2);
            std::memcpy(&right, &output.m_pcmData[sample + 2], 2);
            correct = correct && left >= -2 && left <= 1236 && right <= 2 && right >= -4323 &&
                std::abs(static_cast<int32_t>(left) * 4321 + static_cast<int32_t>(right) * 1234) <= 5555;
            if (sample < output.m_pcmData.size() / 2)
            {
                maxChannelError = (std::max)(maxChannelError, std::abs(static_cast<int32_t>(left) * 4321 + static_cast<int32_t>(right) * 1234));
                maxGainError = (std::max)(maxGainError, std::abs(static_cast<int32_t>(right) + 4321));
                correct = correct && std::abs(static_cast<int32_t>(left) - 1234) <= 2 &&
                    std::abs(static_cast<int32_t>(right) + 4321) <= 2;
            }
        }
        report.info("rate=" + std::to_string(rates[i]) + " maxChannelError=" + std::to_string(maxChannelError) + " maxGainError=" + std::to_string(maxGainError));
    }
    report.check(correct, "0.05至5倍PCM时长误差不超过5ms，声道增益比例保持且媒体时间不变");
    report.check(source.m_pcmData == original, "倍速不修改循环缓存中的源PCM");
    LumaPlayerAudioFrame tone;
    tone.m_format.m_sampleRate = 48000;
    tone.m_format.m_channels = 1;
    tone.m_format.m_bitsPerSample = 16;
    const size_t toneFrames = 9600;
    tone.m_pcmData.resize(toneFrames * sizeof(int16_t));
    for (size_t index = 0; index < toneFrames; ++index)
    {
        const double sample = std::sin(2.0 * 3.14159265358979323846 * 440.0 * index / tone.m_format.m_sampleRate) * 12000.0;
        const int16_t value = static_cast<int16_t>(sample);
        std::memcpy(&tone.m_pcmData[index * sizeof(int16_t)], &value, sizeof(value));
    }
    int32_t inputCrossings = 0;
    int16_t previous = 0;
    for (size_t index = 0; index < toneFrames; ++index)
    {
        int16_t value = 0;
        std::memcpy(&value, &tone.m_pcmData[index * sizeof(int16_t)], sizeof(value));
        if (previous <= 0 && value > 0)
        {
            ++inputCrossings;
        }
        previous = value;
    }
    const int32_t pitchRates[] = {500, 2000, 4000};
    for (size_t rateIndex = 0; rateIndex < sizeof(pitchRates) / sizeof(pitchRates[0]); ++rateIndex)
    {
        LumaPlayerAudioFrame stretched;
        const bool converted = PlayerEngineHelper::scaleAudioRate(tone, pitchRates[rateIndex], &stretched);
        int32_t outputCrossings = 0;
        previous = 0;
        for (size_t index = 0; converted && index < stretched.m_pcmData.size() / sizeof(int16_t); ++index)
        {
            int16_t value = 0;
            std::memcpy(&value, &stretched.m_pcmData[index * sizeof(int16_t)], sizeof(value));
            if (previous <= 0 && value > 0)
            {
                ++outputCrossings;
            }
            previous = value;
        }
        const double outputSeconds = converted ? static_cast<double>(stretched.m_pcmData.size() / sizeof(int16_t)) / tone.m_format.m_sampleRate : 0.0;
        const double outputFrequency = outputSeconds > 0.0 ? outputCrossings / outputSeconds : 0.0;
        const bool pitchPreserved = converted && inputCrossings > 0 && outputCrossings > 0 &&
            std::fabs(outputFrequency - 440.0) < 55.0;
        report.check(pitchPreserved, "倍速=" + std::to_string(pitchRates[rateIndex] / 1000.0) + "时输出频率保持在440Hz附近");
    }
    report.endCase();
}

/** 验证静音补全、跳转、覆盖录制及时间映射
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
*/
static void RunKaraokeTimelineCase(LumaPlayerTestReport& report)
{
    const int64_t second = 10000000;
    KaraokeTimeline timeline;
    report.beginCase("KaraokeTimelineOverwriteAndRateMapping");
    report.check(timeline.advanceTo(80 * second), "推进到01:20，开始录制前时间线为空音");
    report.check(timeline.replaceCapturedRange(80 * second, 83 * second, 1000),
        "写入01:20至01:23的首段人声");
    report.check(timeline.advanceTo(88 * second), "暂停后跳过5秒，时间线延伸到01:28");
    report.check(timeline.replaceCapturedRange(81 * second, 82 * second, 2000),
        "回到01:21并以2倍速覆盖到01:22");

    const std::vector<KaraokeTimelineSegment> segments = timeline.segments();
    report.check(segments.size() == 5, "时间线包含前静音、保留段、覆盖段、保留段及跳过静音");
    if (segments.size() == 5)
    {
        report.check(!segments[0].m_hasMicrophoneAudio &&
                segments[0].m_mediaStart100ns == 0 && segments[0].m_mediaEnd100ns == 80 * second &&
                segments[0].m_ratePermille == 1000,
            "01:20之前自动补80秒静音并按1倍速导出");
        report.check(segments[1].m_hasMicrophoneAudio &&
                segments[1].m_mediaStart100ns == 80 * second && segments[1].m_mediaEnd100ns == 81 * second &&
                segments[1].m_ratePermille == 1000,
            "覆盖前的旧录音区间保留");
        report.check(segments[2].m_hasMicrophoneAudio &&
                segments[2].m_mediaStart100ns == 81 * second && segments[2].m_mediaEnd100ns == 82 * second &&
                segments[2].m_ratePermille == 2000,
            "只有新录音实际覆盖的区间被替换");
        report.check(segments[3].m_hasMicrophoneAudio &&
                segments[3].m_mediaStart100ns == 82 * second && segments[3].m_mediaEnd100ns == 83 * second &&
                segments[3].m_ratePermille == 1000,
            "覆盖后的旧录音区间保留");
        report.check(!segments[4].m_hasMicrophoneAudio &&
                segments[4].m_mediaStart100ns == 83 * second && segments[4].m_mediaEnd100ns == 88 * second &&
                segments[4].m_ratePermille == 1000,
            "跳过的5秒自动补静音并按1倍速导出");
    }
    int64_t outputDuration = 0;
    report.check(timeline.outputDuration100ns(&outputDuration) &&
            outputDuration == 875000000,
        "变速后成品时长为87.5秒，不强行补到88秒");
    int64_t outputPosition = 0;
    report.check(timeline.mediaToOutputTime100ns(82 * second, &outputPosition) &&
            outputPosition == 815000000,
        "媒体01:22映射到变速成品时间轴01:21.5");
    int64_t mediaPosition = 0;
    report.check(timeline.outputToMediaTime100ns(outputPosition, &mediaPosition) &&
            mediaPosition == 82 * second,
        "输出时间反向映射回相同媒体边界");
    report.check(!timeline.replaceCapturedRange(90 * second, 91 * second, 99),
        "拒绝低于0.1倍的无效录音倍率");
    report.check(!timeline.replaceCapturedRange(90 * second, 90 * second, 1000),
        "拒绝空录音区间");
    report.check(timeline.mediaDuration100ns() == 88 * second && timeline.segments().size() == 5,
        "无效写入不改变已提交的时间线");

    KaraokeTimeline rateTimeline;
    report.check(rateTimeline.replaceCapturedRange(10 * second, 14 * second, 2000),
        "建立独立的2倍速录音区间");
    report.check(rateTimeline.outputDuration100ns(&outputDuration) &&
            outputDuration == 12 * second,
        "前10秒静音按1倍速、后4秒录音按2倍速，成品共12秒");
    report.endCase();
}

/** 验证实际PCM覆盖及倍速映射
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
*/
static void RunKaraokePcmTimelineCase(LumaPlayerTestReport& report)
{
    const int64_t second = 10000000;
    KaraokePcmTimeline recording;
    report.beginCase("KaraokePcmTimelineOverwriteAndSilence");
    report.check(recording.init(4, 1, 256), "初始化有界单声道PCM时间线");
    int16_t firstCapture[12];
    for (int32_t i = 0; i < 12; ++i)
    {
        firstCapture[i] = static_cast<int16_t>(i + 1);
    }
    report.check(recording.advanceTo(2 * second), "录制前推进到第2秒");
    report.check(recording.replaceCapturedRange(2 * second, 5 * second, 1000,
                     firstCapture, 12),
        "保存第2至5秒的原录音PCM");
    report.check(recording.advanceTo(6 * second), "向前跳过1秒并保留空白");
    uint64_t frameCount = 0;
    report.check(recording.outputFrameCount(&frameCount) && frameCount == 24,
        "两秒起始静音、三秒录音和一秒尾部静音生成24帧");
    std::vector<int16_t> output(static_cast<size_t>(frameCount), -1);
    report.check(recording.renderOutputFrames(0, output.size(), &output[0]),
        "整段PCM时间线可按输出顺序读取");
    bool initialSilence = true;
    for (size_t i = 0; i < 8 && i < output.size(); ++i)
    {
        initialSilence = initialSilence && output[i] == 0;
    }
    report.check(initialSilence, "媒体起点到首次录音前写出真实静音帧");
    bool originalCapture = output.size() >= 20;
    for (size_t i = 0; i < 12 && output.size() >= 20; ++i)
    {
        originalCapture = originalCapture && output[8 + i] == firstCapture[i];
    }
    bool trailingSilence = true;
    for (size_t i = 20; i < output.size(); ++i)
    {
        trailingSilence = trailingSilence && output[i] == 0;
    }
    report.check(originalCapture && trailingSilence,
        "录音样本原样保留，向前跳过区间以零样本补齐");

    const int16_t replacement[] = {100, 101};
    report.check(recording.replaceCapturedRange(3 * second, 4 * second, 2000,
                     replacement, 2),
        "以2倍速回到第3至4秒覆盖录制");
    report.check(recording.outputFrameCount(&frameCount) && frameCount == 22,
        "覆盖区间按实际2倍速时长映射，成品缩短两帧");
    output.assign(static_cast<size_t>(frameCount), -1);
    const bool rendered = recording.renderOutputFrames(0, output.size(), &output[0]);
    const bool replacedAndPreserved = rendered && output.size() == 22 &&
        output[8] == 1 && output[9] == 2 && output[10] == 3 && output[11] == 4 &&
        output[12] == 100 && output[13] == 101 &&
        output[14] == 9 && output[15] == 10 && output[16] == 11 && output[17] == 12;
    report.check(replacedAndPreserved,
        "新PCM覆盖指定范围，覆盖前后未涉及的旧PCM仍保留");
    int16_t smallRead[6] = {};
    report.check(recording.renderOutputFrames(10, 6, smallRead) &&
            smallRead[0] == 3 && smallRead[1] == 4 && smallRead[2] == 100 &&
            smallRead[3] == 101 && smallRead[4] == 9 && smallRead[5] == 10,
        "跨多个录音片段分块读取时边界连续");

    const size_t bytesBeforeFailure = recording.storedPcmBytes();
    int16_t oversized[1] = {7};
    report.check(!recording.replaceCapturedRange(6 * second, 7 * second, 1000,
                     oversized, 1000) &&
            recording.storedPcmBytes() == bytesBeforeFailure &&
            recording.outputFrameCount(&frameCount) && frameCount == 22,
        "内存上限拒绝超限写入且不改变已提交PCM和时间线");
    report.check(!recording.renderOutputFrames(22, 1, smallRead),
        "拒绝越过已生成的成品PCM末尾读取");
    KaraokePcmTimeline song;
    report.check(song.init(4, 1, 256) && song.configureSong(8 * second, 2000),
        "fixed 2x recording configures the complete eight-second song");
    report.check(song.replaceCapturedRange(2 * second, 4 * second, 2000, firstCapture, 4) &&
            !song.replaceCapturedRange(4 * second, 5 * second, 1000, firstCapture, 4),
        "capture accepts locked rate and rejects rate changes");
    report.check(song.outputFrameCount(&frameCount) && frameCount == 16 && song.capturedFrameCount() == 4,
        "whole-song output is four seconds; capture counter excludes silence");
    output.assign(16, -1);
    report.check(song.renderOutputFrames(0, output.size(), output.data()) && output[0] == 0 &&
            output[3] == 0 && output[4] == 1 && output[7] == 4 && output[8] == 0 && output[15] == 0,
        "fixed speed also maps the unrecorded prefix and tail");
    report.check(song.replaceCapturedRange(3 * second, 4 * second, 2000, replacement, 2) &&
            song.renderOutputFrames(0, output.size(), output.data()) && output[5] == 2 &&
            output[6] == 100 && output[7] == 101 && output[8] == 0,
        "overdub only replaces its interval without moving the remaining song");
    report.check(!song.configureSong(0, 2000) && !song.configureSong(8 * second, 99) &&
            !song.renderOutputFrames(16, 1, smallRead),
        "fixed-song invalid configuration and end bounds rejected");
    report.endCase();
}

/** 验证K歌C接口的输入、状态及读取边界
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
*/
static void RunKaraokeCApiCase(LumaPlayerTestReport& report)
{
    const int64_t second = 10000000;
    report.beginCase("KaraokePcmTimelineCApiLifecycle");
    LumaPlayerCoreHandle handle = lumaPlayerCoreCreate();
    report.check(handle != nullptr, "创建带K歌PCM上下文的Core句柄");
    if (handle != nullptr)
    {
        uint64_t frameCount = 0;
        report.check(lumaPlayerCoreKaraokeOutputFrameCount(handle, &frameCount) ==
                LumaPlayerCoreCResultStateError,
            "未初始化录音时间线时拒绝查询");
        report.check(lumaPlayerCoreKaraokeInit(handle, 4, 1, 256) ==
                LumaPlayerCoreCResultSuccess,
            "通过C ABI初始化PCM录音时间线");
        const int16_t source[] = {11, 12, 13, 14};
        report.check(lumaPlayerCoreKaraokeAdvanceTo(handle, second) ==
                    LumaPlayerCoreCResultSuccess &&
                lumaPlayerCoreKaraokeWritePcm(handle, second, 2 * second, 1000,
                    source, 4) == LumaPlayerCoreCResultSuccess,
            "C ABI接收采集PCM并保留调用方输入副本");
        report.check(lumaPlayerCoreKaraokeAdvanceTo(handle, 3 * second) ==
                    LumaPlayerCoreCResultSuccess &&
                lumaPlayerCoreKaraokeOutputFrameCount(handle, &frameCount) ==
                    LumaPlayerCoreCResultSuccess &&
                frameCount == 12,
            "C ABI按起始静音、录音和尾部静音计算输出长度");
        int16_t output[12] = {};
        report.check(lumaPlayerCoreKaraokeReadPcm(handle, 0, 12, output) ==
                    LumaPlayerCoreCResultSuccess &&
                output[0] == 0 && output[3] == 0 &&
                output[4] == 11 && output[7] == 14 && output[8] == 0 && output[11] == 0,
            "C ABI分块读回样本并生成媒体空白静音");
        report.check(lumaPlayerCoreKaraokeReadPcm(handle, 12, 1, output) ==
                    LumaPlayerCoreCResultInvalidParam &&
                lumaPlayerCoreKaraokeClear(handle) == LumaPlayerCoreCResultSuccess &&
                lumaPlayerCoreKaraokeOutputFrameCount(handle, &frameCount) ==
                    LumaPlayerCoreCResultSuccess &&
                frameCount == 0,
            "越界读取拒绝且清理会话后保留格式配置");
        lumaPlayerCoreDestroy(handle);
    }
    report.endCase();
}

KaraokePitchCompletionState::KaraokePitchCompletionState() :
m_done(false),
m_result(LumaPlayerCoreCResultInternalError)
{

}

/** 记录升降调异步完成结果并发布完成标记
@param [in] completion 借用的C异步完成数据，仅回调期间有效
@param [in] userData 借用的回调上下文，在回调可发生期间保持有效
*/
static void KaraokePitchCompletionCallback(const LumaPlayerCoreCCompletion* completion, void* userData)
{
    KaraokePitchCompletionState* state = static_cast<KaraokePitchCompletionState*>(userData);
    if (state != nullptr && completion != nullptr)
    {
        state->m_result = completion->m_result;
        state->m_done.store(true);
    }
}

/** 提交升降调请求并等待指定结果
@param [in] handle 有效Core会话句柄，调用期间不得销毁
@param [in] requestId 当前请求唯一标识
@param [in] semitones 升降调半音数，-7至7
@param [in] expectedResult 独立预期的C结果码
@return 实际完成结果与独立预期匹配才返回true
*/
static bool SubmitKaraokePitchRequest(LumaPlayerCoreHandle handle, int32_t requestId,
    int32_t semitones, int32_t expectedResult)
{
    KaraokePitchCompletionState state;
    LumaPlayerCoreCRequest request = {};
    request.m_requestId = static_cast<uint64_t>(requestId);
    request.m_operation = LumaPlayerCoreCOperationKaraokePitch;
    request.m_ratePermille = semitones;
    if (lumaPlayerCoreSubmitAsyncEx(handle, &request, KaraokePitchCompletionCallback,
            &state) != LumaPlayerCoreCResultSuccess)
    {
        return false;
    }
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() +
        std::chrono::seconds(2);
    while (!state.m_done.load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return state.m_done.load() && state.m_result == expectedResult;
}

/** 验证升降调频率及异步结果边界
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
*/
static void RunKaraokePitchShiftCase(LumaPlayerTestReport& report)
{
    const int32_t sampleRate = 48000;
    const int32_t testSemitones[] = {-7, 0, 7};
    report.beginCase("KaraokePitchShiftPreservesDuration");
#ifdef _WIN32
    const std::vector<std::string> modules = CSystem::processMountDll(
        static_cast<uint32_t>(QCoreApplication::applicationPid()));
    const char* sharedLibraries[] = {
#ifdef _DEBUG
        "SoundTouchDLLd.dll",
#else
        "SoundTouchDLL.dll",
#endif
        "avformat-60.dll", "avcodec-60.dll", "avutil-58.dll", "swscale-7.dll", "swresample-4.dll"};
    for (size_t i = 0; i < sizeof(sharedLibraries) / sizeof(sharedLibraries[0]); ++i)
    {
        report.check(std::find(modules.begin(), modules.end(), sharedLibraries[i]) != modules.end(),
            std::string("LGPL dependency uses a separate runtime library: ") + sharedLibraries[i]);
    }
#endif
    for (size_t testIndex = 0; testIndex < sizeof(testSemitones) / sizeof(testSemitones[0]); ++testIndex)
    {
        const int32_t semitones = testSemitones[testIndex];
        std::vector<int16_t> input(sampleRate);
        for (size_t frame = 0; frame < input.size(); ++frame)
        {
            const double phase = 2.0 * 3.14159265358979323846 * 220.0 * frame / sampleRate;
            input[frame] = static_cast<int16_t>(std::sin(phase) * 12000.0);
        }

        FFmpegCppAudioTempo tempo;
        bool accepted = tempo.init(sampleRate, 1) && tempo.setRate(1000) &&
            tempo.setPitchSemitones(semitones);
        for (size_t offset = 0; accepted && offset < input.size();)
        {
            const size_t frames = (std::min)(static_cast<size_t>(257), input.size() - offset);
            accepted = tempo.write(&input[offset], frames);
            offset += frames;
        }
        accepted = accepted && tempo.finish();
        const size_t outputFrames = tempo.available();
        std::vector<int16_t> output(outputFrames);
        accepted = accepted && outputFrames > sampleRate * 9 / 10 &&
            outputFrames < sampleRate * 11 / 10 &&
            tempo.read(&output[0], outputFrames) == outputFrames;
        if (!accepted || outputFrames < 4)
        {
            report.check(false, "半音偏移处理失败或时长偏离1秒");
            continue;
        }

        const size_t begin = outputFrames / 5;
        const size_t end = outputFrames * 4 / 5;
        size_t positiveCrossings = 0;
        for (size_t frame = begin + 1; frame < end; ++frame)
        {
            if (output[frame - 1] <= 0 && output[frame] > 0)
            {
                ++positiveCrossings;
            }
        }
        const double measuredHz = static_cast<double>(positiveCrossings) * sampleRate /
            static_cast<double>(end - begin);
        const double expectedHz = 220.0 * std::pow(2.0, static_cast<double>(semitones) / 12.0);
        std::ostringstream details;
        details << "semitones=" << semitones << ", frames=" << outputFrames
                << ", measuredHz=" << measuredHz << ", expectedHz=" << expectedHz;
        report.info(details.str());
        report.check(std::fabs(measuredHz - expectedHz) < 5.0,
            "半音频率符合2^(n/12)，处理不改变速率映射");
    }

    FFmpegCppAudioTempo invalid;
    report.check(!invalid.setPitchSemitones(0) && invalid.init(sampleRate, 1) &&
            !invalid.setPitchSemitones(-8) && !invalid.setPitchSemitones(8),
        "仅接受初始化后的-7至+7半音输入");
    std::vector<int16_t> music(sampleRate * 4);
    for (size_t i = 0; i < music.size() / 2; ++i)
    {
        const double t = static_cast<double>(i) / sampleRate;
        music[2 * i] = static_cast<int16_t>(4000 * std::sin(1382.3007676 * t) +
            2000 * std::sin(2073.4511514 * t) + 1000 * std::sin(3455.751919 * t));
        music[2 * i + 1] = static_cast<int16_t>(3000 * std::sin(1643.185 * t) + 1500 * std::sin(2764.601535 * t));
    }
    for (int32_t pitch = -7; pitch <= 7; ++pitch)
    {
        std::vector<int16_t> outputs[2];
        bool good = true;
        for (int mode = 0; mode != 2; ++mode)
        {
            FFmpegCppAudioTempo stream;
            good = stream.init(sampleRate, 2) && stream.setRate(1100) && stream.setPitchSemitones(pitch) && good;
            for (size_t i = 0; good && i < music.size() / 2;)
            {
                const size_t frames = (std::min<size_t>)(mode ? 257 : music.size() / 2, music.size() / 2 - i);
                good = stream.write(music.data() + i * 2, frames);
                i += frames;
            }
            good = stream.finish() && good;
            outputs[mode].resize(stream.available() * 2);
            good = !outputs[mode].empty() && stream.read(outputs[mode].data(), outputs[mode].size() / 2) * 2 == outputs[mode].size() && good;
        }
        good = good && outputs[0] == outputs[1] && std::abs(static_cast<int64_t>(outputs[0].size() / 2) - 87273) <= 1;
        const std::vector<int16_t>& pcm = outputs[0];
        bool continuity = !pcm.empty();
        for (size_t begin = 9600; begin + 1920 < pcm.size(); begin += 960)
        {
            double energy = 0;
            for (size_t i = begin; i < begin + 960; i += 2)
            {
                energy += static_cast<double>(pcm[i]) * pcm[i];
                continuity = continuity && std::abs(static_cast<int>(pcm[i])) < 16000 &&
                    std::abs(static_cast<int>(pcm[i]) - pcm[i - 2]) < 5000;
            }
            continuity = continuity && energy > 10000000;
        }
        report.check(good && continuity, "stereo harmonic audio at 1.1x: duration, block invariance, no interior gaps/clipping/large discontinuities; pitch=" + std::to_string(pitch));
    }

    LumaPlayerCoreHandle handle = lumaPlayerCoreCreate();
    const bool initialized = handle != nullptr &&
        lumaPlayerCoreInit(handle) == LumaPlayerCoreCResultSuccess;
    report.check(initialized, "初始化播放器Core移调命令测试");
    if (initialized)
    {
        report.check(SubmitKaraokePitchRequest(handle, 4901, -7,
                         LumaPlayerCoreCResultSuccess) &&
                SubmitKaraokePitchRequest(handle, 4902, 7,
                    LumaPlayerCoreCResultSuccess),
            "C ABI异步命令接受两个移调端点并返回实际完成结果");
        report.check(SubmitKaraokePitchRequest(handle, 4903, 8,
                         LumaPlayerCoreCResultInvalidParam),
            "C ABI异步命令拒绝越界半音值");
    }
    if (handle != nullptr)
    {
        lumaPlayerCoreUninit(handle);
        lumaPlayerCoreDestroy(handle);
    }
    report.endCase();
}

/** 验证软件增益、静音、监听和饱和混音
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
*/
static void RunKaraokePcmProcessorCase(LumaPlayerTestReport& report)
{
    report.beginCase("KaraokeMicrophoneGainMuteAndMix");
    const int16_t input[] = {1000, -1000, 20000, -20000, 30000, -30000};
    int16_t recording[6] = {};
    int16_t monitor[6] = {};
    report.check(KaraokePcmProcessor::processMicrophone(input, 6, 100, false, true,
                     recording, monitor) &&
            std::memcmp(recording, input, sizeof(input)) == 0 &&
            std::memcmp(monitor, input, sizeof(input)) == 0,
        "100%增益且监听开启时录音和监听均保持原始PCM");
    report.check(KaraokePcmProcessor::processMicrophone(input, 6, 50, false, false,
                     recording, monitor) &&
            recording[0] == 500 && recording[1] == -500 &&
            monitor[0] == 0 && monitor[5] == 0,
        "监听关闭仅静音监听输出，录制轨仍应用独立麦克风增益");
    report.check(KaraokePcmProcessor::processMicrophone(input, 6, 500, false, true,
                     recording, monitor) &&
            recording[0] == 5000 && recording[1] == -5000 &&
            recording[2] == 32767 && recording[3] == -32768 &&
            recording[4] == 32767 && recording[5] == -32768,
        "500%增益饱和裁剪，不溢出或改变样本数");
    report.check(KaraokePcmProcessor::processMicrophone(input, 6, 2000, false, true,
                     recording, monitor) &&
            recording[0] == 20000 && recording[1] == -20000 &&
            recording[2] == 32767 && recording[3] == -32768,
        "2000%高增益即时放大、饱和保护且不改变样本数");
    report.check(KaraokePcmProcessor::processMicrophone(input, 6, 100, true, true,
                     recording, monitor) &&
            std::all_of(recording, recording + 6, [](int16_t value)
                {
                    return value == 0;
                }) &&
            std::all_of(monitor, monitor + 6, [](int16_t value)
                {
                    return value == 0;
                }),
        "麦克风静音与0%增益分开处理，并同时阻止监听和录入人声");
    const int16_t accompaniment[] = {30000, -30000, 100, -100};
    const int16_t microphone[] = {5000, -5000, -200, 200};
    int16_t mixed[4] = {};
    report.check(KaraokePcmProcessor::mixS16(accompaniment, microphone, 4, mixed) &&
            mixed[0] == 32767 && mixed[1] == -32768 && mixed[2] == -100 && mixed[3] == 100,
        "伴奏和人声相加时双向饱和且保持反相信号关系");
    int16_t unchanged[2] = {123, 456};
    const int16_t quiet[] = {100, -100};
    report.check(KaraokePcmProcessor::processMicrophone(quiet, 2, 5000, false, true, recording, monitor) &&
            recording[0] == 5000 && recording[1] == -5000 && monitor[0] == 5000 && monitor[1] == -5000,
        "实际5000%增益只放大一次，录制与监听一致");
    report.check(!KaraokePcmProcessor::processMicrophone(input, 2, 5001, false, true,
                     unchanged, monitor) &&
            unchanged[0] == 123 && unchanged[1] == 456 &&
            !KaraokePcmProcessor::mixS16(nullptr, microphone, 2, mixed),
        "拒绝超过5000%的增益和空输入且不污染原输出");
    report.endCase();
}

/** 验证录制、暂停与监听的独立语义
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
*/
static void RunKaraokeCaptureFlowCase(LumaPlayerTestReport& report)
{
    report.beginCase("KaraokeCapturePauseAndOverwriteFlow");
    const int64_t second = 10000000;
    LumaPlayerCoreHandle handle = lumaPlayerCoreCreate();
    report.check(handle != nullptr &&
            lumaPlayerCoreKaraokeInit(handle, 4, 1, 4096) == LumaPlayerCoreCResultSuccess,
        "建立4Hz单声道的Core录音时间线");
    if (handle != nullptr)
    {
        const int16_t firstInput[] = {1000, -1000, 2000, -2000};
        int16_t monitor[4] = {};
        report.check(lumaPlayerCoreKaraokeProcessMicrophone(handle,
                         second, 2 * second, 1000, firstInput, 4, 200, 0, 1, 1, 1,
                         monitor) == LumaPlayerCoreCResultSuccess &&
                monitor[0] == 2000 && monitor[1] == -2000,
            "播放且录音开启时应用增益、输出监听并写入媒体区间");

        const int16_t pausedInput[] = {3000, -3000, 4000, -4000};
        std::fill(monitor, monitor + 4, 0);
        report.check(lumaPlayerCoreKaraokeProcessMicrophone(handle,
                         2 * second, 2 * second, 1000, pausedInput, 4, 100, 0, 1, 1, 0,
                         monitor) == LumaPlayerCoreCResultSuccess &&
                monitor[0] == 3000 && monitor[3] == -4000,
            "暂停时即使录音开关仍开也继续监听");

        const int16_t unrecordedInput[] = {500, 500, 500, 500};
        std::fill(monitor, monitor + 4, 1);
        report.check(lumaPlayerCoreKaraokeProcessMicrophone(handle,
                         2 * second, 3 * second, 1000, unrecordedInput, 4, 100, 0, 0, 0, 1,
                         monitor) == LumaPlayerCoreCResultSuccess &&
                std::all_of(monitor, monitor + 4, [](int16_t value)
                    {
                        return value == 0;
                    }),
            "播放但录音开关关闭时推进总时长并保持监听静音");

        const int16_t fastInput[] = {6000, -6000, 7000, -7000};
        report.check(lumaPlayerCoreKaraokeProcessMicrophone(handle,
                         3 * second, 4 * second, 2000, fastInput, 2, 100, 0, 0, 1, 1,
                         monitor) == LumaPlayerCoreCResultSuccess,
            "2倍速录制按实际采样帧数保存并推进媒体区间");

        const int16_t overwriteInput[] = {9000, -9000};
        report.check(lumaPlayerCoreKaraokeProcessMicrophone(handle,
                         second + second / 2, 2 * second, 1000, overwriteInput, 2,
                         100, 0, 0, 1, 1, monitor) == LumaPlayerCoreCResultSuccess,
            "回到旧媒体位置录制时只覆盖相交片段");

        uint64_t frameCount = 0;
        int16_t output[14] = {};
        report.check(lumaPlayerCoreKaraokeOutputFrameCount(handle, &frameCount) ==
                    LumaPlayerCoreCResultSuccess &&
                frameCount == 14 &&
                lumaPlayerCoreKaraokeReadPcm(handle, 0, 14, output) ==
                    LumaPlayerCoreCResultSuccess,
            "首段空白、未录区间、2倍速段映射成完整输出时长");
        report.check(output[0] == 0 && output[1] == 0 && output[2] == 0 &&
                output[3] == 0 && output[4] == 2000 && output[5] == -2000 &&
                output[6] == 9000 && output[7] == -9000 &&
                output[8] == 0 && output[9] == 0 && output[10] == 0 &&
                output[11] == 0 && output[12] == 6000 && output[13] == -6000,
            "输出PCM验证首段静音、精准覆盖、跳过静音及加速后保留帧");
        report.check(lumaPlayerCoreKaraokeProcessMicrophone(handle,
                         4 * second, 4 * second, 1000, firstInput, 4, 100, 0, 1, 1, 1,
                         monitor) == LumaPlayerCoreCResultInvalidParam,
            "播放状态下拒绝零长度媒体区间");
        lumaPlayerCoreDestroy(handle);
    }
    report.endCase();
}

/** 验证跨块采样率和声道转换
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
*/
static void RunKaraokeMonitorPcmConversionCase(LumaPlayerTestReport& report)
{
    report.beginCase("KaraokeMonitorPcmFormatConversion");
    std::vector<int16_t> mono(2400, 1200);
    std::vector<int16_t> stereo;
    uint64_t remainder = 0;
    const bool resampled = KaraokePcmProcessor::convertMonitorS16(mono.data(),
        mono.size(), 48000, 1, 44100, 2, &remainder, &stereo);
    report.check(resampled && stereo.size() == 2205 * 2 && remainder == 0 &&
            std::all_of(stereo.begin(), stereo.end(), [](int16_t value)
                {
                    return value == 1200;
                }),
        "48kHz单声道转44.1kHz立体声后帧数准确且双声道不静音");

    const int16_t antiPhase[] = {2000, -1000, -400, 400};
    std::vector<int16_t> downmixed;
    report.check(KaraokePcmProcessor::convertMonitorS16(antiPhase, 2, 48000, 2,
                     48000, 1, &remainder, &downmixed) &&
            downmixed.size() == 2 &&
            downmixed[0] == 500 && downmixed[1] == 0,
        "立体声转单声道时取双声道均值，采样率不变不引入延迟帧");

    std::vector<int16_t> singleFrameOutput;
    uint64_t carriedRemainder = 0;
    size_t totalFrames = 0;
    const int16_t sample = -777;
    bool chunksValid = true;
    for (int32_t i = 0; i < 441; ++i)
    {
        chunksValid = chunksValid && KaraokePcmProcessor::convertMonitorS16(&sample, 1, 44100, 1, 48000, 1, &carriedRemainder, &singleFrameOutput);
        totalFrames += singleFrameOutput.size();
    }
    report.check(chunksValid && totalFrames == 480 && carriedRemainder == 0,
        "441个小分块从44.1kHz转48kHz累计正好480帧，无时长漂移");

    std::vector<int16_t> unchanged(1, 321);
    uint64_t unchangedRemainder = 7;
    report.check(!KaraokePcmProcessor::convertMonitorS16(&sample, 1, 44100, 1,
                     48000, 3, &unchangedRemainder, &unchanged) &&
            unchanged.size() == 1 &&
            unchanged[0] == 321 && unchangedRemainder == 7,
        "拒绝不支持的声道数并保持原输出与分块余数不变");
    report.endCase();
}

bool LumaPlayerTestThrowingVideoRender::openVideo(const LumaPlayerVideoFormat& format)
{
    (void)format;
    throw std::runtime_error("test video output failure");
}

/** 验证渲染重入、结束帧和跳转边界
@param [in,out] report 接收每个独立检查点的结果，借用至用例结束
@param [in] mediaPath 测试自产媒体路径，调用期间有效
*/
static void RunReentryAndPreviewEndCase(LumaPlayerTestReport& report, const std::string& mediaPath)
{
    report.beginCase("ReentryAndPreviewEnd");
    LumaPlayerTestReentryRender video(nullptr);
    LumaPlayerCore core;
    video.setCore(&core);
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
                frame.timestamp100ns + frame.duration100ns >= reader.duration100ns(),
            "EOF排空后能够定位末展示帧");
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

int RunLegacyTest(int32_t caseId, const std::string& mediaPath, bool sceneFixture, const std::string& reportPath)
{
    LumaPlayerTestReport report;
    if (mediaPath.empty() && caseId != CaseKaraokeMonitorPcmConversion)
    {
        return 2;
    }
    switch (caseId)
    {
    case CaseCppLifecycle:
    {
        RunCppLifecycleCase(report);
        break;
    }
    case CaseRepeatedInit:
    {
        RunCppRepeatedInitCase(report);
        break;
    }
    case CaseCApiLifecycle:
    {
        RunCApiLifecycleCase(report);
        break;
    }
    case CaseCApiMedia:
    {
        RunCApiMediaCase(report, mediaPath);
        break;
    }
    case CaseMediaControl:
    {
        RunMediaCase(report, mediaPath);
        break;
    }
    case CaseConfiguration:
    {
        RunConfigurationCase(report, mediaPath);
        break;
    }
    case CaseFullLoopCache:
    {
        RunLoopStartCase(report, mediaPath, sceneFixture, 64 * 1024 * 1024);
        break;
    }
    case CasePrefixLoopCache:
    {
        RunLoopStartCase(report, mediaPath, sceneFixture, 1);
        break;
    }
    case CaseLatestPreview:
    {
        RunDragPreviewCase(report, mediaPath);
        break;
    }
    case CaseLoopMoveCancel:
    {
        RunLoopMoveTaskCase(report, mediaPath);
        break;
    }
    case CaseFrameAndRate:
    {
        RunFrameBoundaryAndRateCase(report);
        break;
    }
    case CaseReentry:
    {
        RunReentryAndPreviewEndCase(report, mediaPath);
        break;
    }
    case CaseKaraokeTimeline:
    {
        RunKaraokeTimelineCase(report);
        break;
    }
    case CaseKaraokePcmTimeline:
    {
        RunKaraokePcmTimelineCase(report);
        break;
    }
    case CaseKaraokeCApi:
    {
        RunKaraokeCApiCase(report);
        break;
    }
    case CaseKaraokePitchShift:
    {
        RunKaraokePitchShiftCase(report);
        break;
    }
    case CaseKaraokePcmProcessing:
    {
        RunKaraokePcmProcessorCase(report);
        break;
    }
    case CaseKaraokeCaptureFlow:
    {
        RunKaraokeCaptureFlowCase(report);
        break;
    }
    case CaseKaraokeMonitorPcmConversion:
    {
        RunKaraokeMonitorPcmConversionCase(report);
        break;
    }
    default:
    {
        return 2;
    }
    }
    const bool written = report.writeReport(reportPath);
    return written && report.failedCount() == 0 ? 0 : 1;
}