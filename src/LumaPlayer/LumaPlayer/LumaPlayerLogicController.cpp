#include "LumaPlayerLogicController.h"
#include "LumaPlayerLogicTask.h"
#include "LumaPlayerLoadTask.h"
#include "LumaPlayerViewTask.h"
#include "LumaPlayerResetTask.h"
#include "LumaPlayerLoopTask.h"
#include "LumaPlayerPlaybackTask.h"
#include "LumaPlayerWindowTask.h"
#include "LumaPlayerSeekTask.h"
#include "LumaPlayerLifecycleTask.h"
#include "Config.h"
#include "LogManager/LogManagerAPI.h"
#include <algorithm>
#include <cmath>
#include <chrono>

LumaPlayerLogicController::LumaPlayerLogicController(LumaPlayerCoreBridge* core) :
m_core(core),
m_logicThreadId(0),
m_lifecycleThreadId(0),
m_closing(false),
m_ready(false),
m_nextRequestId(0),
m_seekRequest(0),
m_seekTarget(0),
m_seekPlaying(false),
m_rateRequest(0),
m_rateTarget(1000),
m_playRequest(0),
m_playTarget(false),
m_dragActive(false),
m_dragPaused(false),
m_dragFinishing(false),
m_dragSerial(0),
m_dragCommitRequest(0),
m_dragPreviewRequest(0),
m_dragPreviewRevision(0),
m_dragPreviewCount(0),
m_dragLatest(),
m_dragDeferred(),
m_resetRequest(0),
m_resetRevision(0),
m_resetViewApplied(false),
m_resetRateApplied(false)
{
    qRegisterMetaType<LumaPlayerLogicResult>("LumaPlayerLogicResult");
}

LumaPlayerLogicController::~LumaPlayerLogicController()
{
    // 正常关闭已在后台完成回收；异常离开事件循环时仍必须保住回调接收者
    m_closing.store(true);
    if (m_lifecycleThreadId != 0)
    {
        CTaskThreadManager::Instance().Uninit(m_lifecycleThreadId);
        m_lifecycleThreadId = 0;
    }
    if (m_logicThreadId != 0)
    {
        CTaskThreadManager::Instance().Uninit(m_logicThreadId);
        m_logicThreadId = 0;
    }
    // 所有生产者停止之后才允许销毁本QObject和借用的Bridge
    m_core->uninit();
    m_pending.clear();
}

bool LumaPlayerLogicController::init()
{
    try
    {
        m_logicThreadId = CTaskThreadManager::Instance().Init();
        m_lifecycleThreadId = CTaskThreadManager::Instance().Init();
        std::shared_ptr<CTaskThread> lifecycle = CTaskThreadManager::Instance().GetThreadInterface(m_lifecycleThreadId);
        if (m_logicThreadId == 0 || lifecycle == nullptr)
        {
            return false;
        }
        lifecycle->PostTask(std::make_shared<LumaPlayerLifecycleTask>(this, false), 1);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool LumaPlayerLogicController::submit(const LumaPlayerLogicAction& action)
{
    std::lock_guard<std::mutex> lock(m_submitMutex);
    if (m_closing.load())
    {
        return false;
    }
    std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(m_logicThreadId);
    if (thread == nullptr)
    {
        return false;
    }
    try
    {
        if (action.m_type == LumaActionPoll)
        {
            thread->StopTask(4000 + static_cast<int32_t>(LumaActionPoll));
        }
        if (thread->GetWaitTaskCount() >= g_config.m_logicQueueLimit &&
            action.m_type != LumaActionCompleted && action.m_type != LumaActionShutdown)
        {
            LOGWARNING("Logic queue full, action=%d", static_cast<int32_t>(action.m_type));
            return false;
        }
        std::shared_ptr<CTask> task;
        if (action.m_type == LumaActionDragBegin || action.m_type == LumaActionDragPreview ||
            action.m_type == LumaActionDragCommit || action.m_type == LumaActionJump || (action.m_type == LumaActionCore &&
            (action.m_operation == LumaPlayerCoreCOperationSeek || action.m_operation == LumaPlayerCoreCOperationPreview)))
        {
            task = std::make_shared<LumaPlayerSeekTask>(this, action);
        }
        else if (action.m_type == LumaActionLoad)
        {
            task = std::make_shared<LumaPlayerLoadTask>(this, action);
        }
        else if (action.m_type == LumaActionReset)
        {
            task = std::make_shared<LumaPlayerResetTask>(this, action);
        }
        else if (action.m_type == LumaActionPan || action.m_type == LumaActionZoom)
        {
            task = std::make_shared<LumaPlayerViewTask>(this, action);
        }
        else if (action.m_type == LumaActionCancelMove || (action.m_type == LumaActionCore &&
            action.m_operation >= LumaPlayerCoreCOperationSetA && action.m_operation <= LumaPlayerCoreCOperationMoveLoop))
        {
            task = std::make_shared<LumaPlayerLoopTask>(this, action);
        }
        else if (action.m_type == LumaActionTogglePlayback || action.m_type == LumaActionRateDelta || action.m_type == LumaActionCore)
        {
            task = std::make_shared<LumaPlayerPlaybackTask>(this, action);
        }
        else if (action.m_type == LumaActionWindowApplied || action.m_type == LumaActionHelpClose ||
            action.m_type == LumaActionHelp || action.m_type == LumaActionMinimize ||
            action.m_type == LumaActionMaximize || action.m_type == LumaActionFullscreen ||
            action.m_type == LumaActionViewport || action.m_type == LumaActionPin)
        {
            task = std::make_shared<LumaPlayerWindowTask>(this, action);
        }
        else
        {
            task = std::make_shared<LumaPlayerLogicTask>(this, action);
        }
        if (action.m_type == LumaActionShutdown)
        {
            m_closing.store(true);
        }
        thread->PostTask(task, 1);
        return true;
    }
    catch (...)
    {
        if (action.m_type == LumaActionShutdown)
        {
            m_closing.store(false);
        }
        LOGERROR("Logic task submission failed, action=%d", static_cast<int32_t>(action.m_type));
        return false;
    }
}

void LumaPlayerLogicController::publish(LumaPlayerActionType type, int32_t error, uint64_t requestId,
    int32_t operation, int32_t point, uint64_t inputSerial)
{
    LumaPlayerLogicResult result = m_state;
    result.m_type = type;
    result.m_error = error;
    result.m_requestId = requestId;
    result.m_operation = operation;
    result.m_point = point;
    result.m_inputSerial = inputSerial;
    emit resultReady(result);
}

uint64_t LumaPlayerLogicController::submitCore(const LumaPlayerLogicAction& action)
{
    if (m_pending.size() >= static_cast<size_t>(g_config.m_corePendingLimit))
    {
        publish(LumaActionCompleted, LumaPlayerCoreCResultCommandQueueFailed, 0,
            action.m_operation, action.m_point, action.m_revision);
        return 0;
    }
    LumaPlayerCoreCRequest request = {};
    request.m_requestId = ++m_nextRequestId;
    request.m_mediaGeneration = m_state.m_mediaGeneration;
    request.m_operation = action.m_operation;
    request.m_position100ns = action.m_value;
    request.m_playAfterSeek = action.m_flag ? 1 : 0;
    request.m_ratePermille = static_cast<int32_t>(action.m_value);
    request.m_point = action.m_point;
    request.m_frameOffset = static_cast<int32_t>(action.m_value);
    const QByteArray path = action.m_path.toUtf8();
    request.m_filePath = path.constData();
    m_pending[request.m_requestId] = action;
    const int32_t result = m_core->submitAsyncEx(request, &LumaPlayerLogicController::coreCompleted, this);
    if (action.m_operation != LumaPlayerCoreCOperationPreview)
    {
        LOGINFO("Request submitted id=%llu generation=%llu operation=%d result=%d",
            static_cast<unsigned long long>(request.m_requestId),
            static_cast<unsigned long long>(request.m_mediaGeneration), request.m_operation, result);
    }
    if (result != LumaPlayerCoreCResultSuccess)
    {
        LumaPlayerCoreCCompletion completion = {};
        completion.m_requestId = request.m_requestId;
        completion.m_mediaGeneration = request.m_mediaGeneration;
        completion.m_operation = request.m_operation;
        completion.m_result = result;
        complete(completion);
        return 0;
    }
    if (request.m_operation == LumaPlayerCoreCOperationSeek)
    {
        m_seekRequest = request.m_requestId;
        m_seekTarget = request.m_position100ns;
        m_seekPlaying = request.m_playAfterSeek != 0;
    }
    if (request.m_operation == LumaPlayerCoreCOperationRate)
    {
        m_rateRequest = request.m_requestId;
        m_rateTarget = request.m_ratePermille;
    }
    if (request.m_operation == LumaPlayerCoreCOperationPlay || request.m_operation == LumaPlayerCoreCOperationPause)
    {
        m_playRequest = request.m_requestId;
        m_playTarget = request.m_operation == LumaPlayerCoreCOperationPlay;
    }
    return request.m_requestId;
}

void LumaPlayerLogicController::coreCompleted(const LumaPlayerCoreCCompletion* completion, void* userData)
{
    LumaPlayerLogicController* controller = static_cast<LumaPlayerLogicController*>(userData);
    if (controller == nullptr || completion == nullptr)
    {
        return;
    }
    LumaPlayerLogicAction action;
    action.m_type = LumaActionCompleted;
    action.m_completion = *completion;
    controller->submit(action);
}

void LumaPlayerLogicController::finishReset()
{
    if (m_resetRevision != 0 && m_resetRevision == m_state.m_revision &&
        m_resetViewApplied && m_resetRateApplied)
    {
        m_state.m_dirty = false;
        m_resetRevision = 0;
        m_resetRequest = 0;
        publish(LumaActionReset);
        LOGINFO("Reset completed, all view and rate conditions confirmed");
    }
}

void LumaPlayerLogicController::complete(const LumaPlayerCoreCCompletion& completion)
{
    std::map<uint64_t, LumaPlayerLogicAction>::iterator it = m_pending.find(completion.m_requestId);
    if (it == m_pending.end())
    {
        return;
    }
    const LumaPlayerLogicAction action = it->second;
    m_pending.erase(it);
    if (m_seekRequest == completion.m_requestId)
    {
        m_seekRequest = 0;
    }
    if (m_rateRequest == completion.m_requestId)
    {
        m_rateRequest = 0;
    }
    if (m_playRequest == completion.m_requestId)
    {
        m_playRequest = 0;
    }
    if (completion.m_operation != LumaPlayerCoreCOperationPreview)
    {
        LOGINFO("Request completed id=%llu generation=%llu operation=%d result=%d",
            static_cast<unsigned long long>(completion.m_requestId),
            static_cast<unsigned long long>(completion.m_mediaGeneration),
            completion.m_operation, completion.m_result);
    }
    if (completion.m_mediaGeneration != m_state.m_mediaGeneration)
    {
        return;
    }
    if (completion.m_requestId == m_dragPreviewRequest)
    {
        m_dragPreviewRequest = 0;
        if (m_dragActive && m_dragLatest.m_revision != action.m_revision)
        {
            flushDrag();
        }
    }
    if (completion.m_operation == LumaPlayerCoreCOperationPreview &&
        completion.m_result == LumaPlayerCoreCResultCanceled)
    {
        return;
    }
    const bool success = completion.m_result == LumaPlayerCoreCResultSuccess;
    if (action.m_type == LumaActionDragBegin && action.m_revision == m_dragSerial && m_dragActive)
    {
        m_dragPaused = success;
        if (success)
        {
            flushDrag();
        }
        else
        {
            m_dragActive = false;
            m_dragDeferred.clear();
        }
    }
    if (completion.m_requestId == m_dragCommitRequest)
    {
        m_dragCommitRequest = 0;
        LOGINFO("Drag committed input=%llu result=%d position=%lld previews=%llu",
            static_cast<unsigned long long>(action.m_revision), completion.m_result,
            static_cast<long long>(completion.m_snapshot.m_position100ns),
            static_cast<unsigned long long>(m_dragPreviewCount));
        std::deque<LumaPlayerLogicAction> deferred;
        deferred.swap(m_dragDeferred);
        if (success)
        {
            for (size_t i = 0; i < deferred.size(); ++i)
            {
                if (deferred[i].m_type == LumaActionCancelMove ||
                    (deferred[i].m_operation >= LumaPlayerCoreCOperationSetA && deferred[i].m_operation <= LumaPlayerCoreCOperationMoveLoop))
                {
                    executeLoop(deferred[i]);
                }
                else
                {
                    executePlayback(deferred[i]);
                }
            }
        }
        else if (!deferred.empty())
        {
            LOGWARNING("Discarding deferred actions after failed drag count=%u", static_cast<uint32_t>(deferred.size()));
        }
    }
    // 完成快照只用于判断该请求结果；界面显示读取当前状态，避免迟到结果倒退画面
    m_state.m_snapshot = m_core->snapshot();
    if (action.m_type == LumaActionReset && action.m_revision == m_resetRevision &&
        completion.m_requestId == m_resetRequest)
    {
        m_resetRequest = 0;
        m_resetRateApplied = success &&
            completion.m_snapshot.m_ratePermille == g_config.m_coreConfig.m_defaultRatePermille;
        if (!m_resetRateApplied)
        {
            m_resetRevision = 0;
            publish(LumaActionReset, success ? LumaPlayerCoreCResultStateError : completion.m_result);
        }
        finishReset();
    }
    else if (action.m_type != LumaActionReset &&
        completion.m_operation == LumaPlayerCoreCOperationRate && success)
    {
        m_state.m_dirty = true;
    }
    if (completion.m_operation == LumaPlayerCoreCOperationOpen && success)
    {
        m_state.m_zoom = 100;
        m_state.m_pan = QPointF(0, 0);
        m_state.m_dirty = false;
        ++m_state.m_revision;
        publish(LumaActionLoad, 0, completion.m_requestId, completion.m_operation);
        LumaPlayerLogicAction play;
        play.m_type = LumaActionCore;
        play.m_operation = LumaPlayerCoreCOperationPlay;
        submitCore(play);
    }
    else
    {
        publish(LumaActionCompleted, completion.m_result, completion.m_requestId,
            completion.m_operation, action.m_point, action.m_revision);
    }
}

void LumaPlayerLogicController::execute(const LumaPlayerLogicAction& action)
{
    try
    {
        if (action.m_type == LumaActionShutdown)
        {
            std::shared_ptr<CTaskThread> lifecycle = CTaskThreadManager::Instance().GetThreadInterface(m_lifecycleThreadId);
            if (lifecycle == nullptr)
            {
                m_closing.store(false);
                publish(LumaActionShutdown, LumaPlayerCoreCResultThreadUnavailable);
                return;
            }
            lifecycle->PostTask(std::make_shared<LumaPlayerLifecycleTask>(this, true), 1);
            return;
        }
        if (m_closing.load())
        {
            return;
        }
        if (action.m_type == LumaActionInitialized)
        {
            m_ready = action.m_value == 0;
            publish(LumaActionInitialized, static_cast<int32_t>(action.m_value));
            if (m_ready && !m_deferredPath.isEmpty())
            {
                LumaPlayerLogicAction load;
                load.m_type = LumaActionLoad;
                load.m_path = m_deferredPath;
                m_deferredPath.clear();
                executeLoad(load);
            }
            return;
        }

        if (action.m_type == LumaActionCompleted)
        {
            complete(action.m_completion);
            return;
        }

        if (!m_ready)
        {
            return;
        }
        if (action.m_type == LumaActionPoll)
        {
            m_state.m_snapshot = m_core->snapshot();
            publish(LumaActionPoll);
            return;
        }
        if (action.m_type == LumaActionViewApplied)
        {
            if (action.m_revision == m_resetRevision)
            {
                m_resetViewApplied = action.m_flag;
                if (!action.m_flag)
                {
                    m_resetRevision = 0;
                    m_resetRequest = 0;
                }
                finishReset();
            }
            return;
        }

    }
    catch (...)
    {
        if (action.m_type == LumaActionShutdown)
        {
            m_closing.store(false);
        }
        LOGERROR("Logic action failed, action=%d", static_cast<int32_t>(action.m_type));
        publish(action.m_type, LumaPlayerCoreCResultInternalError);
    }
}

void LumaPlayerLogicController::executeLoad(const LumaPlayerLogicAction& action)
{
    if (m_closing.load())
    {
        return;
    }
    try
    {
        if (action.m_type == LumaActionLoad && !m_ready)
        {
            m_deferredPath = action.m_path;
            return;
        }
        LumaPlayerLogicAction operation = action;
        if (action.m_type == LumaActionLoad)
        {
            m_dragActive = false;
            m_dragCommitRequest = 0;
            m_dragDeferred.clear();
            ++m_state.m_mediaGeneration;
            m_seekRequest = 0;
            m_rateRequest = 0;
            m_playRequest = 0;
            m_resetRevision = 0;
            operation.m_operation = LumaPlayerCoreCOperationOpen;
            publish(LumaActionLoad, 0, 0, 0);
        }
        submitCore(operation);
    }
    catch (...)
    {
        LOGERROR("Business task failed, action=%d", static_cast<int32_t>(action.m_type));
        publish(LumaActionCompleted, LumaPlayerCoreCResultInternalError, 0,
            action.m_operation, action.m_point, action.m_revision);
    }
}

void LumaPlayerLogicController::executeView(const LumaPlayerLogicAction& action)
{
    if (m_closing.load() || !m_ready)
    {
        return;
    }
    try
    {
        if (action.m_type == LumaActionPan || action.m_type == LumaActionZoom)
        {
            if (action.m_viewportGeneration != m_state.m_viewportGeneration)
            {
                return;
            }
            if (action.m_type == LumaActionPan)
            {
                m_state.m_pan = action.m_position;
            }
            else
            {
                const int32_t oldZoom = m_state.m_zoom;
                const int32_t zoom = (std::max)(g_config.m_minZoomPercent,
                    (std::min)(g_config.m_maxZoomPercent, oldZoom + static_cast<int32_t>(action.m_value)));
                if (zoom == oldZoom)
                {
                    return;
                }
                const QPointF anchor = action.m_position - action.m_viewport.center();
                m_state.m_pan = anchor - (anchor - m_state.m_pan) * (static_cast<double>(zoom) / oldZoom);
                m_state.m_zoom = zoom;
            }
            m_state.m_dirty = true;
            ++m_state.m_revision;
            m_resetRevision = 0;
            publish(action.m_type);
            return;
        }
    }
    catch (...)
    {
        LOGERROR("Business task failed, action=%d", static_cast<int32_t>(action.m_type));
        publish(LumaActionCompleted, LumaPlayerCoreCResultInternalError, 0,
            action.m_operation, action.m_point, action.m_revision);
    }
}

void LumaPlayerLogicController::executeReset(const LumaPlayerLogicAction& action)
{
    if (m_closing.load() || !m_ready)
    {
        return;
    }
    try
    {
        if (action.m_type == LumaActionReset)
        {
            if (!m_state.m_dirty || m_resetRevision != 0)
            {
                return;
            }
            m_state.m_pan = QPointF(0, 0);
            m_state.m_zoom = 100;
            m_resetRevision = ++m_state.m_revision;
            m_resetViewApplied = false;
            m_resetRateApplied = false;
            publish(LumaActionReset);
            LumaPlayerLogicAction rate = action;
            rate.m_revision = m_resetRevision;
            rate.m_operation = LumaPlayerCoreCOperationRate;
            rate.m_value = g_config.m_coreConfig.m_defaultRatePermille;
            m_resetRequest = submitCore(rate);
            if (m_resetRequest == 0)
            {
                m_resetRevision = 0;
                publish(LumaActionReset, LumaPlayerCoreCResultCommandQueueFailed);
            }
            return;
        }
    }
    catch (...)
    {
        LOGERROR("Business task failed, action=%d", static_cast<int32_t>(action.m_type));
        publish(LumaActionCompleted, LumaPlayerCoreCResultInternalError, 0,
            action.m_operation, action.m_point, action.m_revision);
    }
}

void LumaPlayerLogicController::executeLoop(const LumaPlayerLogicAction& action)
{
    if (m_closing.load() || !m_ready)
    {
        return;
    }
    try
    {
        if (deferForDrag(action))
        {
            return;
        }
        if (action.m_type == LumaActionCancelMove)
        {
            m_core->cancelLoopPointMove();
            return;
        }
        submitCore(action);
    }
    catch (...)
    {
        LOGERROR("Business task failed, action=%d", static_cast<int32_t>(action.m_type));
        publish(LumaActionCompleted, LumaPlayerCoreCResultInternalError, 0,
            action.m_operation, action.m_point, action.m_revision);
    }
}

void LumaPlayerLogicController::executePlayback(const LumaPlayerLogicAction& action)
{
    if (m_closing.load() || !m_ready)
    {
        return;
    }
    try
    {
        if (deferForDrag(action))
        {
            return;
        }
        LumaPlayerLogicAction operation = action;
        if (action.m_type == LumaActionTogglePlayback)
        {
            const bool playing = m_playRequest != 0 ? m_playTarget :
                m_core->snapshot().m_state == LumaPlayerCoreCStatePlaying;
            operation.m_operation = playing ? LumaPlayerCoreCOperationPause : LumaPlayerCoreCOperationPlay;
        }
        if (action.m_type == LumaActionRateDelta)
        {
            const int64_t target = (m_rateRequest != 0 ? m_rateTarget : m_core->snapshot().m_ratePermille) + action.m_value;
            operation.m_operation = LumaPlayerCoreCOperationRate;
            operation.m_value = (std::max<int64_t>)(g_config.m_coreConfig.m_minRatePermille,
                (std::min<int64_t>)(g_config.m_coreConfig.m_maxRatePermille, target));
            if (operation.m_value == (m_rateRequest != 0 ? m_rateTarget : m_core->snapshot().m_ratePermille))
            {
                return;
            }
            ++m_state.m_revision;
            m_resetRevision = 0;
        }
        submitCore(operation);
    }
    catch (...)
    {
        LOGERROR("Business task failed, action=%d", static_cast<int32_t>(action.m_type));
        publish(LumaActionCompleted, LumaPlayerCoreCResultInternalError, 0,
            action.m_operation, action.m_point, action.m_revision);
    }
}

void LumaPlayerLogicController::executeWindow(const LumaPlayerLogicAction& action)
{
    if (m_closing.load())
    {
        return;
    }
    try
    {
        if (action.m_type == LumaActionWindowApplied)
        {
            LOGINFO("GUI action applied action=%d success=%d", action.m_point, action.m_flag ? 1 : 0);
            publish(LumaActionWindowApplied, action.m_flag ? 0 : LumaPlayerCoreCResultStateError);
            return;
        }
        if (action.m_type == LumaActionHelpClose)
        {
            publish(LumaActionHelpClose, 0, action.m_revision);
            return;
        }
        if (action.m_type == LumaActionHelp || action.m_type == LumaActionMinimize ||
            action.m_type == LumaActionMaximize || action.m_type == LumaActionFullscreen)
        {
            publish(action.m_type);
            return;
        }
        if (action.m_type == LumaActionViewport)
        {
            m_state.m_viewportGeneration = action.m_viewportGeneration;
            publish(LumaActionViewport);
            return;
        }
        if (action.m_type == LumaActionPin)
        {
            m_state.m_pinned = !m_state.m_pinned;
            publish(LumaActionPin);
            return;
        }
    }
    catch (...)
    {
        LOGERROR("Business task failed, action=%d", static_cast<int32_t>(action.m_type));
        publish(LumaActionCompleted, LumaPlayerCoreCResultInternalError, 0,
            action.m_operation, action.m_point, action.m_revision);
    }
}

void LumaPlayerLogicController::executeSeek(const LumaPlayerLogicAction& action)
{
    if (m_closing.load() || !m_ready)
    {
        return;
    }
    try
    {
        if (action.m_type == LumaActionDragBegin)
        {
            m_dragActive = true;
            m_dragPaused = false;
            m_dragFinishing = false;
            m_dragSerial = action.m_revision;
            m_dragCommitRequest = 0;
            m_dragPreviewRequest = 0;
            m_dragPreviewRevision = 0;
            m_dragPreviewCount = 0;
            m_dragDeferred.clear();
            m_dragLatest = action;
            m_dragLatest.m_operation = LumaPlayerCoreCOperationPreview;
            LumaPlayerLogicAction pause = action;
            pause.m_operation = LumaPlayerCoreCOperationPause;
            LOGINFO("Drag begin input=%llu", static_cast<unsigned long long>(m_dragSerial));
            if (submitCore(pause) == 0)
            {
                m_dragActive = false;
            }
            return;
        }
        if (action.m_type == LumaActionDragPreview || action.m_type == LumaActionDragCommit)
        {
            if (!m_dragActive || m_dragFinishing)
            {
                return;
            }
            m_dragLatest = action;
            m_dragFinishing = action.m_type == LumaActionDragCommit;
            flushDrag();
            return;
        }
        LumaPlayerLogicAction operation = action;
        if (action.m_type == LumaActionJump)
        {
            const LumaPlayerCoreCSnapshot snapshot = m_core->snapshot();
            const int64_t origin = m_dragActive ? m_dragLatest.m_value :
                m_seekRequest != 0 ? m_seekTarget : snapshot.m_position100ns;
            const long double target = static_cast<long double>(origin) +
                action.m_value * static_cast<long double>(g_config.m_keyboardSeekSeconds) * g_config.m_second100ns;
            operation.m_operation = LumaPlayerCoreCOperationSeek;
            operation.m_value = target <= 0 ? 0 : target >= snapshot.m_duration100ns ?
                snapshot.m_duration100ns : static_cast<int64_t>(target);
            operation.m_flag = m_seekRequest != 0 ? m_seekPlaying : snapshot.m_state == LumaPlayerCoreCStatePlaying;
        }
        m_dragActive = false;
        m_dragDeferred.clear();
        LOGINFO("Position task input=%llu target=%lld resume=%d",
            static_cast<unsigned long long>(action.m_revision), static_cast<long long>(operation.m_value), operation.m_flag ? 1 : 0);
        submitCore(operation);
    }
    catch (...)
    {
        LOGERROR("Business task failed, action=%d", static_cast<int32_t>(action.m_type));
        publish(LumaActionCompleted, LumaPlayerCoreCResultInternalError, 0,
            action.m_operation, action.m_point, action.m_revision);
    }
}

void LumaPlayerLogicController::executeLifecycle(bool shutdown, const std::atomic<bool>* exitFlag)
{
    if (!shutdown)
    {
        int32_t result = LumaPlayerCoreCResultCanceled;
        try
        {
            if (exitFlag == nullptr || !exitFlag->load())
            {
                result = m_core->init(g_config.m_debugEnabled);
            }
        }
        catch (...)
        {
            result = LumaPlayerCoreCResultInternalError;
        }
        LumaPlayerLogicAction initialized;
        initialized.m_type = LumaActionInitialized;
        initialized.m_value = result;
        submit(initialized);
        return;
    }
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    uint32_t logicId = 0;
    {
        std::lock_guard<std::mutex> lock(m_submitMutex);
        logicId = m_logicThreadId;
        m_logicThreadId = 0;
    }
    LOGINFO("Shutdown begin logicThread=%u lifecycleThread=%u", logicId, m_lifecycleThreadId);
    if (logicId != 0)
    {
        CTaskThreadManager::Instance().Uninit(logicId);
    }
    LOGINFO("Shutdown logic complete elapsedMs=%lld, Core begin", static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()));
    m_core->uninit();
    LOGINFO("Shutdown Core and audio complete elapsedMs=%lld, canceledRequests=%u", static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()), static_cast<uint32_t>(m_pending.size()));
    m_pending.clear();
    LumaPlayerLogicResult result;
    result.m_type = LumaActionShutdown;
    emit resultReady(result);
}

void LumaPlayerLogicController::flushDrag()
{
    if (!m_dragActive || !m_dragPaused)
    {
        return;
    }
    LumaPlayerLogicAction request = m_dragLatest;
    request.m_operation = m_dragFinishing ? LumaPlayerCoreCOperationSeek : LumaPlayerCoreCOperationPreview;
    if (m_dragFinishing)
    {
        m_dragActive = false;
        request.m_type = LumaActionDragCommit;
        m_dragCommitRequest = submitCore(request);
        if (m_dragCommitRequest == 0)
        {
            m_dragDeferred.clear();
        }
    }
    else if (m_dragPreviewRequest == 0 && m_dragPreviewRevision != request.m_revision)
    {
        // 输入只覆盖最新目标，不持续取消正在解码的一帧，避免高频拖动时预览饥饿。
        request.m_type = LumaActionDragPreview;
        m_dragPreviewRevision = request.m_revision;
        m_dragPreviewRequest = submitCore(request);
        if (m_dragPreviewRequest != 0)
        {
            ++m_dragPreviewCount;
        }
    }
}

bool LumaPlayerLogicController::deferForDrag(const LumaPlayerLogicAction& action)
{
    if (action.m_type == LumaActionCancelMove || (!m_dragActive && m_dragCommitRequest == 0))
    {
        return false;
    }
    if (m_dragDeferred.size() >= static_cast<size_t>(g_config.m_logicQueueLimit))
    {
        publish(LumaActionCompleted, LumaPlayerCoreCResultCommandQueueFailed, 0,
            action.m_operation, action.m_point, action.m_revision);
        return true;
    }
    m_dragDeferred.push_back(action);
    if (m_dragActive)
    {
        m_dragFinishing = true;
        m_dragLatest.m_flag = false;
        flushDrag();
    }
    return true;
}