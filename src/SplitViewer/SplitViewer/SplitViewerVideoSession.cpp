#include "SplitViewerVideoSession.h"
#include "SplitViewerForeignWindowHelper.h"
#include "Config.h"
#include "CSystem/CSystemAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QVariant>
#include <QtWidgets/QWidget>
#include <vector>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

SplitViewerVideoSession::SplitViewerVideoSession(QObject* parent) :
QObject(parent),
m_timer(new QTimer(this)),
m_nextId(0),
m_stopping(false)
{
    m_clock.start();
    m_timer->setInterval(g_config.m_videoPollIntervalMs);
    connect(m_timer, &QTimer::timeout, this, &SplitViewerVideoSession::poll);
}

SplitViewerVideoSession::~SplitViewerVideoSession()
{
    m_timer->stop();
    foreach (const SplitViewerVideoRequest& request, m_embeddedRequests)
    {
        if (running(request.m_processToken))
        {
            forceClose(request.m_processToken);
        }
        releaseProcess(request.m_processToken);
    }
    foreach (const SplitViewerVideoRequest& request, m_requests)
    {
        if (running(request.m_processToken))
        {
            forceClose(request.m_processToken);
        }
        releaseProcess(request.m_processToken);
    }
}

quintptr SplitViewerVideoSession::captureProcess(qint64 processId)
{
#ifdef Q_OS_WIN
    if (processId <= 0 || processId > MAXDWORD)
    {
        return 0;
    }
    return reinterpret_cast<quintptr>(OpenProcess(
        SYNCHRONIZE | PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(processId)));
#else
    Q_UNUSED(processId);
    return 0;
#endif
}

bool SplitViewerVideoSession::running(quintptr token)
{
#ifdef Q_OS_WIN
    return token && WaitForSingleObject(reinterpret_cast<HANDLE>(token), 0) == WAIT_TIMEOUT;
#else
    Q_UNUSED(token);
    return false;
#endif
}

void SplitViewerVideoSession::releaseProcess(quintptr token)
{
#ifdef Q_OS_WIN
    if (token)
    {
        CloseHandle(reinterpret_cast<HANDLE>(token));
    }
#else
    Q_UNUSED(token);
#endif
}

bool SplitViewerVideoSession::forceClose(quintptr token)
{
#ifdef Q_OS_WIN
    return token && TerminateProcess(reinterpret_cast<HANDLE>(token), 0) != FALSE;
#else
    Q_UNUSED(token);
    return false;
#endif
}

quint64 SplitViewerVideoSession::start(const QString& program, const QString& path)
{
    if (m_stopping || m_requests.size() >= g_config.m_maximumPendingVideos)
    {
        return 0;
    }
    SplitViewerVideoRequest request;
    request.m_id = ++m_nextId;
    request.m_program = program;
    request.m_path = path;
    request.m_deadline = m_clock.elapsed() + g_config.m_videoStartTimeoutMs;
    m_requests.insert(request.m_id, request);
    m_timer->start();
    return request.m_id;
}

bool SplitViewerVideoSession::launchProcess(SplitViewerVideoRequest& request)
{
#ifdef Q_OS_WIN
    const std::wstring program = request.m_program.toStdWString();
    const std::wstring directory = QFileInfo(request.m_program).absolutePath().toStdWString();
    const std::wstring command = L"\"" + program + L"\" \"" + request.m_path.toStdWString() + L"\"";
    std::vector<wchar_t> buffer(command.begin(), command.end());
    buffer.push_back(L'\0');
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process = {};
    if (!CreateProcessW(program.c_str(), buffer.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr,
            directory.c_str(), &startup, &process))
    {
        return false;
    }
    CloseHandle(process.hThread);
    request.m_processId = process.dwProcessId;
    request.m_processToken = reinterpret_cast<quintptr>(process.hProcess);
    return true;
#else
    Q_UNUSED(request);
    return false;
#endif
}

void SplitViewerVideoSession::finish(quint64 id)
{
    if (!m_requests.contains(id))
    {
        return;
    }
    releaseProcess(m_requests.take(id).m_processToken);
    if (m_requests.isEmpty())
    {
        if (m_embeddedRequests.isEmpty())
        {
            m_timer->stop();
        }
        emit idle();
    }
}

void SplitViewerVideoSession::accept(quint64 id)
{
    if (m_requests.contains(id) && m_requests.value(id).m_phase == SPLITVIEWER_VIDEO_READY)
    {
        const SplitViewerVideoRequest request = m_requests.value(id);
        const std::wstring name = QFileInfo(request.m_program).fileName().toLower().toStdWString();
        if (g_config.m_closeEmbeddedProcessNames.find(name) == g_config.m_closeEmbeddedProcessNames.end())
        {
            finish(id);
            return;
        }
        m_embeddedRequests.insert(request.m_windowId, m_requests.take(id));
        LOGINFO("Keep linked embedded process identity; pid=%lld", request.m_processId);
        m_timer->start();
        if (m_requests.isEmpty())
        {
            emit idle();
        }
    }
}

void SplitViewerVideoSession::beginClose(quint64 id)
{
    if (!m_requests.contains(id))
    {
        return;
    }
    SplitViewerVideoRequest& request = m_requests[id];
    if (!request.m_processToken || !running(request.m_processToken))
    {
        finish(id);
        return;
    }
    request.m_phase = SPLITVIEWER_VIDEO_CLOSING;
    request.m_deadline = m_clock.elapsed() + g_config.m_videoCloseTimeoutMs;
#ifdef Q_OS_WIN
    HWND window = reinterpret_cast<HWND>(request.m_windowId);
    if (!window || !IsWindow(window))
    {
        window = CSystem::findVisibleWindowByProcessId(static_cast<uint32_t>(request.m_processId));
    }
    DWORD windowProcess = 0;
    if (window && IsWindow(window))
    {
        GetWindowThreadProcessId(window, &windowProcess);
        if (windowProcess == request.m_processId)
        {
            PostMessageW(window, WM_CLOSE, 0, 0);
        }
    }
#endif
    m_timer->start();
}

void SplitViewerVideoSession::cancel(quint64 id)
{
    if (m_requests.contains(id) && m_requests.value(id).m_phase != SPLITVIEWER_VIDEO_CLOSING)
    {
        beginClose(id);
    }
}

bool SplitViewerVideoSession::releaseWindow(QWidget* container)
{
    if (!container)
    {
        return false;
    }
    const WId windowId = static_cast<WId>(container->property("foreignId").toULongLong());
#ifdef Q_OS_WIN
    if (m_embeddedRequests.contains(windowId))
    {
        const SplitViewerVideoRequest request = m_embeddedRequests.take(windowId);
        m_requests.insert(request.m_id, request);
        SplitViewerForeignWindowHelper::detachForeignWindow(container, false);
        LOGINFO("Close retained embedded process; pid=%lld", request.m_processId);
        beginClose(request.m_id);
        return false;
    }
    foreach (const SplitViewerVideoRequest& request, m_requests)
    {
        if (request.m_windowId == windowId && request.m_phase == SPLITVIEWER_VIDEO_CLOSING)
        {
            SplitViewerForeignWindowHelper::detachForeignWindow(container, false);
            return false;
        }
    }
    const HWND window = reinterpret_cast<HWND>(windowId);
    DWORD processId = 0;
    if (IsWindow(window))
    {
        GetWindowThreadProcessId(window, &processId);
    }
    const std::wstring name = QString::fromStdWString(CSystem::processNameW(processId)).toLower().toStdWString();
    if (!name.empty() && g_config.m_closeEmbeddedProcessNames.find(name) != g_config.m_closeEmbeddedProcessNames.end())
    {
        const quintptr token = captureProcess(processId);
        DWORD confirmedProcess = 0;
        GetWindowThreadProcessId(window, &confirmedProcess);
        if (token && running(token) && confirmedProcess == processId)
        {
            SplitViewerVideoRequest request;
            request.m_id = ++m_nextId;
            request.m_processId = processId;
            request.m_processToken = token;
            request.m_windowId = windowId;
            m_requests.insert(request.m_id, request);
            SplitViewerForeignWindowHelper::detachForeignWindow(container, false);
            beginClose(request.m_id);
            return false;
        }
        releaseProcess(token);
        LOGERROR(
            "Could not acquire embedded process identity for close; pid=%lu", static_cast<unsigned long>(processId));
        if (IsWindow(window) && confirmedProcess == processId)
        {
            // 权限不足时仍尝试正常关闭；不恢复独立播放器，不按名称强杀。
            PostMessageW(window, WM_CLOSE, 0, 0);
            SplitViewerForeignWindowHelper::detachForeignWindow(container, false);
            emit failed(0, g_config.m_videoCloseError);
            return false;
        }
    }
#else
    Q_UNUSED(windowId);
#endif
    const bool valid = SplitViewerForeignWindowHelper::foreignWindowValid(container);
    SplitViewerForeignWindowHelper::detachForeignWindow(container);
    return valid;
}

bool SplitViewerVideoSession::trackWindow(WId windowId)
{
#ifdef Q_OS_WIN
    foreach (const SplitViewerVideoRequest& request, m_requests)
    {
        if (request.m_windowId == windowId && request.m_phase == SPLITVIEWER_VIDEO_READY)
        {
            return true;
        }
    }
    const HWND window = reinterpret_cast<HWND>(windowId);
    DWORD processId = 0;
    if (!IsWindow(window) || m_embeddedRequests.contains(windowId))
    {
        return false;
    }
    GetWindowThreadProcessId(window, &processId);
    const std::wstring name = QString::fromStdWString(CSystem::processNameW(processId)).toLower().toStdWString();
    if (g_config.m_closeEmbeddedProcessNames.find(name) == g_config.m_closeEmbeddedProcessNames.end())
    {
        return true;
    }
    SplitViewerVideoRequest request;
    request.m_id = ++m_nextId;
    request.m_processId = processId;
    request.m_windowId = windowId;
    request.m_processToken = captureProcess(processId);
    if (!windowAvailable(request))
    {
        releaseProcess(request.m_processToken);
        LOGERROR("Cannot retain external embedded process identity; pid=%lu", processId);
        return false;
    }
    m_embeddedRequests.insert(windowId, request);
    m_timer->start();
    LOGINFO("Keep external embedded process identity; pid=%lu", processId);
#else
    Q_UNUSED(windowId);
#endif
    return true;
}

bool SplitViewerVideoSession::windowAvailable(const SplitViewerVideoRequest& request)
{
#ifdef Q_OS_WIN
    const HWND window = reinterpret_cast<HWND>(request.m_windowId);
    DWORD processId = 0;
    if (!running(request.m_processToken) || !IsWindow(window))
    {
        return false;
    }
    GetWindowThreadProcessId(window, &processId);
    return processId == request.m_processId && (GetWindowLongPtrW(window, GWL_STYLE) & WS_VISIBLE) != 0;
#else
    Q_UNUSED(request);
    return false;
#endif
}

bool SplitViewerVideoSession::windowValid(QWidget* container) const
{
    if (!container)
    {
        return false;
    }
    const WId windowId = static_cast<WId>(container->property("foreignId").toULongLong());
    if (m_embeddedRequests.contains(windowId))
    {
        return windowAvailable(m_embeddedRequests.value(windowId));
    }
    foreach (const SplitViewerVideoRequest& request, m_requests)
    {
        if (request.m_windowId == windowId && request.m_phase == SPLITVIEWER_VIDEO_CLOSING)
        {
            return false;
        }
    }
    return SplitViewerForeignWindowHelper::foreignWindowValid(container);
}

void SplitViewerVideoSession::shutdown()
{
    m_stopping = true;
    const QList<WId> windows = m_embeddedRequests.keys();
    foreach (WId windowId, windows)
    {
        const SplitViewerVideoRequest request = m_embeddedRequests.take(windowId);
        m_requests.insert(request.m_id, request);
    }
    const QList<quint64> ids = m_requests.keys();
    foreach (quint64 id, ids)
    {
        cancel(id);
    }
}

bool SplitViewerVideoSession::busy() const
{
    return !m_requests.isEmpty();
}

void SplitViewerVideoSession::poll()
{
    const QList<WId> windows = m_embeddedRequests.keys();
    foreach (WId windowId, windows)
    {
        const SplitViewerVideoRequest request = m_embeddedRequests.value(windowId);
        if (!windowAvailable(request))
        {
            m_embeddedRequests.remove(windowId);
            m_requests.insert(request.m_id, request);
            LOGINFO("Embedded window closed or destroyed; retained pid=%lld", request.m_processId);
            beginClose(request.m_id);
        }
    }
    const QList<quint64> ids = m_requests.keys();
    foreach (quint64 id, ids)
    {
        if (!m_requests.contains(id))
        {
            continue;
        }
        SplitViewerVideoRequest request = m_requests.value(id);
        if (request.m_phase == SPLITVIEWER_VIDEO_QUEUED)
        {
#ifdef Q_OS_WIN
            if (!launchProcess(request))
            {
                finish(id);
                emit failed(id, g_config.m_videoStartError + request.m_program);
                continue;
            }
            m_requests[id].m_processId = request.m_processId;
            m_requests[id].m_processToken = request.m_processToken;
            m_requests[id].m_phase = SPLITVIEWER_VIDEO_STARTING;
            request = m_requests.value(id);
#else
            finish(id);
            emit failed(id, g_config.m_videoPlatformError);
            continue;
#endif
        }
        if (!running(request.m_processToken))
        {
            finish(id);
            if (request.m_phase != SPLITVIEWER_VIDEO_CLOSING)
            {
                emit failed(id, g_config.m_videoWindowError + request.m_path);
            }
            continue;
        }
        if (request.m_phase == SPLITVIEWER_VIDEO_CLOSING)
        {
            if (m_clock.elapsed() >= request.m_deadline && !request.m_forced)
            {
                const bool terminated = forceClose(request.m_processToken);
                m_requests[id].m_forced = true;
                m_requests[id].m_deadline = m_clock.elapsed() + g_config.m_videoCloseTimeoutMs;
                LOGWARNING("Embedded process close timed out; pid=%lld force=%d", request.m_processId, terminated);
                if (!terminated)
                {
                    finish(id);
                    emit failed(0, g_config.m_videoCloseError);
                }
            }
            else if (m_clock.elapsed() >= request.m_deadline && request.m_forced)
            {
                LOGERROR("Embedded process did not signal after termination; pid=%lld", request.m_processId);
                finish(id);
                emit failed(0, g_config.m_videoCloseError);
            }
            continue;
        }
#ifdef Q_OS_WIN
        if (request.m_phase == SPLITVIEWER_VIDEO_STARTING)
        {
            const HWND window = CSystem::findVisibleWindowByProcessId(static_cast<uint32_t>(request.m_processId));
            if (window)
            {
                m_requests[id].m_windowId = reinterpret_cast<WId>(window);
                m_requests[id].m_phase = SPLITVIEWER_VIDEO_READY;
                emit windowReady(id, reinterpret_cast<quintptr>(window));
                continue;
            }
        }
#endif
        if (m_requests.contains(id) && m_clock.elapsed() >= request.m_deadline)
        {
            beginClose(id);
            emit failed(id, g_config.m_videoWindowError + request.m_path);
        }
    }
}