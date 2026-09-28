#include "PdfReaderCoreBridge.h"

#include <QByteArray>
#include "LogManager/LogManagerAPI.h"

PdfReaderCoreBridge::PdfReaderCoreBridge()
    : m_createError(g_config.m_bridgeCreateError), m_internalError(g_config.m_bridgeInternalError),
      m_pageError(g_config.m_pageParameterError), m_renderError(g_config.m_renderParameterError),
      m_lastResult(PdfReaderCoreCResultNotInit), m_maxRenderPixels(g_config.m_core.maxRenderPixels), m_handle(nullptr)
{

}

PdfReaderCoreBridge::~PdfReaderCoreBridge()
{
    if (m_handle)
    {
        pdfReaderCoreUninit(m_handle);
        pdfReaderCoreDestroy(m_handle);
        LOGINFO("PdfReader Core destroyed");
        m_handle = nullptr;
    }
}

QString PdfReaderCoreBridge::takeError() const
{
    if (!m_handle)
    {
        return m_createError;
    }
    const size_t required = pdfReaderCoreGetLastError(m_handle, nullptr, 0);
    if (!required)
    {
        return m_internalError;
    }
    QByteArray buffer(static_cast<int>(required), 0);
    pdfReaderCoreGetLastError(m_handle, buffer.data(), static_cast<size_t>(buffer.size()));
    return QString::fromUtf8(buffer.constData());
}

bool PdfReaderCoreBridge::init(QString* errorText)
{
    PdfReaderCoreCConfig config;
    pdfReaderCoreDefaultConfig(&config);
    return init(config, errorText);
}

bool PdfReaderCoreBridge::init(const PdfReaderCoreCConfig& config, QString* errorText)
{
    if (!m_handle)
    {
        m_handle = pdfReaderCoreCreate();
    }
    if (!m_handle)
    {
        m_lastResult = PdfReaderCoreCResultNotInit;
        if (errorText)
        {
            *errorText = m_createError;
        }
        return false;
    }
    const int32_t result = pdfReaderCoreInitWithConfig(m_handle, &config);
    m_lastResult = result;
    if (result == PdfReaderCoreCResultSuccess)
    {
        LOGINFO("PdfReader init completed, result=%d", result);
    }
    else
    {
        LOGERROR("PdfReader init failed, result=%d, error=%s", result, takeError().toUtf8().constData());
    }
    if (result == PdfReaderCoreCResultSuccess)
    {
        m_maxRenderPixels = config.maxRenderPixels;
        return true;
    }
    if (errorText)
    {
        *errorText = takeError();
    }
    return false;
}

void PdfReaderCoreBridge::shutdown()
{
    if (m_handle)
    {
        pdfReaderCoreUninit(m_handle);
    }
}

bool PdfReaderCoreBridge::isOpen() const
{
    return m_handle && pdfReaderCoreIsOpen(m_handle) != 0;
}

int PdfReaderCoreBridge::pageCount() const
{
    return m_handle ? pdfReaderCorePageCount(m_handle) : 0;
}

QString PdfReaderCoreBridge::lastError() const
{
    return takeError();
}

bool PdfReaderCoreBridge::open(const QString& path, const QString& password, QString* errorText)
{
    if (!m_handle)
    {
        m_lastResult = PdfReaderCoreCResultNotInit;
        if (errorText)
        {
            *errorText = m_createError;
        }
        return false;
    }
    const QByteArray pathData = path.toUtf8();
    const QByteArray passwordData = password.toUtf8();
    const int32_t result = pdfReaderCoreOpen(m_handle, pathData.constData(), passwordData.constData());
    m_lastResult = result;
    if (result == PdfReaderCoreCResultSuccess)
    {
        LOGINFO("PdfReader open completed, result=%d", result);
    }
    else
    {
        LOGERROR("PdfReader open failed, result=%d, error=%s", result, takeError().toUtf8().constData());
    }
    if (result == PdfReaderCoreCResultSuccess)
    {
        return true;
    }
    if (errorText)
    {
        *errorText = takeError();
    }
    return false;
}

void PdfReaderCoreBridge::close()
{
    if (m_handle)
    {
        pdfReaderCoreClose(m_handle);
    }
}

bool PdfReaderCoreBridge::pageInfo(int index, PdfReaderCoreCPageInfo* info, QString* errorText)
{
    if (!m_handle || !info)
    {
        m_lastResult = PdfReaderCoreCResultInvalidParam;
        if (errorText)
        {
            *errorText = m_pageError;
        }
        return false;
    }
    const int32_t result = pdfReaderCoreGetPageInfo(m_handle, index, info);
    m_lastResult = result;
    if (result == PdfReaderCoreCResultSuccess)
    {
        return true;
    }
    if (errorText)
    {
        *errorText = takeError();
    }
    return false;
}

QImage PdfReaderCoreBridge::renderPage(int index, int width, int height, QString* errorText)
{
    if (!m_handle || width <= 0 || height <= 0 ||
        static_cast<uint64_t>(width) * static_cast<uint64_t>(height) > m_maxRenderPixels)
    {
        m_lastResult = PdfReaderCoreCResultInvalidParam;
        if (errorText)
        {
            *errorText = m_renderError;
        }
        return QImage();
    }
    QImage image(width, height, QImage::Format_ARGB32);
    if (image.isNull())
    {
        m_lastResult = PdfReaderCoreCResultRenderFailed;
        if (errorText)
        {
            *errorText = m_renderError;
        }
        return QImage();
    }
    int32_t outWidth = 0;
    int32_t outHeight = 0;
    int32_t outStride = 0;
    size_t outBytes = 0;
    int32_t result = pdfReaderCoreRenderPage(m_handle, index, width, height,
                                             image.bits(), static_cast<size_t>(image.byteCount()),
                                             &outWidth, &outHeight, &outStride, &outBytes);
    if (result == PdfReaderCoreCResultBufferTooSmall && outWidth > 0 && outHeight > 0)
    {
        image = QImage(outWidth, outHeight, QImage::Format_ARGB32);
        result = pdfReaderCoreRenderPage(m_handle, index, width, height,
                                         image.bits(), static_cast<size_t>(image.byteCount()),
                                         &outWidth, &outHeight, &outStride, &outBytes);
    }
    m_lastResult = result;
    if (result != PdfReaderCoreCResultSuccess)
    {
        if (errorText)
        {
            *errorText = takeError();
        }
        return QImage();
    }
    return image;
}

bool PdfReaderCoreBridge::insertDocument(const QString& path, const QString& password, int index, QString* errorText)
{
    const QByteArray pathData = path.toUtf8();
    const QByteArray passwordData = password.toUtf8();
    const int32_t result = m_handle ? pdfReaderCoreInsertDocument(m_handle, pathData.constData(), passwordData.constData(), index)
                                    : PdfReaderCoreCResultInvalidParam;
    m_lastResult = result;
    if (result == PdfReaderCoreCResultSuccess)
    {
        LOGINFO("PdfReader insertDocument completed, result=%d", result);
    }
    else
    {
        LOGERROR("PdfReader insertDocument failed, result=%d, error=%s", result, takeError().toUtf8().constData());
    }
    if (result == PdfReaderCoreCResultSuccess)
    {
        return true;
    }
    if (errorText)
    {
        *errorText = takeError();
    }
    return false;
}

bool PdfReaderCoreBridge::movePage(int from, int to, QString* errorText)
{
    const int32_t result = m_handle ? pdfReaderCoreMovePage(m_handle, from, to) : PdfReaderCoreCResultInvalidParam;
    m_lastResult = result;
    if (result == PdfReaderCoreCResultSuccess)
    {
        LOGINFO("PdfReader movePage completed, result=%d", result);
    }
    else
    {
        LOGERROR("PdfReader movePage failed, result=%d, error=%s", result, takeError().toUtf8().constData());
    }
    if (result == PdfReaderCoreCResultSuccess)
    {
        return true;
    }
    if (errorText)
    {
        *errorText = takeError();
    }
    return false;
}

bool PdfReaderCoreBridge::saveTo(const QString& path, QString* errorText)
{
    const QByteArray pathData = path.toUtf8();
    const int32_t result = m_handle ? pdfReaderCoreSaveTo(m_handle, pathData.constData()) : PdfReaderCoreCResultInvalidParam;
    m_lastResult = result;
    if (result == PdfReaderCoreCResultSuccess)
    {
        LOGINFO("PdfReader saveTo completed, result=%d", result);
    }
    else
    {
        LOGERROR("PdfReader saveTo failed, result=%d, error=%s", result, takeError().toUtf8().constData());
    }
    if (result == PdfReaderCoreCResultSuccess)
    {
        return true;
    }
    if (errorText)
    {
        *errorText = takeError();
    }
    return false;
}

bool PdfReaderCoreBridge::saveToMain(QString* errorText)
{
    const int32_t result = m_handle ? pdfReaderCoreSaveToMain(m_handle) : PdfReaderCoreCResultInvalidParam;
    m_lastResult = result;
    if (result == PdfReaderCoreCResultSuccess)
    {
        LOGINFO("PdfReader saveToMain completed, result=%d", result);
    }
    else
    {
        LOGERROR("PdfReader saveToMain failed, result=%d, error=%s", result, takeError().toUtf8().constData());
    }
    if (result == PdfReaderCoreCResultSuccess)
    {
        return true;
    }
    if (errorText)
    {
        *errorText = takeError();
    }
    return false;
}

bool PdfReaderCoreBridge::validatePageRange(const QString& range, QString* errorText)
{
    const QByteArray rangeData = range.toUtf8();
    m_lastResult = m_handle ? pdfReaderCoreValidatePageRange(m_handle, rangeData.constData()) : PdfReaderCoreCResultNotInit;
    if (m_lastResult == PdfReaderCoreCResultSuccess)
    {
        return true;
    }
    if (errorText)
    {
        *errorText = takeError();
    }
    return false;
}

bool PdfReaderCoreBridge::savePageRange(const QString& range, const QString& path, QString* errorText)
{
    const QByteArray rangeData = range.toUtf8();
    const QByteArray pathData = path.toUtf8();
    const int32_t result = m_handle ? pdfReaderCoreSavePageRange(m_handle, rangeData.constData(), pathData.constData())
                                    : PdfReaderCoreCResultInvalidParam;
    m_lastResult = result;
    if (result == PdfReaderCoreCResultSuccess)
    {
        LOGINFO("PdfReader savePageRange completed, result=%d", result);
    }
    else
    {
        LOGERROR("PdfReader savePageRange failed, result=%d, error=%s", result, takeError().toUtf8().constData());
    }
    if (result == PdfReaderCoreCResultSuccess)
    {
        return true;
    }
    if (errorText)
    {
        *errorText = takeError();
    }
    return false;
}

bool PdfReaderCoreBridge::saveEachPage(const QString& directory, const QString& prefix, QString* errorText, bool overwrite)
{
    const QByteArray directoryData = directory.toUtf8();
    const QByteArray prefixData = prefix.toUtf8();
    const int32_t result = m_handle ? pdfReaderCoreSaveEachPageEx(m_handle, directoryData.constData(), prefixData.constData(), overwrite ? 1 : 0)
                                    : PdfReaderCoreCResultInvalidParam;
    m_lastResult = result;
    if (result == PdfReaderCoreCResultSuccess)
    {
        LOGINFO("PdfReader saveEachPage completed, result=%d", result);
    }
    else
    {
        LOGERROR("PdfReader saveEachPage failed, result=%d, error=%s", result, takeError().toUtf8().constData());
    }
    if (result == PdfReaderCoreCResultSuccess)
    {
        return true;
    }
    if (errorText)
    {
        *errorText = takeError();
    }
    return false;
}

int32_t PdfReaderCoreBridge::lastResult() const
{
    return m_lastResult;
}