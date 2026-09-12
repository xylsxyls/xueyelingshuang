#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "CoreTestTask.h"
#include "LegacyTests.h"
#include "TestConfig.h"
#include "TestCaseRegistry.h"
#include "TestCommandTask.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QCoreApplication>
#include <QCryptographicHash>
#include "TestResults.h"
#include "LogManager/LogManagerAPI.h"
#include <QFileInfo>
#include <QDir>
#include <chrono>
#include <thread>
#include <exception>
#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")
#endif

extern "C" int RunPureCApiSmoke(void);

/** 等待真实帧输出到达门槛，退出或超时均返回false
@param [in] video 帧计数器
@param [in] target 期望累计帧数
@param [in] exit 协作停止标记
@return 是否实际达到
*/
static bool WaitFrames(LumaPlayerTestVideoRender& video, int32_t target, const std::atomic<bool>& exit)
{
    const int64_t deadline = LumaPlayerTestHelper::nowMs() + 4000;
    while (!exit.load() && LumaPlayerTestHelper::nowMs() < deadline)
    {
        if (video.stats().m_frameCount >= target)
        {
            return true;
        }
        LumaPlayerTestHelper::sleepMs(5);
    }
    return false;
}

CoreTestTask::CoreTestTask(const QString& mode, const QString& media, const QString& fixture,
    const QString& directory, const std::shared_ptr<TestRunState>& state) :
m_exit(false),
m_mode(mode),
m_media(media),
m_fixture(fixture),
m_directory(directory),
m_state(state)
{

}

void CoreTestTask::StopTask()
{
    m_exit.store(true);
}

void CoreTestTask::DoTask()
{
    const int32_t id = m_mode.toInt();
    TestResults report(m_directory, id);
    try
    {
        if (m_exit.load())
        {
            m_state->m_result.store(3);
            m_state->m_done.store(true);
            return;
        }
        if (id >= 1 && id <= 12)
        {
            const int32_t result = RunLegacyTest(id, m_fixture.toUtf8().constData(), true,
                (m_directory + "/legacy.txt").toUtf8().constData());
            report.check(id, result == 0, kTestCases.at(static_cast<TestCaseId>(id)).m_description + QStringLiteral("；断言详见legacy.txt"));
            if (id == CaseCApiLifecycle)
            {
                report.check(id, RunPureCApiSmoke() == 0, QStringLiteral("独立.c文件编译及C生命周期实际运行"));
            }
        }
        else if (id == CaseThreadStopRace)
        {
            const int32_t before = CTaskThreadManager::Instance().Count();
            for (int32_t iteration = 0; iteration < 200 && !m_exit.load(); ++iteration)
            {
                const uint32_t threadId = CTaskThreadManager::Instance().Init();
                std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
                if (thread == nullptr)
                {
                    report.check(id, false, "regression thread creation failed");
                    break;
                }
                // 有限空任务可能恰好在StopCurTask判空与调用之间完成并被清空。
                std::shared_ptr<std::atomic<bool>> started(new std::atomic<bool>(false));
                std::shared_ptr<TestCommandTask> task(new TestCommandTask([started](const std::atomic<bool>& exit) {
                    started->store(true);
                    for (int32_t step = 0; step < 32 && !exit.load(); ++step)
                    {
                        std::this_thread::yield();
                    }
                }));
                thread->PostTask(task, 1);
                const int64_t deadline = LumaPlayerTestHelper::nowMs() + 1000;
                while (!started->load() && !m_exit.load() && LumaPlayerTestHelper::nowMs() < deadline)
                {
                    std::this_thread::yield();
                }
                for (int32_t stop = 0; stop < 32; ++stop)
                {
                    thread->StopCurTask();
                }
                thread->StopTask(task->GetTaskId());
                thread.reset();
                CTaskThreadManager::Instance().Uninit(threadId);
                report.check(id, started->load() && CTaskThreadManager::Instance().Count() == before,
                    "thread stop and reclaim iteration=" + QString::number(iteration + 1));
            }
        }
        else if (id == CaseSelectionRules)
        {
            std::set<int32_t> active;
            active.insert(8); active.insert(9); active.insert(11); active.insert(12);
            std::set<int32_t> deleted;
            deleted.insert(10);
            std::vector<int32_t> selected;
            std::vector<int32_t> skipped;
            report.check(id, TestCaseRegistry::parse("8-12,8", active, deleted, &selected, &skipped).isEmpty() &&
                selected == std::vector<int32_t>({8, 9, 11, 12}) && skipped == std::vector<int32_t>({10}),
                QStringLiteral("闭区间去重并自动跳过已删除10号"));
            const char* invalid[] = {"0", "12-8", "8,99", "8,,9", "-1", "2147483648", "10"};
            for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
            {
                report.check(id, !TestCaseRegistry::parse(invalid[i], active, deleted, &selected, &skipped).isEmpty(),
                    QStringLiteral("应拒绝无效或空选择：") + invalid[i]);
            }
            report.check(id, TestCaseRegistry::validate().isEmpty(), QStringLiteral("枚举与中文登记完整且无冲突"));
        }
        else if (id == CaseDiagnostics)
        {
            LOGINFO("Diagnostic case checks actual log evidence");
            const QStringList logs = QDir(m_directory).entryList(QStringList() << "*.log", QDir::Files);
            bool haveLog = false;
            for (int32_t index = 0; index < logs.size(); ++index)
            {
                QFile log(m_directory + "/" + logs[index]);
                if (log.open(QIODevice::ReadOnly) && log.readAll().contains("main.cpp"))
                {
                    haveLog = true;
                }
            }
            report.check(id, haveLog, QStringLiteral("日志实际落盘并包含源码文件信息"));
        }
        else
        {
            LumaPlayerTestAudioRender audio;
            LumaPlayerTestVideoRender video;
            LumaPlayerCore core;
            core.setAudioRender(&audio);
            core.setVideoRender(&video);
            core.setLogEnabled(true);
            report.check(2, core.init() == LumaPlayerCoreResultSuccess, "Core init");
            if (core.openMedia(m_fixture.toUtf8().constData()) != LumaPlayerCoreResultSuccess)
            {
                report.check(3, false, QStringLiteral("确定性素材打开失败：") + QString::fromStdString(core.lastError()));
                core.uninit();
            }
            else if (id == CaseLongPlayback || id == CaseLongLoop)
            {
                TestConfig config;
                for (int32_t phase = id == CaseLongPlayback ? 0 : 1; phase <= (id == CaseLongPlayback ? 0 : 1) && !m_exit.load(); ++phase)
                {
                    core.pause();
                    core.clearLoop();
                    if (phase == 1)
                    {
                        report.check(19, core.setLoopAAtPosition(10000000) == LumaPlayerCoreResultSuccess &&
                            core.setLoopBAtPosition(11600000) == LumaPlayerCoreResultSuccess, "short AB setup");
                    }
                    core.seekTo(phase == 0 ? 0 : 10000000, false);
                    report.check(19, core.play() == LumaPlayerCoreResultSuccess, "stress play actual result");
                    const int64_t begin = LumaPlayerTestHelper::nowMs();
                    int32_t previous = video.stats().m_frameCount;
                    int64_t lastFrame = begin;
                    int32_t cycles = 0;
                    int64_t lastPts = -1;
                    int32_t samples = 0;
                    bool alive = true;
                    while (!m_exit.load() && LumaPlayerTestHelper::nowMs() - begin < config.m_stressMs)
                    {
                        LumaPlayerTestHelper::sleepMs(100);
                        LumaPlayerTestVideoStats stats = video.stats();
                        if (stats.m_frameCount > previous)
                        {
                            lastFrame = LumaPlayerTestHelper::nowMs();
                            previous = stats.m_frameCount;
                        }
                        alive = alive && LumaPlayerTestHelper::nowMs() - lastFrame < 3000;
                        const std::vector<int64_t> history = video.frameHistory();
                        for (size_t i = 0; i < history.size(); ++i)
                        {
                            if (lastPts >= 0 && history[i] < lastPts)
                            {
                                ++cycles;
                            }
                            lastPts = history[i];
                        }
                        video.clearFrameHistory();
                        if (++samples % 50 == 0)
                        {
#ifdef _WIN32
                            PROCESS_MEMORY_COUNTERS memory = {};
                            memory.cb = sizeof(memory);
                            DWORD handles = 0;
                            GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory));
                            GetProcessHandleCount(GetCurrentProcess(), &handles);
                            LOGINFO("Stress phase=%d elapsed=%lld frames=%d workingSet=%llu handles=%lu", phase,
                                LumaPlayerTestHelper::nowMs() - begin, previous,
                                static_cast<unsigned long long>(memory.WorkingSetSize), handles);
#endif
                            report.check(19, alive, "playback liveness checkpoint phase=" + QString::number(phase) +
                                " elapsedMs=" + QString::number(LumaPlayerTestHelper::nowMs() - begin));
                        }
                    }
                    core.pause();
                    report.check(19, !m_exit.load() && alive && cycles >= 2,
                        "phase=" + QString::number(phase) + " cycles=" + QString::number(cycles));
                }
                core.uninit();
            }
            else
            {
                if (id == CaseMediaAndPixels)
                {
                    report.check(3, core.mediaInfo().m_hasVideo && core.mediaInfo().m_hasAudio, "AVI video/audio streams present");
                    report.check(20, core.previewFrame(9600000) == LumaPlayerCoreResultSuccess, "known red frame at 0.96s");
                    std::vector<uint32_t> colors = video.frameColors();
                    report.check(20, !colors.empty() && ((colors.back() >> 16) & 255) > 200 && (colors.back() & 255) < 30,
                        "decoded first pixel matches independent red fixture");
                    report.check(20, core.previewFrame(10000000) == LumaPlayerCoreResultSuccess, "known blue frame at 1s");
                    colors = video.frameColors();
                    report.check(20, !colors.empty() && (colors.back() & 255) > 200 && ((colors.back() >> 16) & 255) < 30,
                        "decoded first pixel matches independent blue fixture");
                }
                if (id == CaseAbBoundary)
                {
                    report.check(8, core.setLoopAAtPosition(10100000) == LumaPlayerCoreResultSuccess &&
                        core.setLoopBAtPosition(20100000) == LumaPlayerCoreResultSuccess, "set frame-rounded AB");
                    LumaPlayerLoopRange range = core.snapshot().m_loopRange;
                    report.check(8, range.start100ns() == 10000000 && range.end100ns() == 20400000,
                        "25fps independent boundary expected [10000000,20400000)");
                    core.seekTo(range.start100ns(), false);
                    video.clearFrameHistory();
                    report.check(8, core.play() == LumaPlayerCoreResultSuccess && WaitFrames(video, video.stats().m_frameCount + 65, m_exit),
                        "play at least two AB cycles");
                    core.pause();
                    std::vector<int64_t> frames = video.frameHistory();
                    bool bounded = !frames.empty();
                    bool endSeen = false;
                    for (size_t i = 0; i < frames.size(); ++i)
                    {
                        bounded = bounded && frames[i] >= range.start100ns() && frames[i] < range.end100ns();
                        endSeen = endSeen || frames[i] == 20000000;
                    }
                    report.check(8, bounded && endSeen, "B frame included; no frame at/after exclusive B");
                }
                if (id == CasePlayingAb)
                {
                    core.clearLoop();
                    core.seekTo(0, false);
                    core.play();
                    WaitFrames(video, video.stats().m_frameCount + 3, m_exit);
                    LumaPlayerTestAudioStats before = audio.stats();
                    report.check(9, core.setLoopAAtPosition(30000000) == LumaPlayerCoreResultSuccess &&
                        core.setLoopBAtPosition(50000000) == LumaPlayerCoreResultSuccess, "edit AB while playing");
                    LumaPlayerTestAudioStats after = audio.stats();
                    report.check(9, core.snapshot().m_state == LumaPlayerCoreStatePlaying &&
                        before.m_pauseCount == after.m_pauseCount && before.m_flushCount == after.m_flushCount &&
                            before.m_resumeCount == after.m_resumeCount, "playing AB edit causes no pause/flush/resume");
                }
                if (id == CaseAbReentry)
                {
                    core.pause();
                    core.setLoopAAtPosition(10000000);
                    core.setLoopBAtPosition(20000000);
                    core.seekTo(30000000, true);
                    report.check(10, WaitFrames(video, video.stats().m_frameCount + 8, m_exit) &&
                        video.stats().m_lastTimestamp100ns >= 30000000, "manual seek outside AB continues outside");
                    core.seekTo(12000000, true);
                    video.clearFrameHistory();
                    WaitFrames(video, video.stats().m_frameCount + 40, m_exit);
                    core.pause();
                    std::vector<int64_t> frames = video.frameHistory();
                    bool wrapped = false;
                    for (size_t i = 1; i < frames.size(); ++i)
                    {
                        wrapped = wrapped || frames[i] < frames[i - 1];
                    }
                    report.check(10, wrapped, "manual reentry restores AB looping");
                    core.seekTo(core.mediaInfo().m_duration100ns - 800000, true);
                    video.clearFrameHistory();
                    WaitFrames(video, video.stats().m_frameCount + 35, m_exit);
                    core.pause();
                    frames = video.frameHistory();
                    bool zeroSeen = false;
                    for (size_t i = 0; i < frames.size(); ++i)
                    {
                        zeroSeen = zeroSeen || frames[i] == 0;
                    }
                    report.check(10, zeroSeen, "outside AB EOF restarts at zero");
                }
                core.uninit();
                if (id == CaseExitRace)
                {
                    for (int32_t action = 0; action < 5 && !m_exit.load(); ++action)
                    {
                        for (int32_t iteration = 0; iteration < 5 && !m_exit.load(); ++iteration)
                        {
                            LumaPlayerCore instance;
                            LumaPlayerTestVideoRender output;
                            instance.setVideoRender(&output);
                            instance.init();
                            if (action == 0)
                            {
                                instance.openMediaAsync(m_fixture.toUtf8().constData());
                            }
                            else
                            {
                                instance.openMedia(m_fixture.toUtf8().constData());
                                if (action == 1)
                                {
                                    instance.seekToAsync(30000000, true);
                                }
                                else if (action == 2)
                                {
                                    for (int32_t n = 0; n < 50; ++n)
                                    {
                                        instance.previewFrameAsync(n * 800000);
                                    }
                                }
                                else if (action == 3)
                                {
                                    instance.setLoopAAtPosition(10000000);
                                    instance.setLoopBAtPosition(20000000);
                                    instance.moveLoopPointAsync(LumaPlayerCoreLoopPointA, 1);
                                    instance.cancelLoopPointMove();
                                }
                                else
                                {
                                    instance.setPlaybackRatePermilleAsync(1000);
                                    instance.playAsync();
                                }
                            }
                            instance.uninit();
                            const int32_t count = output.stats().m_frameCount;
                            LumaPlayerTestHelper::sleepMs(30);
                            report.check(16, count == output.stats().m_frameCount && !instance.isInit(),
                                "exit action=" + QString::number(action) + " iteration=" + QString::number(iteration));
                        }
                    }
                }
                if (id == CaseMultiCore)
                {
                    LumaPlayerCore first;
                    LumaPlayerCore second;
                    LumaPlayerTestVideoRender firstVideo;
                    LumaPlayerTestVideoRender secondVideo;
                    first.setVideoRender(&firstVideo);
                    second.setVideoRender(&secondVideo);
                    first.init();
                    second.init();
                    first.openMedia(m_fixture.toUtf8().constData());
                    second.openMedia(m_fixture.toUtf8().constData());
                    first.play();
                    second.play();
                    first.uninit();
                    report.check(17, WaitFrames(secondVideo, secondVideo.stats().m_frameCount + 5, m_exit), "second Core plays after first destroyed");
                    second.uninit();
                }
                if (id == CaseMediaAndPixels)
                {
                    core.init();
                    report.check(3, core.openMedia("Z:/absent-luma-test.mp4") != LumaPlayerCoreResultSuccess, "missing file rejected");
                    report.check(3, core.openMedia(m_media.toUtf8().constData()) == LumaPlayerCoreResultSuccess, "real MP4 opens after error");
                    core.uninit();
                }
            }
        }
        m_state->m_result.store(m_exit.load() ? 3 : (report.failures() == 0 ? 0 : 1));
    }
    catch (const std::exception& error)
    {
        report.check(1, false, QString::fromUtf8(error.what()));
        m_state->m_result.store(1);
    }
    catch (...)
    {
        report.check(1, false, "unhandled test exception");
        m_state->m_result.store(1);
    }
    m_state->m_done.store(true);
}