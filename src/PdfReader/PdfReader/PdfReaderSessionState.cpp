#include "PdfReaderSessionState.h"
#include "PdfReaderTaskManager.h"
#include "Config.h"
#include "LogManager/LogManagerAPI.h"
#include <QObject>

PdfReaderSessionState::PdfReaderSessionState(QObject* receiver) :
m_sessionId(PdfReaderTaskManager::instance().nextSessionId()),
m_epoch(1), m_closing(false), m_receiver(receiver), m_core(new PdfReaderCoreBridge()),
m_config(g_config.m_core), m_initialized(false)
{

}

void PdfReaderSessionState::dispatch(const PdfReaderRequest& request)
{
    std::shared_ptr<PdfReaderResult> result(new PdfReaderResult());
    result->m_request = request;
    const std::shared_ptr<PdfReaderSessionState> self = shared_from_this();
    try
    {
        PdfReaderTaskManager::instance().postWork(std::shared_ptr<CTask>(new PdfReaderTask(
            [self, result](const std::atomic<bool>& exit) {
                self->execute(result, exit);
                PdfReaderTaskManager::instance().postLogic(std::shared_ptr<CTask>(new PdfReaderTask(
                    [self, result](const std::atomic<bool>&) { self->deliver(result); },
                    [self, result](const std::string& error) { self->fail(result, error); })));
            }, [self, result](const std::string& error) { self->fail(result, error); })));
    }
    catch (const std::exception& error)
    {
        fail(result, error.what());
    }
}

void PdfReaderSessionState::execute(const std::shared_ptr<PdfReaderResult>& result, const std::atomic<bool>& exit)
{
    const PdfReaderRequest& request = result->m_request;
    if (request.m_operation == PdfReaderClose)
    {
        m_core.reset();
        m_initialized = false;
        result->m_success = true;
        result->m_code = PdfReaderCoreCResultSuccess;
        LOGINFO("PdfReader close complete session=%llu request=%llu", m_sessionId, request.m_id);
        return;
    }
    if (request.m_operation == PdfReaderRender &&
        (exit.load() || m_closing.load() || request.m_epoch != m_epoch.load()))
    {
        result->m_cancelled = true;
        return;
    }
    if (!m_initialized)
    {
        m_initialized = m_core->init(m_config, &result->m_error);
        if (!m_initialized)
        {
            result->m_code = m_core->lastResult();
            return;
        }
    }
    switch (request.m_operation)
    {
    case PdfReaderOpen:
        result->m_success = m_core->open(request.m_path, request.m_password, &result->m_error);
        break;
    case PdfReaderInsert:
        result->m_success = m_core->insertDocument(request.m_path, request.m_password, request.m_index, &result->m_error);
        break;
    case PdfReaderMove:
        result->m_success = m_core->movePage(request.m_index, request.m_target, &result->m_error);
        break;
    case PdfReaderSave:
        result->m_success = m_core->saveTo(request.m_path, &result->m_error);
        break;
    case PdfReaderSaveMain:
        result->m_success = m_core->saveToMain(&result->m_error);
        break;
    case PdfReaderValidateRange:
        result->m_success = m_core->validatePageRange(request.m_text, &result->m_error);
        break;
    case PdfReaderSaveRange:
        result->m_success = m_core->savePageRange(request.m_text, request.m_path, &result->m_error);
        break;
    case PdfReaderSaveEach:
        result->m_success = m_core->saveEachPage(request.m_path, request.m_text, &result->m_error, request.m_overwrite);
        break;
    case PdfReaderRender:
        result->m_image = m_core->renderPage(request.m_index, request.m_size.width(), request.m_size.height(), &result->m_error);
        result->m_success = !result->m_image.isNull();
        break;
    default:
        break;
    }
    result->m_code = m_core->lastResult();
    if ((result->m_success && (request.m_operation == PdfReaderOpen || request.m_operation == PdfReaderInsert ||
        request.m_operation == PdfReaderMove)) || request.m_operation == PdfReaderSaveMain)
    {
        result->m_pages.reserve(m_core->pageCount());
        for (int32_t index = 0; index < m_core->pageCount(); ++index)
        {
            PdfReaderCoreCPageInfo info = {};
            if (!m_core->pageInfo(index, &info, &result->m_error))
            {
                result->m_success = false;
                result->m_code = m_core->lastResult();
                break;
            }
            result->m_pages.push_back(info);
        }
        result->m_hasSnapshot = true;
    }
    LOGINFO("PdfReader completed session=%llu request=%llu epoch=%llu operation=%d success=%d", m_sessionId, request.m_id,
        request.m_epoch, static_cast<int>(request.m_operation), result->m_success ? 1 : 0);
}

void PdfReaderSessionState::deliver(const std::shared_ptr<PdfReaderResult>& result)
{
    if (result->m_request.m_operation == PdfReaderRender &&
        (m_closing.load() || result->m_request.m_epoch != m_epoch.load()))
    {
        result->m_cancelled = true;
        result->m_image = QImage();
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_receiver)
    {
        m_results.push_back(result);
        QMetaObject::invokeMethod(m_receiver, "processCoreResults", Qt::QueuedConnection);
    }
}

void PdfReaderSessionState::fail(const std::shared_ptr<PdfReaderResult>& result, const std::string& error)
{
    result->m_success = false;
    result->m_code = PdfReaderCoreCResultInternalError;
    result->m_error = QString::fromUtf8(error.c_str());
    deliver(result);
}