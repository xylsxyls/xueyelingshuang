#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "PlayerEngine.h"
#include "PlayerLoopBuffer.h"
#include "PlayerEngineHelper.h"
#include "PlayerPreviewCancelContext.h"
#include "PlayerPreviewTask.h"
#include "PlayerWorkerTask.h"
#include "PlayerCommandTask.h"

#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "FFmpegCpp/FFmpegCppAPI.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <new>

// 一秒包含的100纳秒计时单位，64位计算避免中间乘法溢出
#define LUMAPLAYER_TICKS_PER_SECOND (static_cast<int64_t>(1000) * 1000 * 10)

/** FFmpegCpp预览取消回调，返回true表示当前预览请求已经被更新请求替代
@param [in] userData PlayerPreviewCancelContext指针
@return true表示取消当前预览
*/
static bool PlayerPreviewCancelCallback(void* userData)
{
	PlayerPreviewCancelContext* context = static_cast<PlayerPreviewCancelContext*>(userData);
	if (context == nullptr)
	{
		return false;
	}
	if (context->m_exitFlag != nullptr && context->m_exitFlag->load())
	{
		return true;
	}
	return context->m_previewSerial != nullptr && context->m_requestSerial != 0 && context->m_requestSerial != context->m_previewSerial->load();
}

/** 将音频偏移换算为有界采样下标，避免时间乘采样率溢出
@param [in] offset100ns 非负偏移
@param [in] sampleRate 有效采样率
@param [in] limit 总采样数
@param [in] roundUp true为起点向上取整，避免包含范围之前的采样
@return 不超过limit的下标
*/
static size_t PlayerAudioSampleOffset(int64_t offset100ns, int32_t sampleRate, size_t limit, bool roundUp)
{
    if (offset100ns <= 0) { return 0; }
    const uint64_t seconds = static_cast<uint64_t>(offset100ns / LUMAPLAYER_TICKS_PER_SECOND);
    const uint64_t rate = static_cast<uint64_t>(sampleRate);
    if (seconds > limit / rate) { return limit; }
    const uint64_t whole = seconds * rate;
    const uint64_t fraction = static_cast<uint64_t>(offset100ns % LUMAPLAYER_TICKS_PER_SECOND) * rate;
    const uint64_t partial = fraction / LUMAPLAYER_TICKS_PER_SECOND + (roundUp && fraction % LUMAPLAYER_TICKS_PER_SECOND != 0 ? 1 : 0);
    return partial >= limit - whole ? limit : static_cast<size_t>(whole + partial);
}

PlayerEngine::PlayerEngine() :
m_threadId(0),
m_completionThreadId(0),
m_previewThreadId(0),
m_loopThreadId(0),
m_isInit(false),
m_audioRender(nullptr),
m_videoRender(nullptr),
m_state(LumaPlayerCoreStateClosed),
m_mediaSerial(0),
m_position100ns(0),
m_ratePermille(LumaPlayerCoreConfig().m_defaultRatePermille),
m_enableAutoReplay(true),
m_loopBoundaryEnabled(false),
m_videoReader(nullptr),
m_audioReader(nullptr),
m_previewReader(nullptr),
m_hasPendingVideoFrame(false),
m_hasPendingAudioFrame(false),
m_videoEnded(false),
m_audioEnded(false),
m_videoFrameIndex(0),
m_decodeFloor100ns(0),
m_previewSerial(0),
m_loopVideoIndex(0),
m_loopAudioIndex(0)
{
	m_snapshot.reset();
}

PlayerEngine::~PlayerEngine()
{
	uninit();
}

LumaPlayerCoreResult PlayerEngine::init(const LumaPlayerCoreConfig& config)
{
	if (config.m_controlCommandTimeoutMs <= 0 || config.m_openCommandTimeoutMs <= 0 || config.m_idleWaitMs <= 0 || config.m_videoEarlyTolerance100ns < 0 || config.m_audioLead100ns < 0 || config.m_loopBufferMaxBytes <= 0 || config.m_loopBufferMaxBytes > 1073741824 ||
		config.m_defaultFrameDuration100ns <= 0 || config.m_minRatePermille <= 0 ||
		config.m_maxRatePermille < config.m_minRatePermille ||
		config.m_defaultRatePermille < config.m_minRatePermille || config.m_defaultRatePermille > config.m_maxRatePermille)
	{
		return LumaPlayerCoreResultInvalidParam;
	}
	std::shared_ptr<PlayerWorkerTask> workerTask;
	std::shared_ptr<PlayerPreviewTask> previewTask;
	uint32_t threadId = 0;
	uint32_t previewThreadId = 0;
	uint32_t loopThreadId = 0;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_isInit)
		{
			return LumaPlayerCoreResultSuccess;
		}
		m_config = config;
	}

	try
	{
		workerTask = std::make_shared<PlayerWorkerTask>(this);
		previewTask = std::make_shared<PlayerPreviewTask>(this);
	}
	catch (...)
	{
		setLastError(LumaPlayerCoreResultWorkerAllocationFailed);
		return LumaPlayerCoreResultWorkerAllocationFailed;
	}

	try
	{
		threadId = CTaskThreadManager::Instance().Init();
		previewThreadId = CTaskThreadManager::Instance().Init();
		loopThreadId = CTaskThreadManager::Instance().Init();
	}
	catch (...)
	{
		if (threadId != 0)
		{
			CTaskThreadManager::Instance().Uninit(threadId);
		}
		if (previewThreadId != 0)
		{
			CTaskThreadManager::Instance().Uninit(previewThreadId);
		}
		if (loopThreadId != 0)
		{
			CTaskThreadManager::Instance().Uninit(loopThreadId);
		}
		setLastError(LumaPlayerCoreResultThreadInitException);
		return LumaPlayerCoreResultThreadInitException;
	}
	if (threadId == 0 || previewThreadId == 0 || loopThreadId == 0)
	{
		if (threadId != 0)
		{
			CTaskThreadManager::Instance().Uninit(threadId);
		}
		if (previewThreadId != 0)
		{
			CTaskThreadManager::Instance().Uninit(previewThreadId);
		}
		if (loopThreadId != 0)
		{
			CTaskThreadManager::Instance().Uninit(loopThreadId);
		}
		setLastError(LumaPlayerCoreResultThreadInitFailed);
		return LumaPlayerCoreResultThreadInitFailed;
	}

	std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
	std::shared_ptr<CTaskThread> previewThread = CTaskThreadManager::Instance().GetThreadInterface(previewThreadId);
	if (thread == nullptr || previewThread == nullptr)
	{
		CTaskThreadManager::Instance().Uninit(loopThreadId);
		CTaskThreadManager::Instance().Uninit(previewThreadId);
		CTaskThreadManager::Instance().Uninit(threadId);
		setLastError(LumaPlayerCoreResultThreadUnavailable);
		return LumaPlayerCoreResultThreadUnavailable;
	}

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_workerTask = workerTask;
		m_previewTask = previewTask;
		m_threadId = threadId;
		m_previewThreadId = previewThreadId;
		m_loopThreadId = loopThreadId;
		m_ratePermille = m_config.m_defaultRatePermille;
		m_clock.setRate(m_ratePermille);
		m_mediaInfo.m_defaultVideoFrameDuration100ns = m_config.m_defaultFrameDuration100ns;
		m_snapshot.m_ratePermille = m_ratePermille;
		m_snapshot.m_mediaInfo.m_defaultVideoFrameDuration100ns = m_config.m_defaultFrameDuration100ns;
		m_isInit = true;
	}
	try
	{
		previewThread->PostTask(previewTask, 1);
		thread->PostTask(workerTask, 1);
	}
	catch (...)
	{
		uninit();
		setLastError(LumaPlayerCoreResultCommandQueueFailed);
		return LumaPlayerCoreResultCommandQueueFailed;
	}
    try
    {
        m_completionTask = std::make_shared<PlayerCompletionTask>();
        m_completionThreadId = CTaskThreadManager::Instance().Init();
        std::shared_ptr<CTaskThread> completionThread = CTaskThreadManager::Instance().GetThreadInterface(m_completionThreadId);
        if (completionThread == nullptr)
        {
            uninit();
            return LumaPlayerCoreResultThreadUnavailable;
        }
        completionThread->PostTask(m_completionTask, 1);
    }
    catch (...)
    {
        uninit();
        return LumaPlayerCoreResultThreadInitException;
    }
    return LumaPlayerCoreResultSuccess;
}

void PlayerEngine::uninit()
{
    std::unique_lock<std::mutex> asyncSubmitLock(m_asyncSubmitMutex);
    std::unique_lock<std::mutex> submitLock(m_submitMutex);
	uint32_t threadId = 0;
	uint32_t previewThreadId = 0;
	uint32_t loopThreadId = 0;
	std::shared_ptr<PlayerWorkerTask> workerTask;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_isInit && m_threadId == 0 && m_previewThreadId == 0 && m_loopThreadId == 0 && m_completionThreadId == 0)
		{
			return;
		}
		threadId = m_threadId;
		previewThreadId = m_previewThreadId;
		loopThreadId = m_loopThreadId;
		workerTask = m_workerTask;
		m_threadId = 0;
		m_previewThreadId = 0;
		m_loopThreadId = 0;
		m_isInit = false;
	}
    submitLock.unlock();
    asyncSubmitLock.unlock();
    nextPreviewSerial();
    if (workerTask != nullptr)
    {
        workerTask->StopTask();
    }
    if (threadId != 0)
    {
        CTaskThreadManager::Instance().Uninit(threadId);
    }
    if (previewThreadId != 0)
    {
        CTaskThreadManager::Instance().Uninit(previewThreadId);
    }
    if (loopThreadId != 0)
    {
        CTaskThreadManager::Instance().Uninit(loopThreadId);
    }
	closeMediaInternal();
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_workerTask.reset();
		m_previewTask.reset();
		m_workerStdThreadId = std::thread::id();
		m_previewStdThreadId = std::thread::id();

	}
    if (m_completionThreadId != 0)
    {
        m_completionTask->StopTask();
        CTaskThreadManager::Instance().Uninit(m_completionThreadId);
        m_completionThreadId = 0;
        // 处理线程尚未运行就被回收的极快关闭，所有生产者已结束
        m_completionTask->DoTask();
    }
    m_completionTask.reset();
}

bool PlayerEngine::isInit() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_isInit;
}

void PlayerEngine::setAudioRender(IAudioRender* audioRender)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_audioRender = audioRender;
}

void PlayerEngine::setVideoRender(IVideoRender* videoRender)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_videoRender = videoRender;
}

LumaPlayerCoreResult PlayerEngine::openMedia(const std::string& filePath)
{
	if (!isInit())
	{
		return LumaPlayerCoreResultNotInit;
	}

	if (filePath.empty())
	{
		setLastError(LumaPlayerCoreResultInvalidParam);
		return LumaPlayerCoreResultInvalidParam;
	}
	PlayerCommand command;
	command.m_type = PlayerCommandOpen;
	command.m_filePath = filePath;
	nextPreviewSerial();
	return postSyncCommand(command, m_config.m_openCommandTimeoutMs);
}

LumaPlayerCoreResult PlayerEngine::openMediaAsync(const std::string& filePath)
{
	if (filePath.empty())
	{
		setLastError(LumaPlayerCoreResultInvalidParam);
		return LumaPlayerCoreResultInvalidParam;
	}
	PlayerCommand command;
	command.m_type = PlayerCommandOpen;
	command.m_filePath = filePath;
	nextPreviewSerial();
	return postCommand(command);
}

void PlayerEngine::closeMedia()
{
	if (!isInit())
	{
		return;
	}
	PlayerCommand command;
	command.m_type = PlayerCommandClose;
	nextPreviewSerial();
	postSyncCommand(command, m_config.m_controlCommandTimeoutMs);
}

LumaPlayerCoreResult PlayerEngine::play()
{
	if (!isInit())
	{
		return LumaPlayerCoreResultNotInit;
	}

	PlayerCommand command;
	command.m_type = PlayerCommandPlay;
	nextPreviewSerial();
	return postSyncCommand(command, m_config.m_controlCommandTimeoutMs);
}

LumaPlayerCoreResult PlayerEngine::playAsync()
{
	PlayerCommand command;
	command.m_type = PlayerCommandPlay;
	nextPreviewSerial();
	return postCommand(command);
}

LumaPlayerCoreResult PlayerEngine::pause()
{
	if (!isInit())
	{
		return LumaPlayerCoreResultNotInit;
	}

	PlayerCommand command;
	command.m_type = PlayerCommandPause;
	nextPreviewSerial();
	return postSyncCommand(command, m_config.m_controlCommandTimeoutMs);
}

LumaPlayerCoreResult PlayerEngine::pauseAsync()
{
	PlayerCommand command;
	command.m_type = PlayerCommandPause;
	nextPreviewSerial();
	return postCommand(command);
}

LumaPlayerCoreResult PlayerEngine::stop()
{
	if (!isInit())
	{
		return LumaPlayerCoreResultNotInit;
	}

	PlayerCommand command;
	command.m_type = PlayerCommandStop;
	nextPreviewSerial();
	return postSyncCommand(command, m_config.m_controlCommandTimeoutMs);
}

LumaPlayerCoreResult PlayerEngine::seekTo(int64_t position100ns, bool keepPlayState)
{
	if (!isInit())
	{
		return LumaPlayerCoreResultNotInit;
	}

	PlayerCommand command;
	command.m_type = PlayerCommandSeek;
	command.m_position100ns = position100ns;
	command.m_keepPlayState = keepPlayState;
	command.m_serial = nextPreviewSerial();
	return postSyncCommand(command, m_config.m_controlCommandTimeoutMs);
}

LumaPlayerCoreResult PlayerEngine::seekToAsync(int64_t position100ns, bool keepPlayState)
{
	PlayerCommand command;
	command.m_type = PlayerCommandSeek;
	command.m_position100ns = position100ns;
	command.m_keepPlayState = keepPlayState;
	command.m_serial = nextPreviewSerial();
	return postCommand(command);
}

LumaPlayerCoreResult PlayerEngine::previewFrame(int64_t position100ns)
{
	if (!isInit())
	{
		return LumaPlayerCoreResultNotInit;
	}

	PlayerCommand command;
	command.m_type = PlayerCommandPreview;
	command.m_position100ns = position100ns;
	command.m_keepPlayState = false;
	command.m_serial = nextPreviewSerial();
	return postSyncCommand(command, m_config.m_controlCommandTimeoutMs);
}

LumaPlayerCoreResult PlayerEngine::previewFrameAsync(int64_t position100ns)
{
	uint64_t requestSerial = nextPreviewSerial();
	return postPreviewTask(position100ns, requestSerial);
}

LumaPlayerCoreResult PlayerEngine::setLoopAByCurrentFrame()
{
	if (!isInit())
	{
		return LumaPlayerCoreResultNotInit;
	}

	PlayerCommand command;
	command.m_type = PlayerCommandSetLoopA;
	return postSyncCommand(command, m_config.m_controlCommandTimeoutMs);
}

LumaPlayerCoreResult PlayerEngine::setLoopAAtPosition(int64_t position100ns)
{
	if (!isInit())
	{
		return LumaPlayerCoreResultNotInit;
	}

	PlayerCommand command;
	command.m_type = PlayerCommandSetLoopA;
	command.m_position100ns = position100ns;
	command.m_usePosition = true;
	return postSyncCommand(command, m_config.m_controlCommandTimeoutMs);
}

LumaPlayerCoreResult PlayerEngine::setLoopAAtPositionAsync(int64_t position100ns)
{
	PlayerCommand command;
	command.m_type = PlayerCommandSetLoopA;
	command.m_position100ns = position100ns;
	command.m_usePosition = true;
	return postCommand(command);
}

LumaPlayerCoreResult PlayerEngine::setLoopBByCurrentFrame()
{
	if (!isInit())
	{
		return LumaPlayerCoreResultNotInit;
	}

	PlayerCommand command;
	command.m_type = PlayerCommandSetLoopB;
	return postSyncCommand(command, m_config.m_controlCommandTimeoutMs);
}

LumaPlayerCoreResult PlayerEngine::setLoopBAtPosition(int64_t position100ns)
{
	if (!isInit())
	{
		return LumaPlayerCoreResultNotInit;
	}

	PlayerCommand command;
	command.m_type = PlayerCommandSetLoopB;
	command.m_position100ns = position100ns;
	command.m_usePosition = true;
	return postSyncCommand(command, m_config.m_controlCommandTimeoutMs);
}

LumaPlayerCoreResult PlayerEngine::setLoopBAtPositionAsync(int64_t position100ns)
{
	PlayerCommand command;
	command.m_type = PlayerCommandSetLoopB;
	command.m_position100ns = position100ns;
	command.m_usePosition = true;
	return postCommand(command);
}

LumaPlayerCoreResult PlayerEngine::clearLoop()
{
	if (!isInit())
	{
		return LumaPlayerCoreResultNotInit;
	}

	PlayerCommand command;
	command.m_type = PlayerCommandClearLoop;
	return postSyncCommand(command, m_config.m_controlCommandTimeoutMs);
}

LumaPlayerCoreResult PlayerEngine::clearLoopAsync()
{
	PlayerCommand command;
	command.m_type = PlayerCommandClearLoop;
	return postCommand(command);
}

LumaPlayerCoreResult PlayerEngine::moveLoopPoint(LumaPlayerCoreLoopPointType pointType, int32_t frameOffset)
{
	if (!isInit())
	{
		return LumaPlayerCoreResultNotInit;
	}

	PlayerCommand command;
	command.m_type = PlayerCommandMoveLoopPoint;
	command.m_loopPointType = pointType;
	command.m_frameOffset = frameOffset;
	command.m_serial = nextPreviewSerial();
	return postSyncCommand(command, m_config.m_controlCommandTimeoutMs);
}

LumaPlayerCoreResult PlayerEngine::moveLoopPointAsync(LumaPlayerCoreLoopPointType pointType, int32_t frameOffset)
{
	PlayerCommand command;
	command.m_type = PlayerCommandMoveLoopPoint;
	command.m_loopPointType = pointType;
	command.m_frameOffset = frameOffset;
	command.m_serial = nextPreviewSerial();
	return postCommand(command);
}

LumaPlayerCoreResult PlayerEngine::setPlaybackRatePermille(int32_t ratePermille)
{
	if (!isInit())
	{
		return LumaPlayerCoreResultNotInit;
	}

	PlayerCommand command;
	command.m_type = PlayerCommandSetRate;
	command.m_ratePermille = ratePermille;
	nextPreviewSerial();
	return postSyncCommand(command, m_config.m_controlCommandTimeoutMs);
}

LumaPlayerCoreResult PlayerEngine::setPlaybackRatePermilleAsync(int32_t ratePermille)
{
	PlayerCommand command;
	command.m_type = PlayerCommandSetRate;
	command.m_ratePermille = ratePermille;
	nextPreviewSerial();
	return postCommand(command);
}

LumaPlayerSnapshot PlayerEngine::snapshot() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_snapshot;
}

LumaPlayerMediaInfo PlayerEngine::mediaInfo() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_snapshot.m_mediaInfo;
}

std::string PlayerEngine::lastError() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_lastError;
}

LumaPlayerCoreResult PlayerEngine::previewFrameFromTask(int64_t position100ns, uint64_t requestSerial, const std::atomic<bool>* exitFlag)
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_previewStdThreadId = std::this_thread::get_id();
	}
	LumaPlayerCoreResult result = LumaPlayerCoreResultPreviewDecodeFailed;
	try
	{
		result = previewFrameInternal(position100ns, requestSerial, exitFlag, false);
	}
	catch (...)
	{
		// 异常不能离开任务入口，下一次预览仍可继续
		result = LumaPlayerCoreResultPreviewDecodeFailed;
	}
	if (requestSerial == m_previewSerial.load())
	{
		setLastError(result);
	}
	return result;
}

LumaPlayerCoreResult PlayerEngine::postSyncCommand(PlayerCommand& command, int32_t timeoutMs)
{
	bool isWorkerThread = false;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!m_isInit)
		{
			return LumaPlayerCoreResultNotInit;
		}
		isWorkerThread = std::this_thread::get_id() == m_workerStdThreadId ||
			std::this_thread::get_id() == m_previewStdThreadId;
	}

	if (isWorkerThread)
	{
		// 渲染回调内同步重入会破坏正在读取或发布的帧，只允许提交异步意图
		return LumaPlayerCoreResultStateError;
	}

	std::shared_ptr<PlayerSyncResult> syncResult;
	PlayerSyncResult* rawResult = new(std::nothrow) PlayerSyncResult();
	if (rawResult == nullptr)
	{
		setLastError(LumaPlayerCoreResultResultAllocationFailed);
		return LumaPlayerCoreResultResultAllocationFailed;
	}
	try
	{
		syncResult.reset(rawResult);
	}
	catch (...)
	{
		setLastError(LumaPlayerCoreResultResultOwnershipFailed);
		return LumaPlayerCoreResultResultOwnershipFailed;
	}
	command.m_syncResult = syncResult;
	LumaPlayerCoreResult postResult = postCommand(command);
	if (postResult != LumaPlayerCoreResultSuccess)
	{
		return postResult;
	}
	if (!syncResult->m_semaphore.wait(timeoutMs))
	{
		setLastError(LumaPlayerCoreResultTimeout);
		return LumaPlayerCoreResultTimeout;
	}
	if (!syncResult->m_errorText.empty())
	{
		setLastError(syncResult->m_result);
	}
	return syncResult->m_result;
}

LumaPlayerCoreResult PlayerEngine::postCommand(const PlayerCommand& command)
{
    std::lock_guard<std::mutex> submitLock(m_submitMutex);
    uint32_t threadId = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_isInit)
        {
            return LumaPlayerCoreResultNotInit;
        }
        threadId = m_threadId;
    }
    try
    {
        std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
        if (thread == nullptr)
        {
            return LumaPlayerCoreResultThreadUnavailable;
        }
        if (thread->GetWaitTaskCount() >= PlayerCommandTask::kMaxPendingCommands)
        {
            return LumaPlayerCoreResultCommandQueueFailed;
        }
        std::shared_ptr<PlayerCommandTask> task = std::make_shared<PlayerCommandTask>(this, command);
        if (command.m_syncResult == nullptr && (command.m_type == PlayerCommandMoveLoopPoint || command.m_type == PlayerCommandSeek || command.m_type == PlayerCommandPreview))
        {
            thread->StopTask(static_cast<int32_t>(command.m_type) + PlayerCommandTask::kTaskIdBase);
        }
        if (command.m_type != PlayerCommandMoveLoopPoint)
        {
            thread->StopTask(static_cast<int32_t>(PlayerCommandMoveLoopPoint) + PlayerCommandTask::kTaskIdBase);
        }
        thread->StopTask(PlayerWorkerTask::kTaskId);
        thread->PostTask(task, PlayerCommandTask::kPriority);
        return LumaPlayerCoreResultSuccess;
    }
    catch (...)
    {
        return LumaPlayerCoreResultCommandQueueFailed;
    }
}

void PlayerEngine::cancelLoopPointMove()
{
    {
        std::lock_guard<std::mutex> submitLock(m_submitMutex);
        uint32_t threadId = 0;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            threadId = m_threadId;
        }
        std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
        if (thread != nullptr)
        {
            thread->StopTask(static_cast<int32_t>(PlayerCommandMoveLoopPoint) + PlayerCommandTask::kTaskIdBase);
        }
    }
    // 即使移动尚未进入DoTask就被取消，也通过控制线程恢复原来的播放调度
    PlayerCommand command;
    command.m_type = PlayerCommandNone;
    postCommand(command);
}

void PlayerEngine::executeCommandTask(const PlayerCommand& command, const std::atomic<bool>* exitFlag)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_workerStdThreadId = std::this_thread::get_id();
    }
    try
    {
        handleCommand(command, exitFlag);
    }
    catch (...)
    {
        // 异常不能让GUI永久停在Opening/Seeking，也不能继续使用失配的播放时钟。
        m_clock.pause();
        m_state = LumaPlayerCoreStateError;
        updateSnapshot(m_state, m_position100ns);
        setLastError(LumaPlayerCoreResultInternalError);
        completeSyncCommand(command, LumaPlayerCoreResultInternalError);
    }
    try
    {
        resumePlaybackTask();
    }
    catch (...)
    {
        m_state = LumaPlayerCoreStateError;
        updateSnapshot(m_state, m_position100ns);
        setLastError(LumaPlayerCoreResultWorkerAllocationFailed);
    }
}

void PlayerEngine::resumePlaybackTask()
{
    std::lock_guard<std::mutex> submitLock(m_submitMutex);
    uint32_t threadId = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_isInit || m_state != LumaPlayerCoreStatePlaying)
        {
            return;
        }
        threadId = m_threadId;
    }
    std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
    if (thread != nullptr)
    {
        std::shared_ptr<PlayerWorkerTask> task = std::make_shared<PlayerWorkerTask>(this);
        thread->StopTask(PlayerWorkerTask::kTaskId);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_workerTask = task;
        }
        thread->PostTask(task, 1);
    }
}

LumaPlayerCoreResult PlayerEngine::postPreviewTask(int64_t position100ns, uint64_t requestSerial)
{
	std::shared_ptr<PlayerPreviewTask> previewTask;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!m_isInit || m_previewTask == nullptr)
		{
			return LumaPlayerCoreResultNotInit;
		}
		previewTask = m_previewTask;
	}
	previewTask->request(position100ns, requestSerial);
	return LumaPlayerCoreResultSuccess;
}

uint64_t PlayerEngine::nextPreviewSerial()
{
	return ++m_previewSerial;
}

void PlayerEngine::completeSyncCommand(const PlayerCommand& command, LumaPlayerCoreResult result)
{
    if (command.m_completion != nullptr)
    {
        command.m_completion->m_delivery->m_completion.m_result = result;
        command.m_completion->m_delivery->m_completion.m_snapshot = snapshot();
    }
	if (command.m_syncResult == nullptr)
	{
		return;
	}
	command.m_syncResult->m_result = result;
	command.m_syncResult->m_snapshot = snapshot();
	command.m_syncResult->m_errorText = lastError();
	command.m_syncResult->m_semaphore.signal();
}

void PlayerEngine::handleCommand(const PlayerCommand& command, const std::atomic<bool>* exitFlag)
{
	LumaPlayerCoreResult result = LumaPlayerCoreResultSuccess;
	switch (command.m_type)
	{
	case PlayerCommandNone:
	{
		break;
	}
	case PlayerCommandOpen:
        result = openMediaInternal(command.m_filePath, exitFlag);
        if (result != LumaPlayerCoreResultSuccess && result != LumaPlayerCoreResultCanceled)
		{
			m_state = LumaPlayerCoreStateError;
			updateSnapshot(m_state, m_position100ns);
		}
		break;
	case PlayerCommandClose:
		closeMediaInternal();
		break;
	case PlayerCommandPlay:
		if (m_videoReader == nullptr && m_audioReader == nullptr)
		{
			result = LumaPlayerCoreResultNotOpen;
			break;
		}
        if (m_position100ns < 0 || m_position100ns >= m_mediaInfo.m_duration100ns)
		{
            result = seekInternal(0, true, false);
		}
		else
		{
			IAudioRender* audioRender = nullptr;
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				audioRender = m_audioRender;
			}
			if (audioRender != nullptr)
			{
				audioRender->resumeAudio();
			}
			result = primePlayback(m_position100ns);
			if (result != LumaPlayerCoreResultSuccess)
			{
				break;
			}
			m_clock.start(m_position100ns, m_ratePermille);
			m_state = LumaPlayerCoreStatePlaying;
			updateSnapshot(m_state, m_position100ns);
		}
		break;
	case PlayerCommandPause:
		if (m_videoReader == nullptr && m_audioReader == nullptr)
		{
			m_state = LumaPlayerCoreStateClosed;
			updateSnapshot(m_state, m_position100ns);
			break;
		}
		if (m_state == LumaPlayerCoreStatePlaying)
		{
			m_position100ns = clampPosition(m_clock.pause());
		}
		{
			IAudioRender* audioRender = nullptr;
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				audioRender = m_audioRender;
			}
			if (audioRender != nullptr)
			{
				audioRender->pauseAudio();
			}
		}
		m_state = LumaPlayerCoreStatePaused;
		updateSnapshot(m_state, m_position100ns);
		break;
	case PlayerCommandStop:
		if (m_videoReader == nullptr && m_audioReader == nullptr)
		{
			result = LumaPlayerCoreResultNotOpen;
			break;
		}
        result = seekInternal(0, false, true, exitFlag);
        if (result == LumaPlayerCoreResultSuccess)
        {
            m_state = LumaPlayerCoreStateStopped;
            updateSnapshot(m_state, m_position100ns);
        }
		break;
	case PlayerCommandSeek:
	{
		uint64_t currentSerial = m_previewSerial.load();
		if (command.m_serial != currentSerial)
		{
			result = LumaPlayerCoreResultCanceled;
			break;
		}
        {
            // 请求序号已淘汰旧预览，等待已进入的发布退出，避免旧画面覆盖最终定位。
            std::lock_guard<std::mutex> previewLock(m_previewReaderMutex);
        }
        if (command.m_serial != m_previewSerial.load())
        {
            result = LumaPlayerCoreResultCanceled;
            break;
        }
        result = seekInternal(command.m_position100ns, command.m_keepPlayState, !command.m_keepPlayState, exitFlag);
		break;
	}
	case PlayerCommandPreview:
	{
		uint64_t currentSerial = m_previewSerial.load();
		if (command.m_serial != currentSerial)
		{
			result = LumaPlayerCoreResultCanceled;
			break;
		}
        result = previewFrameInternal(command.m_position100ns, command.m_serial, exitFlag, true);
		break;
	}
	case PlayerCommandSetLoopA:
	{
		if (!m_mediaInfo.m_hasVideo || m_videoReader == nullptr)
		{
			result = LumaPlayerCoreResultNotOpen;
			break;
		}
		LumaPlayerLoopPointInfo currentPoint;
		int64_t targetPosition100ns = clampPosition(command.m_usePosition ? command.m_position100ns : m_position100ns);
		if (!command.m_usePosition)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			currentPoint = m_currentVideoFrame;
			if (currentPoint.m_isSet)
			{
				targetPosition100ns = currentPoint.m_frameStart100ns;
			}
		}
		if (!currentPoint.m_isSet)
		{
			currentPoint.m_isSet = true;
			currentPoint.m_frameStart100ns = targetPosition100ns;
			currentPoint.m_frameEnd100ns = clampPosition(m_mediaInfo.m_defaultVideoFrameDuration100ns > (std::numeric_limits<int64_t>::max)() - targetPosition100ns ?
				(std::numeric_limits<int64_t>::max)() : targetPosition100ns + m_mediaInfo.m_defaultVideoFrameDuration100ns);
			currentPoint.m_frameIndex = -1;
		}
        bool refined = refineLoopPointByPreview(&currentPoint, targetPosition100ns, exitFlag);
        if (exitFlag != nullptr && exitFlag->load())
        {
            result = LumaPlayerCoreResultCanceled;
            break;
        }
		if (m_mediaInfo.m_hasVideo && !refined)
		{
			result = LumaPlayerCoreResultLoopAFrameUnavailable;
			setLastError(LumaPlayerCoreResultLoopAFrameUnavailable);
			break;
		}
		if (m_loopRange.m_bPoint.m_isSet && currentPoint.m_frameStart100ns >= m_loopRange.m_bPoint.m_frameEnd100ns)
		{
			result = LumaPlayerCoreResultLoopAOutOfRange;
			setLastError(LumaPlayerCoreResultLoopAOutOfRange);
			break;
		}
		m_loopRange.m_aPoint = currentPoint;
		updateSnapshot(m_state, m_position100ns);
		break;
	}
	case PlayerCommandSetLoopB:
	{
		if (!m_mediaInfo.m_hasVideo || m_videoReader == nullptr)
		{
			result = LumaPlayerCoreResultNotOpen;
			break;
		}
		LumaPlayerLoopPointInfo currentPoint;
		int64_t targetPosition100ns = clampPosition(command.m_usePosition ? command.m_position100ns : m_position100ns);
		if (!command.m_usePosition)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			currentPoint = m_currentVideoFrame;
			if (currentPoint.m_isSet)
			{
				targetPosition100ns = currentPoint.m_frameStart100ns;
			}
		}
		if (!currentPoint.m_isSet)
		{
			currentPoint.m_isSet = true;
			currentPoint.m_frameStart100ns = targetPosition100ns;
			currentPoint.m_frameEnd100ns = clampPosition(m_mediaInfo.m_defaultVideoFrameDuration100ns > (std::numeric_limits<int64_t>::max)() - targetPosition100ns ?
				(std::numeric_limits<int64_t>::max)() : targetPosition100ns + m_mediaInfo.m_defaultVideoFrameDuration100ns);
			currentPoint.m_frameIndex = -1;
		}
        bool refined = refineLoopPointByPreview(&currentPoint, targetPosition100ns, exitFlag);
        if (exitFlag != nullptr && exitFlag->load())
        {
            result = LumaPlayerCoreResultCanceled;
            break;
        }
		if (m_mediaInfo.m_hasVideo && !refined)
		{
			result = LumaPlayerCoreResultLoopBFrameUnavailable;
			setLastError(LumaPlayerCoreResultLoopBFrameUnavailable);
			break;
		}
		if (!m_loopRange.m_aPoint.m_isSet)
		{
			m_loopRange.m_aPoint.m_isSet = true;
			m_loopRange.m_aPoint.m_frameStart100ns = 0;
            m_loopRange.m_aPoint.m_frameEnd100ns = (std::min<int64_t>)(m_mediaInfo.m_defaultVideoFrameDuration100ns, m_mediaInfo.m_duration100ns);
			m_loopRange.m_aPoint.m_frameIndex = 0;
            bool refinedA = refineLoopPointByPreview(&m_loopRange.m_aPoint, 0, exitFlag);
            if (exitFlag != nullptr && exitFlag->load())
            {
                m_loopRange.m_aPoint.reset();
                result = LumaPlayerCoreResultCanceled;
                break;
            }
			if (m_mediaInfo.m_hasVideo && !refinedA)
			{
				m_loopRange.m_aPoint.reset();
				result = LumaPlayerCoreResultLoopAutoAUnavailable;
				setLastError(LumaPlayerCoreResultLoopAutoAUnavailable);
				break;
			}
		}
		if (currentPoint.m_frameEnd100ns <= m_loopRange.m_aPoint.m_frameStart100ns)
		{
			result = LumaPlayerCoreResultLoopBOutOfRange;
			setLastError(LumaPlayerCoreResultLoopBOutOfRange);
			break;
		}
		m_loopRange.m_bPoint = currentPoint;
		updateSnapshot(m_state, m_position100ns);
		break;
	}
	case PlayerCommandClearLoop:
		m_loopRange.reset();
		updateSnapshot(m_state, m_position100ns);
		break;
	case PlayerCommandMoveLoopPoint:
	{
		if (command.m_loopPointType != LumaPlayerCoreLoopPointA && command.m_loopPointType != LumaPlayerCoreLoopPointB)
		{
			result = LumaPlayerCoreResultInvalidParam;
			break;
		}
		LumaPlayerLoopPointInfo originalPoint = command.m_loopPointType == LumaPlayerCoreLoopPointA ? m_loopRange.m_aPoint : m_loopRange.m_bPoint;
		if (!originalPoint.m_isSet)
		{
			result = LumaPlayerCoreResultLoopPointNotSet;
			setLastError(LumaPlayerCoreResultLoopPointNotSet);
			break;
		}
		if (command.m_frameOffset == 0)
		{
			break;
		}
        // 端点编辑始终在暂停状态进行，不能由播放时钟覆盖预览位置
        PlayerCommand pauseCommand;
        pauseCommand.m_type = PlayerCommandPause;
        handleCommand(pauseCommand);
		LumaPlayerLoopPointInfo candidatePoint = originalPoint;
		LumaPlayerVideoFrame candidateFrame;
		int32_t direction = command.m_frameOffset > 0 ? 1 : -1;
		int64_t stepCount = command.m_frameOffset > 0 ? command.m_frameOffset : -static_cast<int64_t>(command.m_frameOffset);
		for (int64_t stepIndex = 0; stepIndex < stepCount; ++stepIndex)
		{
			result = readAdjacentVideoFrame(candidatePoint, direction, &candidateFrame, command.m_serial, exitFlag);
			if (result != LumaPlayerCoreResultSuccess)
			{
				break;
			}
			LumaPlayerLoopPointInfo nextPoint;
			nextPoint.m_isSet = true;
			nextPoint.m_frameStart100ns = candidateFrame.m_timestamp100ns;
			nextPoint.m_frameEnd100ns = clampPosition(candidateFrame.endTime100ns());
			nextPoint.m_frameIndex = candidateFrame.m_frameIndex;
			if (command.m_loopPointType == LumaPlayerCoreLoopPointA && m_loopRange.m_bPoint.m_isSet && nextPoint.m_frameStart100ns >= m_loopRange.m_bPoint.m_frameEnd100ns)
			{
				result = LumaPlayerCoreResultLoopAOutOfRange;
				break;
			}
			if (command.m_loopPointType == LumaPlayerCoreLoopPointB && m_loopRange.m_aPoint.m_isSet && nextPoint.m_frameEnd100ns <= m_loopRange.m_aPoint.m_frameStart100ns)
			{
				result = LumaPlayerCoreResultLoopBOutOfRange;
				break;
			}
			candidatePoint = nextPoint;
		}
		if (result == LumaPlayerCoreResultSuccess)
		{
			if ((exitFlag != nullptr && exitFlag->load()) || command.m_serial != m_previewSerial.load())
			{
				result = LumaPlayerCoreResultCanceled;
				break;
			}
            // 同时重置音视频读取位置和时钟，再次播放不能沿用编辑前的缓存
            result = seekInternal(candidatePoint.m_frameStart100ns, false, false);
            if (result != LumaPlayerCoreResultSuccess)
            {
                break;
            }
            // 取消检查已在定位之前完成，定位后必须完成本帧提交，避免进度和画面分离
			result = renderVideoFrame(candidateFrame, true);
			if (result != LumaPlayerCoreResultSuccess)
			{
				break;
			}
			if (command.m_loopPointType == LumaPlayerCoreLoopPointA)
			{
				m_loopRange.m_aPoint = candidatePoint;
			}
			else
			{
				m_loopRange.m_bPoint = candidatePoint;
			}
			updateSnapshot(m_state, m_position100ns);
		}
		else if (result != LumaPlayerCoreResultCanceled)
		{
            // 越界时先取得并显示原端点帧，取消预览不能提前改变播放位置
            LumaPlayerCoreResult previewResult = previewFrameInternal(originalPoint.m_frameStart100ns, command.m_serial, exitFlag, false);
            if (previewResult == LumaPlayerCoreResultSuccess)
            {
                seekInternal(originalPoint.m_frameStart100ns, false, false);
            }
			updateSnapshot(m_state, m_position100ns);
		}
		break;
	}
	case PlayerCommandSetRate:
	{
		const bool wasPlaying = m_state == LumaPlayerCoreStatePlaying;
		const int64_t currentPosition = clampPosition(m_clock.position100ns());
		m_ratePermille = PlayerEngineHelper::clampRate(command.m_ratePermille, m_config.m_minRatePermille, m_config.m_maxRatePermille);
		m_clock.setRate(m_ratePermille);
		if (m_videoReader != nullptr || m_audioReader != nullptr)
		{
			// 清空设备中旧倍率PCM，按同一媒体位置重新准备音视频
            result = seekInternal(currentPosition, wasPlaying, false, exitFlag);
		}
		updateSnapshot(m_state, m_position100ns);
		break;
	}
	default:
		result = LumaPlayerCoreResultInvalidParam;
		break;
	}
    if (result == LumaPlayerCoreResultSuccess)
    {
        if (command.m_type == PlayerCommandSetLoopA || command.m_type == PlayerCommandSetLoopB ||
            command.m_type == PlayerCommandClearLoop || command.m_type == PlayerCommandMoveLoopPoint)
        {
            // 编辑端点是建立新的循环意图，不是主动跳出B；当前位置已越界时从A接续。
            const bool validLoop = m_loopRange.isValid();
            const int64_t loopPosition = validLoop && m_position100ns >= m_loopRange.end100ns() ?
                m_loopRange.start100ns() : m_position100ns;
            m_loopBoundaryEnabled = validLoop;
            // 旧B可能让读帧提前停止；新边界由下一播放步重新检查，保留待呈现帧。
            m_videoEnded = m_videoReader == nullptr;
            m_audioEnded = m_audioReader == nullptr;
            if (command.m_type != PlayerCommandMoveLoopPoint && m_state != LumaPlayerCoreStatePlaying &&
                (m_videoReader != nullptr || m_audioReader != nullptr))
            {
                // 暂停编辑保持画面与实际帧一致；播放中只更新边界，不重启时钟和声卡。
                result = seekInternal(loopPosition, false, true, exitFlag);
            }
        }
        if (result == LumaPlayerCoreResultSuccess)
        {
            refreshLoopBuffer();
        }
	}
	setLastError(result);
	completeSyncCommand(command, result);
}

LumaPlayerCoreResult PlayerEngine::openMediaInternal(const std::string& filePath, const std::atomic<bool>* exitFlag)
{
    if (filePath.empty())
    {
        setLastError(LumaPlayerCoreResultInvalidParam);
        return LumaPlayerCoreResultInvalidParam;
    }
    if (exitFlag != nullptr && exitFlag->load())
    {
        closeMediaInternal();
        return LumaPlayerCoreResultCanceled;
    }
    closeMediaInternal();
    m_state = LumaPlayerCoreStateOpening;
    updateSnapshot(m_state, 0);

    FFmpegCppPlaybackOpenOption infoOption;
    infoOption.decodeVideo = true;
    infoOption.decodeAudio = true;
    infoOption.outputChannels = 2;
    infoOption.outputBitsPerSample = 16;
    infoOption.fallbackVideoFrameDuration100ns = m_config.m_defaultFrameDuration100ns;
    FFmpegCppPlaybackReader infoReader;
    if (!infoReader.open(filePath, infoOption))
    {
        setLastError(LumaPlayerCoreResultMediaInfoOpenFailed);
        return LumaPlayerCoreResultMediaInfoOpenFailed;
    }

    if (exitFlag != nullptr && exitFlag->load())
    {
        closeMediaInternal();
        return LumaPlayerCoreResultCanceled;
    }
    m_mediaInfo.reset();
    m_mediaInfo.m_filePath = filePath;
    m_mediaInfo.m_duration100ns = infoReader.duration100ns();
    m_mediaInfo.m_defaultVideoFrameDuration100ns = infoReader.videoFrameDuration100ns();
    if (m_mediaInfo.m_defaultVideoFrameDuration100ns <= 0)
    {
        m_mediaInfo.m_defaultVideoFrameDuration100ns = m_config.m_defaultFrameDuration100ns;
    }
    m_mediaInfo.m_hasVideo = infoReader.hasVideo();
    m_mediaInfo.m_hasAudio = infoReader.hasAudio();
    infoReader.getVideoSize(&m_mediaInfo.m_videoFormat.m_width, &m_mediaInfo.m_videoFormat.m_height);
    infoReader.getAudioFormat(&m_mediaInfo.m_audioFormat.m_sampleRate, &m_mediaInfo.m_audioFormat.m_channels, &m_mediaInfo.m_audioFormat.m_bitsPerSample);

    if (m_mediaInfo.m_duration100ns <= 0)
    {
        m_mediaInfo.m_duration100ns = m_mediaInfo.m_defaultVideoFrameDuration100ns;
    }

    if (m_mediaInfo.m_hasVideo)
    {
        m_videoReader = new(std::nothrow) FFmpegCppPlaybackReader();
        std::unique_ptr<FFmpegCppPlaybackReader> previewReader(new(std::nothrow) FFmpegCppPlaybackReader());
        if (m_videoReader == nullptr || previewReader == nullptr)
        {
            previewReader.reset();
            setLastError(LumaPlayerCoreResultVideoAllocationFailed);
            closeMediaInternal();
            return LumaPlayerCoreResultVideoAllocationFailed;
        }
        FFmpegCppPlaybackOpenOption videoOption;
        videoOption.decodeVideo = true;
        videoOption.decodeAudio = false;
        videoOption.fallbackVideoFrameDuration100ns = m_mediaInfo.m_defaultVideoFrameDuration100ns;
        if (!m_videoReader->open(filePath, videoOption))
        {
            previewReader.reset();
            setLastError(LumaPlayerCoreResultVideoOpenFailed);
            closeMediaInternal();
            return LumaPlayerCoreResultVideoOpenFailed;
        }
        if (!previewReader->open(filePath, videoOption))
        {

            previewReader.reset();
            setLastError(LumaPlayerCoreResultPreviewOpenFailed);
            closeMediaInternal();
            return LumaPlayerCoreResultPreviewOpenFailed;
        }
        {
            std::lock_guard<std::mutex> previewLock(m_previewReaderMutex);
            PlayerEngineHelper::deleteReader(&m_previewReader);
            m_previewReader = previewReader.release();
        }
        IVideoRender* videoRender = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            videoRender = m_videoRender;
        }
        if (videoRender != nullptr && !videoRender->openVideo(m_mediaInfo.m_videoFormat))
        {
            setLastError(LumaPlayerCoreResultVideoRenderOpenFailed);
            closeMediaInternal();
            return LumaPlayerCoreResultVideoRenderOpenFailed;
        }
    }

    if (exitFlag != nullptr && exitFlag->load())
    {
        closeMediaInternal();
        return LumaPlayerCoreResultCanceled;
    }
    if (m_mediaInfo.m_hasAudio)
    {
        m_audioReader = new(std::nothrow) FFmpegCppPlaybackReader();
        if (m_audioReader == nullptr)
        {
            setLastError(LumaPlayerCoreResultAudioAllocationFailed);
            closeMediaInternal();
            return LumaPlayerCoreResultAudioAllocationFailed;
        }
        FFmpegCppPlaybackOpenOption audioOption;
        audioOption.decodeVideo = false;
        audioOption.decodeAudio = true;
        audioOption.outputChannels = 2;
        audioOption.outputBitsPerSample = 16;
        if (!m_audioReader->open(filePath, audioOption))
        {
            setLastError(LumaPlayerCoreResultAudioOpenFailed);
            closeMediaInternal();
            return LumaPlayerCoreResultAudioOpenFailed;
        }
        else
        {
            m_audioReader->getAudioFormat(&m_mediaInfo.m_audioFormat.m_sampleRate, &m_mediaInfo.m_audioFormat.m_channels, &m_mediaInfo.m_audioFormat.m_bitsPerSample);
            IAudioRender* audioRender = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                audioRender = m_audioRender;
            }
            if (audioRender != nullptr)
            {
                if (!audioRender->openAudio(m_mediaInfo.m_audioFormat))
                {
                    setLastError(LumaPlayerCoreResultAudioRenderOpenFailed);
                    closeMediaInternal();
                    return LumaPlayerCoreResultAudioRenderOpenFailed;
                }
                audioRender->pauseAudio();
            }
        }
    }

    if (exitFlag != nullptr && exitFlag->load())
    {
        closeMediaInternal();
        return LumaPlayerCoreResultCanceled;
    }
    m_position100ns = 0;
    m_decodeFloor100ns = 0;
    m_ratePermille = m_config.m_defaultRatePermille;
    m_clock.seek(0);
    m_clock.setRate(m_ratePermille);
    m_loopRange.reset();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentVideoFrame.reset();
    }
    m_pendingVideoFrame.reset();
    m_pendingAudioFrame.reset();
    m_hasPendingVideoFrame = false;
    m_hasPendingAudioFrame = false;
    m_videoEnded = !m_mediaInfo.m_hasVideo;
    m_audioEnded = !m_mediaInfo.m_hasAudio;
    m_videoFrameIndex = 0;
    m_state = LumaPlayerCoreStatePaused;
    ++m_mediaSerial;

    if (m_mediaInfo.m_hasVideo)
    {
        LumaPlayerCoreResult previewResult = previewFrameInternal(0, 0, exitFlag, true);
        if (previewResult != LumaPlayerCoreResultSuccess)
        {
            closeMediaInternal();
            return previewResult;
        }
    }
    updateSnapshot(m_state, m_position100ns);
    return LumaPlayerCoreResultSuccess;
}

void PlayerEngine::closeMediaInternal()
{
    m_loopBoundaryEnabled = false;
	clearLoopBuffers();
	std::lock_guard<std::mutex> previewLock(m_previewReaderMutex);
	m_clock.pause();
	IAudioRender* audioRender = nullptr;
	IVideoRender* videoRender = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		audioRender = m_audioRender;
		videoRender = m_videoRender;
	}
	if (audioRender != nullptr)
	{
		audioRender->flushAudio();
		audioRender->closeAudio();
	}
	if (videoRender != nullptr)
	{
		videoRender->closeVideo();
	}
	PlayerEngineHelper::deleteReader(&m_videoReader);
	PlayerEngineHelper::deleteReader(&m_audioReader);
	PlayerEngineHelper::deleteReader(&m_previewReader);
	m_pendingVideoFrame.reset();
	m_pendingAudioFrame.reset();
	m_hasPendingVideoFrame = false;
	m_hasPendingAudioFrame = false;
	m_videoEnded = false;
	m_audioEnded = false;
	m_videoFrameIndex = 0;
	m_decodeFloor100ns = 0;
	m_position100ns = 0;
	m_state = LumaPlayerCoreStateClosed;
	m_mediaInfo.reset();
	m_loopRange.reset();
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_currentVideoFrame.reset();
		m_lastError.clear();
	}
	updateSnapshot(m_state, 0);
}

void PlayerEngine::workerLoop(PlayerWorkerTask* task, const std::atomic<bool>* exitFlag)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_workerStdThreadId = std::this_thread::get_id();
    }
    while (task != nullptr && !(exitFlag != nullptr && exitFlag->load()) && m_state == LumaPlayerCoreStatePlaying)
    {
        try
        {
            playbackStep(task, exitFlag);
        }
        catch (...)
        {
            m_clock.pause();
            m_state = LumaPlayerCoreStateError;
            updateSnapshot(m_state, m_position100ns);
            setLastError(LumaPlayerCoreResultInternalError);
            break;
        }
    }
}

void PlayerEngine::playbackStep(PlayerWorkerTask* task, const std::atomic<bool>* exitFlag)
{
    if (task == nullptr || (exitFlag != nullptr && exitFlag->load()))
    {
        return;
    }
    m_position100ns = clampPosition(m_clock.position100ns());
    const int64_t segmentStart = 0;
    const int64_t segmentEnd = activeSegmentEnd100ns();
    if (segmentEnd > segmentStart && m_position100ns >= segmentEnd)
    {
        replayFromLoopStart();
        return;
    }

    if (!m_hasPendingVideoFrame && !m_videoEnded && m_videoReader != nullptr)
    {
        LumaPlayerCoreResult result = readNextVideoFrame(&m_pendingVideoFrame);
        if (result == LumaPlayerCoreResultSuccess)
        {
            m_hasPendingVideoFrame = true;
        }
        else if (isReadEnd(result))
        {
            m_videoEnded = true;
        }
        else
        {
            m_state = LumaPlayerCoreStateError;
            updateSnapshot(m_state, m_position100ns);
            return;
        }
    }
    if (!m_hasPendingAudioFrame && !m_audioEnded && m_audioReader != nullptr)
    {
        LumaPlayerCoreResult result = readNextAudioFrame(&m_pendingAudioFrame);
        if (result == LumaPlayerCoreResultSuccess)
        {
            m_hasPendingAudioFrame = true;
        }
        else if (isReadEnd(result))
        {
            m_audioEnded = true;
        }
        else
        {
            m_state = LumaPlayerCoreStateError;
            updateSnapshot(m_state, m_position100ns);
            return;
        }
    }

    bool didWork = false;
    if (m_hasPendingAudioFrame)
    {
        if (m_pendingAudioFrame.m_timestamp100ns >= segmentEnd)
        {
            // 保留边界外PCM，修改范围后重新裁剪，避免为编辑端点清空声卡。
            m_audioEnded = true;
        }
        else if (m_pendingAudioFrame.endTime100ns() <= m_decodeFloor100ns || m_pendingAudioFrame.endTime100ns() <= segmentStart)
        {
            m_hasPendingAudioFrame = false;
            didWork = true;
        }
        else if (m_pendingAudioFrame.m_timestamp100ns <= m_position100ns ||
            m_pendingAudioFrame.m_timestamp100ns - m_position100ns <= m_config.m_audioLead100ns)
        {
            LumaPlayerAudioFrame clipped;
            int64_t clipStart = (std::max<int64_t>)(segmentStart, m_decodeFloor100ns);
            if (clipAudioFrame(m_pendingAudioFrame, clipStart, segmentEnd, &clipped))
            {
                if (renderAudioFrame(clipped) != LumaPlayerCoreResultSuccess)
            {
                m_state = LumaPlayerCoreStateError;
                updateSnapshot(m_state, m_position100ns);
                return;
            }
            }
            m_hasPendingAudioFrame = false;
            didWork = true;
        }
    }

    if (m_hasPendingVideoFrame)
    {
        if (m_pendingVideoFrame.m_timestamp100ns >= segmentEnd)
        {
            // 保留边界外第一帧，播放中延长B时直接接续，不丢已读取帧。
            m_videoEnded = true;
        }
        else if (m_pendingVideoFrame.endTime100ns() <= m_decodeFloor100ns || m_pendingVideoFrame.endTime100ns() <= segmentStart)
        {
            m_hasPendingVideoFrame = false;
            didWork = true;
        }
        else if (m_pendingVideoFrame.m_timestamp100ns <= m_position100ns ||
            m_pendingVideoFrame.m_timestamp100ns - m_position100ns <= m_config.m_videoEarlyTolerance100ns)
        {
            if (renderVideoFrame(m_pendingVideoFrame, true) != LumaPlayerCoreResultSuccess)
            {
                m_state = LumaPlayerCoreStateError;
                updateSnapshot(m_state, m_position100ns);
                return;
            }
            m_hasPendingVideoFrame = false;
            didWork = true;
        }
        else
        {
            int64_t wait100ns = m_pendingVideoFrame.m_timestamp100ns - m_position100ns;
            int32_t waitMs = static_cast<int32_t>(PlayerEngineHelper::clampInt64(wait100ns / (m_ratePermille * 10), 1, 10));
            task->wait(waitMs);
            return;
        }
    }

    if ((m_videoReader == nullptr || m_videoEnded) && (m_audioReader == nullptr || m_audioEnded) && !m_hasPendingVideoFrame && !m_hasPendingAudioFrame)
    {
        if (segmentEnd > m_position100ns)
        {
            int64_t remaining100ns = segmentEnd - m_position100ns;
            int32_t waitMs = static_cast<int32_t>(PlayerEngineHelper::clampInt64(remaining100ns / (m_ratePermille * 10), 1, 10));
            updateSnapshot(m_state, m_position100ns);
            task->wait(waitMs);
            return;
        }
        if (m_enableAutoReplay)
        {
            replayFromLoopStart();
            return;
        }
        m_state = LumaPlayerCoreStateEnded;
        m_position100ns = m_mediaInfo.m_duration100ns;
        m_clock.pause();
        updateSnapshot(m_state, m_position100ns);
        return;
    }

    updateSnapshot(m_state, m_position100ns);
    if (!didWork)
    {
        task->wait(2);
    }
}

LumaPlayerCoreResult PlayerEngine::seekInternal(int64_t position100ns, bool resumeAfterSeek, bool renderPreview,
    const std::atomic<bool>* exitFlag)
{
    if (m_videoReader == nullptr && m_audioReader == nullptr)
    {
        setLastError(LumaPlayerCoreResultNotOpen);
        return LumaPlayerCoreResultNotOpen;
    }
    m_activeLoopBuffer.reset();
    position100ns = clampPosition(position100ns);
    m_loopBoundaryEnabled = m_loopRange.isValid() && position100ns < m_loopRange.end100ns();
    m_clock.pause();
    m_state = LumaPlayerCoreStateSeeking;
    m_position100ns = position100ns;
    m_decodeFloor100ns = position100ns;
    updateSnapshot(m_state, m_position100ns);

    IAudioRender* audioRender = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        audioRender = m_audioRender;
    }
    if (audioRender != nullptr)
    {
        audioRender->pauseAudio();
        audioRender->flushAudio();
    }
    m_pendingVideoFrame.reset();
    m_pendingAudioFrame.reset();
    m_hasPendingVideoFrame = false;
    m_hasPendingAudioFrame = false;
    if (m_videoReader != nullptr && !m_videoReader->seek(position100ns))
    {
        m_state = LumaPlayerCoreStatePaused;
        updateSnapshot(m_state, m_position100ns);
        setLastError(LumaPlayerCoreResultVideoSeekFailed);
        return LumaPlayerCoreResultVideoSeekFailed;
    }
    if (m_audioReader != nullptr && !m_audioReader->seek(position100ns))
    {
        m_state = LumaPlayerCoreStatePaused;
        updateSnapshot(m_state, m_position100ns);
        setLastError(LumaPlayerCoreResultAudioSeekFailed);
        return LumaPlayerCoreResultAudioSeekFailed;
    }
    m_pendingVideoFrame.reset();
    m_pendingAudioFrame.reset();
    m_hasPendingVideoFrame = false;
    m_hasPendingAudioFrame = false;
    m_videoEnded = m_videoReader == nullptr;
    m_audioEnded = m_audioReader == nullptr;
    if (exitFlag != nullptr && exitFlag->load())
    {
        m_clock.seek(position100ns);
        m_state = LumaPlayerCoreStatePaused;
        updateSnapshot(m_state, position100ns);
        return LumaPlayerCoreResultCanceled;
    }
    if (resumeAfterSeek || renderPreview)
    {
        LumaPlayerCoreResult result = primePlayback(position100ns);
        if (result != LumaPlayerCoreResultSuccess)
        {
            m_state = LumaPlayerCoreStatePaused;
            updateSnapshot(m_state, m_position100ns);
            return result;
        }
    }
    if (!resumeAfterSeek && renderPreview)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_currentVideoFrame.m_isSet && m_currentVideoFrame.m_frameStart100ns <= position100ns &&
            position100ns < m_currentVideoFrame.m_frameEnd100ns)
        {
            position100ns = m_currentVideoFrame.m_frameStart100ns;
            m_position100ns = position100ns;
            m_decodeFloor100ns = position100ns;
        }
    }
    m_clock.seek(position100ns);
    if (resumeAfterSeek)
    {
        if (audioRender != nullptr)
        {
            audioRender->resumeAudio();
        }
        m_clock.start(position100ns, m_ratePermille);
        m_state = LumaPlayerCoreStatePlaying;
    }
    else
    {
        if (audioRender != nullptr)
        {
            audioRender->pauseAudio();
        }
        m_state = LumaPlayerCoreStatePaused;
    }
    updateSnapshot(m_state, m_position100ns);
    return LumaPlayerCoreResultSuccess;
}

bool PlayerEngine::refineLoopPointByPreview(LumaPlayerLoopPointInfo* pointInfo, int64_t position100ns,
    const std::atomic<bool>* exitFlag)
{
    if (pointInfo == nullptr || !pointInfo->m_isSet || (exitFlag != nullptr && exitFlag->load()))
    {
        return false;
    }

    FFmpegCppPlaybackPreviewProfile profile;
    FFmpegCppPlaybackVideoFrame ffmpegFrame;

    int64_t requestPosition100ns = position100ns;
    int64_t mediaDuration100ns = 0;

    bool readOk = false;
    {
        std::lock_guard<std::mutex> previewLock(m_previewReaderMutex);
        if (m_previewReader == nullptr)
        {
            return false;
        }
        mediaDuration100ns = m_previewReader->duration100ns();
        requestPosition100ns = PlayerEngineHelper::clampInt64(requestPosition100ns, 0, mediaDuration100ns > 0 ? mediaDuration100ns : 0);
        PlayerPreviewCancelContext cancelContext;
        cancelContext.m_previewSerial = &m_previewSerial;
        cancelContext.m_requestSerial = 0;
        cancelContext.m_exitFlag = exitFlag;
        FFmpegCppPlaybackPreviewOption previewOption;
        previewOption.cancelCallback = exitFlag != nullptr ? &PlayerPreviewCancelCallback : nullptr;
        previewOption.cancelUserData = exitFlag != nullptr ? &cancelContext : nullptr;
        readOk = m_previewReader->readVideoFrameAtEx(requestPosition100ns, previewOption, &ffmpegFrame, &profile);

    }

    if (!readOk)
    {
        return false;
    }

    int64_t frameIndex = pointInfo->m_frameIndex;
    pointInfo->m_isSet = true;
    pointInfo->m_frameStart100ns = PlayerEngineHelper::clampInt64(ffmpegFrame.timestamp100ns, 0, mediaDuration100ns > 0 ? mediaDuration100ns : ffmpegFrame.timestamp100ns);
    pointInfo->m_frameEnd100ns = PlayerEngineHelper::clampInt64(ffmpegFrame.timestamp100ns + ffmpegFrame.duration100ns, 0, mediaDuration100ns > 0 ? mediaDuration100ns : ffmpegFrame.timestamp100ns + ffmpegFrame.duration100ns);
    pointInfo->m_frameIndex = frameIndex;
    return true;
}

LumaPlayerCoreResult PlayerEngine::previewFrameInternal(int64_t position100ns, uint64_t requestSerial, const std::atomic<bool>* exitFlag, bool updatePlaybackPosition)
{
	if (exitFlag != nullptr && exitFlag->load())
	{
		return LumaPlayerCoreResultCanceled;
	}
	if (requestSerial != 0 && requestSerial != m_previewSerial.load())
	{
		return LumaPlayerCoreResultCanceled;
	}
	if (updatePlaybackPosition && m_videoReader == nullptr && m_audioReader == nullptr)
	{
		setLastError(LumaPlayerCoreResultNotOpen);
		return LumaPlayerCoreResultNotOpen;
	}

	PlayerPreviewCancelContext cancelContext;
	cancelContext.m_previewSerial = &m_previewSerial;
	cancelContext.m_exitFlag = exitFlag;
	cancelContext.m_requestSerial = requestSerial;
	FFmpegCppPlaybackPreviewOption previewOption;
	previewOption.cancelCallback = requestSerial != 0 || exitFlag != nullptr ? &PlayerPreviewCancelCallback : nullptr;
	previewOption.cancelUserData = previewOption.cancelCallback != nullptr ? &cancelContext : nullptr;
	FFmpegCppPlaybackPreviewProfile profile;
	FFmpegCppPlaybackVideoFrame ffmpegFrame;

	int64_t requestPosition100ns = position100ns;

	bool readOk = false;
	// reader锁覆盖最终发布，close必须等已进入的回调结束后才能清空画面
	std::lock_guard<std::mutex> previewLock(m_previewReaderMutex);
	{
		if (m_previewReader == nullptr)
		{
			if (updatePlaybackPosition)
			{
				m_position100ns = clampPosition(position100ns);
				m_clock.seek(m_position100ns);
				updateSnapshot(m_state, m_position100ns);
				return LumaPlayerCoreResultSuccess;
			}
			return LumaPlayerCoreResultNotOpen;
		}
		int64_t duration100ns = m_previewReader->duration100ns();
		requestPosition100ns = PlayerEngineHelper::clampInt64(position100ns, 0, duration100ns > 0 ? duration100ns : 0);
		if (updatePlaybackPosition)
		{
			m_position100ns = requestPosition100ns;
			m_clock.seek(requestPosition100ns);
		}
		readOk = m_previewReader->readVideoFrameAtEx(requestPosition100ns, previewOption, &ffmpegFrame, &profile);

	}

	if (!readOk)
	{
		if (profile.canceled || (exitFlag != nullptr && exitFlag->load()) || (requestSerial != 0 && requestSerial != m_previewSerial.load()))
		{
			return LumaPlayerCoreResultCanceled;
		}
		setLastError(LumaPlayerCoreResultPreviewDecodeFailed);
		return LumaPlayerCoreResultPreviewDecodeFailed;
	}
	if ((exitFlag != nullptr && exitFlag->load()) || (requestSerial != 0 && requestSerial != m_previewSerial.load()))
	{
		return LumaPlayerCoreResultCanceled;
	}

	LumaPlayerVideoFrame frame;
	PlayerEngineHelper::convertVideoFrame(&ffmpegFrame, -1, &frame);
	LumaPlayerCoreResult renderResult = renderVideoFrame(frame, updatePlaybackPosition);
	if (renderResult != LumaPlayerCoreResultSuccess)
	{
		return renderResult;
	}
	if (updatePlaybackPosition)
	{
		updateSnapshot(m_state, m_position100ns);
	}
	return LumaPlayerCoreResultSuccess;
}

LumaPlayerCoreResult PlayerEngine::readAdjacentVideoFrame(const LumaPlayerLoopPointInfo& origin, int32_t direction, LumaPlayerVideoFrame* frame, uint64_t requestSerial, const std::atomic<bool>* exitFlag)
{
    if (frame == nullptr || !origin.m_isSet || direction == 0)
    {
        return LumaPlayerCoreResultInvalidParam;
    }
    PlayerPreviewCancelContext context;
    context.m_previewSerial = &m_previewSerial;
    context.m_requestSerial = requestSerial;
    context.m_exitFlag = exitFlag;
    FFmpegCppPlaybackPreviewOption option;
    option.cancelCallback = &PlayerPreviewCancelCallback;
    option.cancelUserData = &context;
    FFmpegCppPlaybackPreviewProfile profile;
    FFmpegCppPlaybackVideoFrame decoded;
    std::lock_guard<std::mutex> lock(m_previewReaderMutex);
    if (PlayerPreviewCancelCallback(&context))
    {
        return LumaPlayerCoreResultCanceled;
    }
    if (m_previewReader == nullptr)
    {
        return LumaPlayerCoreResultNotOpen;
    }
    if (!m_previewReader->readAdjacentVideoFrameEx(origin.m_frameStart100ns, direction, option, &decoded, &profile))
    {
        return profile.canceled ? LumaPlayerCoreResultCanceled : (direction > 0 ? LumaPlayerCoreResultNextFrameUnavailable : LumaPlayerCoreResultPreviousFrameUnavailable);
    }
    if (PlayerPreviewCancelCallback(&context))
    {
        return LumaPlayerCoreResultCanceled;
    }
    PlayerEngineHelper::convertVideoFrame(&decoded, -1, frame);
    return LumaPlayerCoreResultSuccess;
}

LumaPlayerCoreResult PlayerEngine::readNextVideoFrame(LumaPlayerVideoFrame* frame)
{
    if (m_activeLoopBuffer != nullptr)
    {
        LumaPlayerCoreResult result = m_activeLoopBuffer->videoFrame(m_loopVideoIndex, frame);
        if (result == LumaPlayerCoreResultSuccess)
        {
            ++m_loopVideoIndex;
            return result;
        }
        const bool changedRange = !m_activeLoopBuffer->matches(m_mediaInfo,
            activeSegmentStart100ns(), activeSegmentEnd100ns());
        if (result != LumaPlayerCoreResultNotOpen && !(changedRange && result == LumaPlayerCoreResultEnd))
        {
            return result;
        }
    }

	if (frame == nullptr || m_videoReader == nullptr)
	{
		return LumaPlayerCoreResultInvalidParam;
	}
	for (;;)
	{
		FFmpegCppPlaybackFrame decodedFrame;
		FFmpegCppPlaybackReadResult readResult = m_videoReader->read(&decodedFrame);
		if (readResult == FFmpegCppPlaybackReadResultEnd)
		{
			return LumaPlayerCoreResultEnd;
		}
		if (readResult == FFmpegCppPlaybackReadResultError)
		{
			setLastError(LumaPlayerCoreResultVideoReadFailed);
			return LumaPlayerCoreResultVideoReadFailed;
		}
		if (decodedFrame.type == FFmpegCppPlaybackFrameTypeVideo)
		{
			PlayerEngineHelper::convertVideoFrame(&decodedFrame.videoFrame, m_videoFrameIndex++, frame);
			return LumaPlayerCoreResultSuccess;
		}
	}
}

LumaPlayerCoreResult PlayerEngine::readNextAudioFrame(LumaPlayerAudioFrame* frame)
{
    if (m_activeLoopBuffer != nullptr)
    {
        LumaPlayerCoreResult result = m_activeLoopBuffer->audioFrame(m_loopAudioIndex, frame);
        if (result == LumaPlayerCoreResultSuccess)
        {
            ++m_loopAudioIndex;
            return result;
        }
        const bool changedRange = !m_activeLoopBuffer->matches(m_mediaInfo,
            activeSegmentStart100ns(), activeSegmentEnd100ns());
        if (result != LumaPlayerCoreResultNotOpen && !(changedRange && result == LumaPlayerCoreResultEnd))
        {
            return result;
        }
    }

	if (frame == nullptr || m_audioReader == nullptr)
	{
		return LumaPlayerCoreResultInvalidParam;
	}
	for (;;)
	{
		FFmpegCppPlaybackFrame decodedFrame;
		FFmpegCppPlaybackReadResult readResult = m_audioReader->read(&decodedFrame);
		if (readResult == FFmpegCppPlaybackReadResultEnd)
		{
			return LumaPlayerCoreResultEnd;
		}
		if (readResult == FFmpegCppPlaybackReadResultError)
		{
			setLastError(LumaPlayerCoreResultAudioReadFailed);
			return LumaPlayerCoreResultAudioReadFailed;
		}
		if (decodedFrame.type == FFmpegCppPlaybackFrameTypeAudio)
		{
			PlayerEngineHelper::convertAudioFrame(&decodedFrame.audioFrame, frame);
			return LumaPlayerCoreResultSuccess;
		}
	}
}

LumaPlayerCoreResult PlayerEngine::renderVideoFrame(const LumaPlayerVideoFrame& frame, bool updatePlaybackPosition)
{
	if (frame.empty())
	{
		return LumaPlayerCoreResultSuccess;
	}
	IVideoRender* videoRender = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		videoRender = m_videoRender;
	}
	if (videoRender != nullptr)
	{
		if (!videoRender->renderVideo(frame))
		{
			setLastError(LumaPlayerCoreResultVideoRenderFailed);
			return LumaPlayerCoreResultVideoRenderFailed;
		}
	}

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		int64_t duration100ns = m_snapshot.m_mediaInfo.m_duration100ns;
		int64_t frameEnd100ns = frame.endTime100ns();
		if (duration100ns > 0)
		{
			frameEnd100ns = PlayerEngineHelper::clampInt64(frameEnd100ns, 0, duration100ns);
		}
		m_currentVideoFrame.m_isSet = true;
		m_currentVideoFrame.m_frameStart100ns = frame.m_timestamp100ns;
		m_currentVideoFrame.m_frameEnd100ns = frameEnd100ns;
		m_currentVideoFrame.m_frameIndex = frame.m_frameIndex;
		m_snapshot.m_currentVideoFrame = m_currentVideoFrame;
		if (updatePlaybackPosition)
		{
            m_position100ns = (std::max<int64_t>)(m_position100ns, frame.m_timestamp100ns);
			m_snapshot.m_state = m_state;
			m_snapshot.m_mediaSerial = m_mediaSerial;
			m_snapshot.m_mediaInfo = m_mediaInfo;
			m_snapshot.m_position100ns = clampPosition(m_position100ns);
			m_snapshot.m_ratePermille = m_ratePermille;
			m_snapshot.m_loopRange = m_loopRange;
			m_snapshot.m_enableAutoReplay = m_enableAutoReplay;
		}
	}
	return LumaPlayerCoreResultSuccess;
}

LumaPlayerCoreResult PlayerEngine::renderAudioFrame(const LumaPlayerAudioFrame& frame)
{
	IAudioRender* audioRender = nullptr;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		audioRender = m_audioRender;
	}
	if (audioRender == nullptr)
	{
		return LumaPlayerCoreResultSuccess;
	}
	if (frame.empty())
	{
		audioRender->flushAudio();
		return LumaPlayerCoreResultSuccess;
	}
	LumaPlayerAudioFrame scaledFrame;
	const LumaPlayerAudioFrame* outputFrame = &frame;
	if (m_ratePermille != 1000)
	{
		if (!PlayerEngineHelper::scaleAudioRate(frame, m_ratePermille, &scaledFrame))
		{
			setLastError(LumaPlayerCoreResultAudioRenderFailed);
			return LumaPlayerCoreResultAudioRenderFailed;
		}
		outputFrame = &scaledFrame;
	}
	if (!audioRender->renderAudio(*outputFrame))
		{
			setLastError(LumaPlayerCoreResultAudioRenderFailed);
			return LumaPlayerCoreResultAudioRenderFailed;
		}
	return LumaPlayerCoreResultSuccess;
}

bool PlayerEngine::clipAudioFrame(const LumaPlayerAudioFrame& source, int64_t start100ns, int64_t end100ns, LumaPlayerAudioFrame* clipped) const
{
	if (clipped == nullptr)
	{
		return false;
	}
	clipped->reset();
	if (source.empty() || end100ns <= start100ns)
	{
		return false;
	}
	int64_t sourceStart = source.m_timestamp100ns;
	int64_t sourceEnd = source.endTime100ns();
    int64_t clipStart = (std::max<int64_t>)(sourceStart, start100ns);
    int64_t clipEnd = (std::min<int64_t>)(sourceEnd, end100ns);
	if (clipEnd <= clipStart)
	{
		return false;
	}
	int32_t bytesPerFrame = source.m_format.bytesPerFrame();
	if (bytesPerFrame <= 0)
	{
		return false;
	}
	size_t totalSampleCount = source.m_pcmData.size() / static_cast<size_t>(bytesPerFrame);
	if (totalSampleCount == 0)
	{
		return false;
	}
	int64_t startOffset100ns = clipStart - sourceStart;
	int64_t endOffset100ns = clipEnd - sourceStart;
	size_t startSample = PlayerAudioSampleOffset(startOffset100ns, source.m_format.m_sampleRate, totalSampleCount, true);
	size_t endSample = PlayerAudioSampleOffset(endOffset100ns, source.m_format.m_sampleRate, totalSampleCount, false);
	if (endSample > totalSampleCount)
	{
		endSample = totalSampleCount;
	}
	if (startSample >= endSample)
	{
		return false;
	}
	size_t startByte = startSample * static_cast<size_t>(bytesPerFrame);
	size_t endByte = endSample * static_cast<size_t>(bytesPerFrame);
	try
	{
		clipped->m_pcmData.assign(source.m_pcmData.begin() + startByte, source.m_pcmData.begin() + endByte);
	}
	catch (...)
	{
		return false;
	}
	clipped->m_format = source.m_format;
	clipped->m_timestamp100ns = clipStart;
	clipped->m_duration100ns = ((int64_t)(endSample - startSample) * LUMAPLAYER_TICKS_PER_SECOND) / source.m_format.m_sampleRate;
	return !clipped->empty();
}

void PlayerEngine::replayFromLoopStart()
{
    if (!m_loopBoundaryEnabled && m_loopRange.isValid())
    {
        const LumaPlayerCoreResult result = seekInternal(0, true, false);
        if (result != LumaPlayerCoreResultSuccess)
        {
            setLastError(result);
            m_state = LumaPlayerCoreStateError;
            updateSnapshot(m_state, m_position100ns);
        }
        refreshLoopBuffer();
        return;
    }
    const int64_t start = activeSegmentStart100ns();
    const int64_t end = activeSegmentEnd100ns();
    bool buffered = m_activeLoopBuffer != nullptr && m_activeLoopBuffer->matches(m_mediaInfo, start, end) && m_activeLoopBuffer->isComplete();
    if (!buffered && m_preparedLoopBuffer != nullptr && m_preparedLoopBuffer->matches(m_mediaInfo, start, end) &&
        m_preparedLoopBuffer->isReady() && m_preparedLoopBuffer->result() == LumaPlayerCoreResultSuccess)
    {
        m_activeLoopBuffer = m_preparedLoopBuffer;
        m_preparedLoopBuffer.reset();
        PlayerEngineHelper::deleteReader(&m_videoReader);
        PlayerEngineHelper::deleteReader(&m_audioReader);
        m_videoReader = m_activeLoopBuffer->takeVideoReader();
        m_audioReader = m_activeLoopBuffer->takeAudioReader();
        buffered = true;
    }
    LumaPlayerCoreResult result = LumaPlayerCoreResultSuccess;
    if (buffered)
    {
        m_clock.pause();
        m_loopVideoIndex = 0;
        m_loopAudioIndex = 0;
        m_videoEnded = !m_mediaInfo.m_hasVideo;
        m_audioEnded = !m_mediaInfo.m_hasAudio;
        m_hasPendingVideoFrame = false;
        m_hasPendingAudioFrame = false;
        m_position100ns = start;
        m_decodeFloor100ns = start;
        // 上轮PCM已按B边界裁剪，直接追加新一轮，避免reset声卡造成断音
        result = primePlayback(start);
        if (result == LumaPlayerCoreResultSuccess)
        {
            m_clock.start(start, m_ratePermille);
            m_state = LumaPlayerCoreStatePlaying;
            updateSnapshot(m_state, start);
        }
    }
    else
    {
        // 首轮预备尚未完成时保证准确首帧，不能让时钟吞掉解码时间
        result = seekInternal(start, true, false);
    }
    if (result != LumaPlayerCoreResultSuccess)
    {
        m_clock.pause();
        setLastError(result);
        m_state = result == LumaPlayerCoreResultCanceled ? LumaPlayerCoreStatePaused : LumaPlayerCoreStateError;
        updateSnapshot(m_state, m_position100ns);
    }
    refreshLoopBuffer();
}

void PlayerEngine::clearLoopBuffers()
{
    if (m_preparedLoopBuffer != nullptr)
    {
        m_preparedLoopBuffer->StopTask();
        m_preparedLoopBuffer.reset();
    }
    m_activeLoopBuffer.reset();
    m_loopVideoIndex = 0;
    m_loopAudioIndex = 0;
}

void PlayerEngine::refreshLoopBuffer()
{
    const int64_t start = activeSegmentStart100ns();
    const int64_t end = activeSegmentEnd100ns();
    if (end <= start || m_mediaInfo.m_filePath.empty())
    {
        clearLoopBuffers();
        return;
    }
    // 编辑端点后旧缓存仍是连续读取源；不能清缓存并seek打断当前播放。
    // 回绕时只选匹配新范围的缓存，旧缓存读尽后由原读取器接续。
    if (m_activeLoopBuffer != nullptr && m_activeLoopBuffer->matches(m_mediaInfo, start, end) &&
        m_activeLoopBuffer->isComplete())
    {
        return;
    }
    if (m_preparedLoopBuffer != nullptr)
    {
        if (m_preparedLoopBuffer->matches(m_mediaInfo, start, end))
        {
            return;
        }
        m_preparedLoopBuffer->StopTask();
        m_preparedLoopBuffer.reset();
    }
    uint32_t threadId = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_isInit)
        {
            return;
        }
        threadId = m_loopThreadId;
    }
    std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
    if (thread == nullptr)
    {
        return;
    }
    try
    {
        m_preparedLoopBuffer.reset(new PlayerLoopBuffer(m_mediaInfo, start, end, m_config.m_loopBufferMaxBytes));
        thread->StopTask(PlayerLoopBuffer::kTaskId);
        thread->PostTask(m_preparedLoopBuffer, 1);
    }
    catch (...)
    {
        m_preparedLoopBuffer.reset();
        // 预备属于优化，失败后循环仍走精确定位路径
    }
}

LumaPlayerCoreResult PlayerEngine::primePlayback(int64_t position100ns)
{
    const uint64_t serial = m_previewSerial.load();
    // 先完成音频预滚动，避免显示A后再花时间解码音频而压缩A的展示时长
    while (!m_audioEnded && !m_hasPendingAudioFrame)
    {
        if (!isInit() || serial != m_previewSerial.load())
        {
            return LumaPlayerCoreResultCanceled;
        }
        LumaPlayerCoreResult result = readNextAudioFrame(&m_pendingAudioFrame);
        if (result == LumaPlayerCoreResultEnd)
        {
            m_audioEnded = true;
            break;
        }
        if (result != LumaPlayerCoreResultSuccess)
        {
            return result;
        }
        m_hasPendingAudioFrame = m_pendingAudioFrame.endTime100ns() > position100ns;
    }
    while (!m_videoEnded && !m_hasPendingVideoFrame)
    {
        if (!isInit() || serial != m_previewSerial.load())
        {
            return LumaPlayerCoreResultCanceled;
        }
        LumaPlayerCoreResult result = readNextVideoFrame(&m_pendingVideoFrame);
        if (result == LumaPlayerCoreResultEnd)
        {
            m_videoEnded = true;
            break;
        }
        if (result != LumaPlayerCoreResultSuccess)
        {
            return result;
        }
        m_hasPendingVideoFrame = m_pendingVideoFrame.endTime100ns() > position100ns;
    }
    if (m_hasPendingVideoFrame && m_pendingVideoFrame.m_timestamp100ns <= position100ns &&
        m_pendingVideoFrame.m_timestamp100ns < activeSegmentEnd100ns())
    {
        LumaPlayerCoreResult result = renderVideoFrame(m_pendingVideoFrame, true);
        m_hasPendingVideoFrame = false;
        if (result != LumaPlayerCoreResultSuccess)
        {
            return result;
        }
    }
    return LumaPlayerCoreResultSuccess;
}

void PlayerEngine::updateSnapshot(LumaPlayerCoreState state, int64_t position100ns)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_snapshot.m_state = state;
	m_snapshot.m_mediaSerial = m_mediaSerial;
	m_snapshot.m_mediaInfo = m_mediaInfo;
	m_snapshot.m_position100ns = clampPosition(position100ns);
	m_snapshot.m_ratePermille = m_ratePermille;
	m_snapshot.m_loopRange = m_loopRange;
	m_snapshot.m_currentVideoFrame = m_currentVideoFrame;
	m_snapshot.m_enableAutoReplay = m_enableAutoReplay;
}

void PlayerEngine::setLastError(LumaPlayerCoreResult result)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_snapshot.m_result = result;
    std::map<LumaPlayerCoreResult, std::string>::const_iterator it = kLumaPlayerCoreResultDescriptions.find(result);
    m_lastError = result == LumaPlayerCoreResultSuccess ? std::string() :
        (it != kLumaPlayerCoreResultDescriptions.end() ? it->second : "未知的播放器错误码");
}

int64_t PlayerEngine::clampPosition(int64_t position100ns) const
{
	int64_t duration = m_mediaInfo.m_duration100ns;
	if (duration < 0)
	{
		duration = 0;
	}
	return PlayerEngineHelper::clampInt64(position100ns, 0, duration);
}

int64_t PlayerEngine::activeSegmentStart100ns() const
{
    return m_loopBoundaryEnabled && m_loopRange.isValid() ? m_loopRange.start100ns() : 0;
}

int64_t PlayerEngine::activeSegmentEnd100ns() const
{
    if (m_loopBoundaryEnabled && m_loopRange.isValid())
	{
		return m_loopRange.end100ns();
	}
	return m_mediaInfo.m_duration100ns > 0 ? m_mediaInfo.m_duration100ns : 0;
}

bool PlayerEngine::isReadEnd(LumaPlayerCoreResult result) const
{
	return result == LumaPlayerCoreResultEnd;
}

LumaPlayerCoreResult PlayerEngine::submitAsyncEx(const LumaPlayerCoreRequest& request,
    const LumaPlayerCoreCompletionCallback& callback)
{
    std::lock_guard<std::mutex> asyncSubmitLock(m_asyncSubmitMutex);
    if (!callback || request.m_requestId == 0 ||
        request.m_operation < LumaPlayerCoreOperationOpen || request.m_operation > LumaPlayerCoreOperationRate)
    {
        return LumaPlayerCoreResultInvalidParam;
    }
    std::shared_ptr<PlayerCompletionTask> dispatcher;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_isInit || m_completionTask == nullptr)
        {
            return LumaPlayerCoreResultNotInit;
        }
        dispatcher = m_completionTask;
    }
    PlayerCommand command;
    command.m_type = static_cast<PlayerCommandType>(request.m_operation);
    command.m_filePath = request.m_filePath;
    command.m_position100ns = request.m_position100ns;
    command.m_keepPlayState = request.m_playAfterSeek;
    command.m_usePosition = true;
    command.m_ratePermille = request.m_ratePermille;
    command.m_loopPointType = request.m_point;
    command.m_frameOffset = request.m_frameOffset;
    command.m_serial = (command.m_type == PlayerCommandOpen || command.m_type == PlayerCommandSeek ||
        command.m_type == PlayerCommandPreview || command.m_type == PlayerCommandMoveLoopPoint) ?
        nextPreviewSerial() : m_previewSerial.load();
    command.m_completion = std::make_shared<PlayerRequestCompletion>(request, callback, dispatcher);
    if (!dispatcher->reserve(command.m_completion->m_delivery))
    {
        return LumaPlayerCoreResultCommandQueueFailed;
    }
    LumaPlayerCoreResult result = LumaPlayerCoreResultNotInit;
    if (command.m_type == PlayerCommandPreview)
    {
        std::shared_ptr<PlayerPreviewTask> task;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            task = m_previewTask;
        }
        if (task != nullptr)
        {
            task->request(command.m_position100ns, command.m_serial, command.m_completion);
            result = LumaPlayerCoreResultSuccess;
        }
    }
    else
    {
        result = postCommand(command);
    }
    command.m_completion->m_accepted.store(result == LumaPlayerCoreResultSuccess);
    return result;
}