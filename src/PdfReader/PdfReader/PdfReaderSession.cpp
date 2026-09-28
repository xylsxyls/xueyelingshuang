#include "PdfReaderSession.h"
#include "PdfReaderTaskManager.h"
#include "Config.h"
#include <algorithm>

PdfReaderSession::PdfReaderSession(QObject* receiver) : m_state(new PdfReaderSessionState(receiver)),
m_nextId(0), m_businessId(0), m_maxPending(g_config.m_maxPendingRenders),
m_maxPendingPixels(g_config.m_core.maxRenderPixels), m_pendingPixels(0)
{

}

PdfReaderSession::~PdfReaderSession()
{
    {
        std::lock_guard<std::mutex> lock(m_state->m_mutex);
        m_state->m_receiver = nullptr;
        m_state->m_results.clear();
    }
    close(std::function<void(const PdfReaderResult&)>());
}

uint64_t PdfReaderSession::submit(PdfReaderRequest request, const std::function<void(const PdfReaderResult&)>& completion)
{
    const bool render = request.m_operation == PdfReaderRender;
    // Oversized requests still reach Core for an explicit error, reserving at most
    // the per-session budget; valid images never exceed this total pixel count.
    const uint64_t pixels = render && request.m_size.width() > 0 && request.m_size.height() > 0 ?
        (std::min)(static_cast<uint64_t>(request.m_size.width()) * request.m_size.height(), m_maxPendingPixels) : 0;
    if (m_state->m_closing.load() || (!render && m_businessId) ||
        (render && (m_businessId || m_pending.size() >= static_cast<size_t>(m_maxPending) ||
            pixels > m_maxPendingPixels - m_pendingPixels)))
    {
        return 0;
    }
    if (!render)
    {
        invalidateRenders();
    }
    request.m_id = ++m_nextId;
    request.m_epoch = m_state->m_epoch.load();
    m_pending.insert(std::make_pair(request.m_id, completion));
    if (render)
    {
        m_pixelReservations.insert(std::make_pair(request.m_id, pixels));
        m_pendingPixels += pixels;
    }
    if (!render)
    {
        m_businessId = request.m_id;
    }
    const std::shared_ptr<PdfReaderSessionState> state = m_state;
    try
    {
        PdfReaderTaskManager::instance().postLogic(std::shared_ptr<CTask>(new PdfReaderTask(
            [state, request](const std::atomic<bool>&) { state->dispatch(request); },
            [state, request](const std::string& error) {
                std::shared_ptr<PdfReaderResult> result(new PdfReaderResult());
                result->m_request = request;
                state->fail(result, error);
            })));
    }
    catch (...)
    {
        m_pending.erase(request.m_id);
        if (render)
        {
            m_pendingPixels -= pixels;
            m_pixelReservations.erase(request.m_id);
        }
        if (!render)
        {
            m_businessId = 0;
        }
        return 0;
    }
    return request.m_id;
}

void PdfReaderSession::processResults()
{
    std::deque<std::shared_ptr<PdfReaderResult>> results;
    {
        std::lock_guard<std::mutex> lock(m_state->m_mutex);
        results.swap(m_state->m_results);
    }
    for (size_t index = 0; index < results.size(); ++index)
    {
        PdfReaderResult& result = *results[index];
        auto pending = m_pending.find(result.m_request.m_id);
        if (pending == m_pending.end())
        {
            continue;
        }
        std::function<void(const PdfReaderResult&)> callback = pending->second;
        m_pending.erase(pending);
        auto reservation = m_pixelReservations.find(result.m_request.m_id);
        if (reservation != m_pixelReservations.end())
        {
            m_pendingPixels -= reservation->second;
            m_pixelReservations.erase(reservation);
        }
        if (m_businessId == result.m_request.m_id)
        {
            m_businessId = 0;
        }
        if (result.m_request.m_operation == PdfReaderRender && result.m_request.m_epoch != m_state->m_epoch.load())
        {
            result.m_cancelled = true;
        }
        if (result.m_hasSnapshot)
        {
            m_pages = result.m_pages;
        }
        if (callback && (!m_state->m_closing.load() || result.m_request.m_operation == PdfReaderClose))
        {
            callback(result);
        }
    }
}

void PdfReaderSession::invalidateRenders()
{
    ++m_state->m_epoch;
}

void PdfReaderSession::close(const std::function<void(const PdfReaderResult&)>& completion)
{
    if (m_state->m_closing.exchange(true))
    {
        return;
    }
    invalidateRenders();
    PdfReaderRequest request;
    request.m_operation = PdfReaderClose;
    request.m_id = ++m_nextId;
    m_pending.insert(std::make_pair(request.m_id, completion));
    const std::shared_ptr<PdfReaderSessionState> state = m_state;
    PdfReaderTaskManager::instance().postLogic(std::shared_ptr<CTask>(new PdfReaderTask(
        [state, request](const std::atomic<bool>&) { state->dispatch(request); },
        [state, request](const std::string& error) {
            std::shared_ptr<PdfReaderResult> result(new PdfReaderResult());
            result->m_request = request;
            state->fail(result, error);
        })));
}

bool PdfReaderSession::isOpen() const
{
    return !m_pages.isEmpty();
}

int32_t PdfReaderSession::pageCount() const
{
    return m_pages.size();
}

bool PdfReaderSession::pageInfo(int32_t index, PdfReaderCoreCPageInfo* info) const
{
    if (!info || index < 0 || index >= m_pages.size())
    {
        return false;
    }
    *info = m_pages[index];
    return true;
}

bool PdfReaderSession::busy() const
{
    return m_businessId != 0 || m_state->m_closing.load();
}

bool PdfReaderSession::idle() const
{
    return m_pending.empty();
}