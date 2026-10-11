#include "LumaPlayerKaraokeJob.h"

#include <QTemporaryDir>
#include "Config.h"
#include "LumaPlayerKaraokeTask.h"
#include "Semaphore/SemaphoreAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <chrono>
#include <QFileInfo>
#include <QSaveFile>
#include <QDir>
#include <exception>
#include <cstring>
#include <algorithm>

LumaPlayerKaraokeJob::LumaPlayerKaraokeJob(const std::string& source,
    const std::string& runtime, const std::string& model) :
CTask(1),
m_source(source),
m_runtime(runtime),
m_model(model),
m_threadId(0),
m_cancel(false),
m_done(false),
m_progress(0),
m_result(nullptr),
m_core(nullptr),
m_mix(false),
m_video(false),
m_pitch(0),
m_gain(100),
m_separationWindowFrames(g_config.m_karaokeSeparationWindowFrames),
m_separationOverlapFrames(g_config.m_karaokeSeparationOverlapFrames),
m_backing(nullptr),
m_success(false),
m_end100ns(0)
{
    m_error[0] = 0;
}

LumaPlayerKaraokeJob::LumaPlayerKaraokeJob(LumaPlayerCoreBridge* core, const QString& source,
    const QString& destination, bool mix, bool video, int pitch, int gain, LumaPlayerCoreCBacking backing) :
CTask(1),
m_source(source.toUtf8().constData()),
m_threadId(0),
m_cancel(false),
m_done(false),
m_progress(0),
m_result(nullptr),
m_core(core),
m_destination(destination),
m_mix(mix),
m_video(video),
m_pitch(pitch),
m_gain(gain),
m_separationWindowFrames(g_config.m_karaokeSeparationWindowFrames),
m_separationOverlapFrames(g_config.m_karaokeSeparationOverlapFrames),
m_backing(backing),
m_success(false),
m_end100ns(0)
{
    m_error[0] = 0;
}

LumaPlayerKaraokeJob::LumaPlayerKaraokeJob(const std::vector<LumaPlayerKaraokeTask*>& captures,
    LumaPlayerCoreBridge* core, int64_t end100ns) :
CTask(1),
m_threadId(0),
m_cancel(false),
m_done(false),
m_progress(0),
m_result(nullptr),
m_core(core),
m_mix(false),
m_video(false),
m_pitch(0),
m_gain(100),
m_separationWindowFrames(g_config.m_karaokeSeparationWindowFrames),
m_separationOverlapFrames(g_config.m_karaokeSeparationOverlapFrames),
m_backing(nullptr),
m_success(false),
m_captures(captures),
m_end100ns(end100ns)
{
    m_error[0] = 0;
}

LumaPlayerKaraokeJob::~LumaPlayerKaraokeJob()
{
    StopTask();
    if (m_threadId != 0)
    {
        CTaskThreadManager::Instance().Uninit(m_threadId);
    }
    lumaPlayerCoreBackingDestroy(m_result);
}

bool LumaPlayerKaraokeJob::start()
{
    if (m_threadId != 0)
    {
        return false;
    }
    try
    {
        m_threadId = CTaskThreadManager::Instance().Init();
        std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(m_threadId);
        if (!thread)
        {
            return false;
        }
        /** 工作对象由宿主拥有，队列只借用而不销毁
        @param [in] task 借用的任务指针，宿主等待任务退出后才销毁
        */
        thread->PostTask(std::shared_ptr<CTask>(this, [](CTask* task)
                             {
                                 Q_UNUSED(task);
                             }),
            1);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void LumaPlayerKaraokeJob::DoTask()
{
    try
    {
        if (!m_captures.empty())
        {
            finishRecording();
        }
        else if (m_core)
        {
            exportRecording();
        }
        else
        {
            LumaPlayerCoreCSeparationOptions options = {sizeof(options), m_separationWindowFrames,
                m_separationOverlapFrames};
            m_result = lumaPlayerCoreSeparateVocalsEx(m_source.c_str(), m_runtime.c_str(), m_model.c_str(),
                static_cast<size_t>(512) * 1024 * 1024, &options, &LumaPlayerKaraokeJob::onProgress, this,
                m_error, sizeof(m_error));
            m_success = m_result != nullptr;
        }
        if (!m_success && !m_cancel.load())
        {
            LOGERROR("Karaoke task failed: %s", m_error);
        }
    }
    catch (const std::exception& e)
    {
        setError(e.what());
    }
    catch (...)
    {
        setError("Unexpected karaoke task failure");
    }
    m_done.store(true, std::memory_order_release);
}

void LumaPlayerKaraokeJob::setError(const char* message)
{
    if (message == nullptr || *message == 0)
    {
        message = "Karaoke task failed without a diagnostic";
    }
    const size_t count = (std::min)(sizeof(m_error) - 1, std::strlen(message));
    std::memcpy(m_error, message, count);
    m_error[count] = 0;
}

void LumaPlayerKaraokeJob::finishRecording()
{
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    Semaphore wait;
    while (!recordingSettled())
    {
        if (m_cancel.load())
        {
            setError("Recording stop canceled; recording retained");
            return;
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            setError("Recording stop timed out; recording retained");
            return;
        }
        wait.wait(5);
    }
    if (m_cancel.load())
    {
        setError("Recording stop canceled; recording retained");
        return;
    }
    if (m_core == nullptr || m_core->advanceKaraokeRecording(m_end100ns) != 0)
    {
        setError("Cannot finalize recording timeline");
        return;
    }
    m_success = true;
    m_progress.store(100);
}

void LumaPlayerKaraokeJob::exportRecording()
{
    if (m_cancel.load())
    {
        return;
    }
    QTemporaryDir temporary(QFileInfo(m_destination).absolutePath() + QStringLiteral("/.LumaPlayer-recording-XXXXXX"));
    if (!temporary.isValid())
    {
        setError("Cannot create temporary recording directory");
        return;
    }
    const QString path = temporary.path() + (m_video ? QStringLiteral("/recording.mp4") : QStringLiteral("/recording.mp3"));
    if (m_core->exportKaraoke(m_source.c_str(), path.toUtf8().constData(), m_mix, m_video, m_pitch, m_gain,
            m_backing, &LumaPlayerKaraokeJob::onProgress, this, m_error, sizeof(m_error)) != 0 ||
        m_cancel.load())
    {
        return;
    }
    QFile input(path);
    QSaveFile output(m_destination);
    output.setDirectWriteFallback(false);
    if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly))
    {
        setError("Cannot open recording destination; original file unchanged");
        return;
    }
    while (!input.atEnd())
    {
        if (m_cancel.load())
        {
            return;
        }
        const QByteArray bytes = input.read(1024 * 1024);
        if (bytes.isEmpty() || output.write(bytes) != bytes.size())
        {
            setError("Recording write failed; original file unchanged");
            return;
        }
    }
    if (m_cancel.load())
    {
        return;
    }
    if (!output.commit())
    {
        setError("Atomic recording save failed; original file unchanged");
        return;
    }
    m_progress.store(100);
    m_success = true;
}

void LumaPlayerKaraokeJob::StopTask()
{
    m_cancel.store(true);
}
bool LumaPlayerKaraokeJob::done() const
{
    return m_done.load(std::memory_order_acquire);
}
int LumaPlayerKaraokeJob::progress() const
{
    return m_progress.load();
}
bool LumaPlayerKaraokeJob::canceled() const
{
    return m_cancel.load();
}
bool LumaPlayerKaraokeJob::succeeded() const
{
    return done() && m_success;
}
QString LumaPlayerKaraokeJob::title() const
{
    return !m_captures.empty() ? QStringLiteral("正在结束录音") : (m_core ? QStringLiteral("正在保存录音") : QStringLiteral("正在消除人声"));
}

bool LumaPlayerKaraokeJob::recordingSettled() const
{
    for (size_t i = 0; i < m_captures.size(); ++i)
    {
        if (m_captures[i] && !m_captures[i]->recordingSettled())
        {
            return false;
        }
    }
    return true;
}
QString LumaPlayerKaraokeJob::error() const
{
    return done() ? QString::fromUtf8(m_error) : QString();
}

LumaPlayerCoreCBacking LumaPlayerKaraokeJob::takeResult()
{
    if (!done() || canceled())
    {
        return nullptr;
    }
    LumaPlayerCoreCBacking result = m_result;
    m_result = nullptr;
    return result;
}

int LumaPlayerKaraokeJob::onProgress(int percent, void* user)
{
    LumaPlayerKaraokeJob* task = static_cast<LumaPlayerKaraokeJob*>(user);
    task->m_progress.store(task->m_core ? percent * 95 / 100 : percent);
    return task->m_cancel.load() ? 0 : 1;
}