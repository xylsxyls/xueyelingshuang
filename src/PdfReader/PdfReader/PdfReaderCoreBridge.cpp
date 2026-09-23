#include "PdfReaderCoreBridge.h"

#include <QByteArray>
#include "LogManager/LogManagerAPI.h"

PdfReaderCoreBridge::PdfReaderCoreBridge(const Config& config)
    : m_config(config), m_maxRenderPixels(config.core.maxRenderPixels), m_handle(nullptr)
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
    if (!m_handle) return m_config.bridgeCreateError;
    const size_t required = pdfReaderCoreGetLastError(m_handle, nullptr, 0);
    if (!required) return m_config.bridgeInternalError;
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
    if (!m_handle) { m_handle = pdfReaderCoreCreate(); }
    if (!m_handle)
    {
        if (errorText) *errorText = m_config.bridgeCreateError;
        return false;
    }
    const int32_t result = pdfReaderCoreInitWithConfig(m_handle, &config);
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
    if (errorText) *errorText = takeError();
    return false;
}

void PdfReaderCoreBridge::shutdown()
{
    if (m_handle) pdfReaderCoreUninit(m_handle);
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
        if (errorText) *errorText = m_config.bridgeCreateError;
        return false;
    }
    const QByteArray pathData = path.toUtf8();
    const QByteArray passwordData = password.toUtf8();
    const int32_t result = pdfReaderCoreOpen(m_handle, pathData.constData(), passwordData.constData());
    if (result == PdfReaderCoreCResultSuccess)
    {
        LOGINFO("PdfReader open completed, result=%d", result);
    }
    else
    {
        LOGERROR("PdfReader open failed, result=%d, error=%s", result, takeError().toUtf8().constData());
    }
    if (result == PdfReaderCoreCResultSuccess) return true;
    if (errorText) *errorText = takeError();
    return false;
}

void PdfReaderCoreBridge::close()
{
    if (m_handle) pdfReaderCoreClose(m_handle);
}

bool PdfReaderCoreBridge::pageInfo(int index, PdfReaderCoreCPageInfo* info, QString* errorText) const
{
    if (!m_handle || !info)
    {
        if (errorText) *errorText = m_config.pageParameterError;
        return false;
    }
    const int32_t result = pdfReaderCoreGetPageInfo(m_handle, index, info);
    if (result == PdfReaderCoreCResultSuccess) return true;
    if (errorText) *errorText = takeError();
    return false;
}

QImage PdfReaderCoreBridge::renderPage(int index, int width, int height, QString* errorText) const
{
    if (!m_handle || width <= 0 || height <= 0 ||
        static_cast<uint64_t>(width) * static_cast<uint64_t>(height) > m_maxRenderPixels)
    {
        if (errorText) *errorText = m_config.renderParameterError;
        return QImage();
    }
    QImage image(width, height, QImage::Format_ARGB32);
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
    if (result != PdfReaderCoreCResultSuccess)
    {
        if (errorText) *errorText = takeError();
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
    if (result == PdfReaderCoreCResultSuccess)
    {
        LOGINFO("PdfReader insertDocument completed, result=%d", result);
    }
    else
    {
        LOGERROR("PdfReader insertDocument failed, result=%d, error=%s", result, takeError().toUtf8().constData());
    }
    if (result == PdfReaderCoreCResultSuccess) return true;
    if (errorText) *errorText = takeError();
    return false;
}

bool PdfReaderCoreBridge::movePage(int from, int to, QString* errorText)
{
    const int32_t result = m_handle ? pdfReaderCoreMovePage(m_handle, from, to) : PdfReaderCoreCResultInvalidParam;
    if (result == PdfReaderCoreCResultSuccess)
    {
        LOGINFO("PdfReader movePage completed, result=%d", result);
    }
    else
    {
        LOGERROR("PdfReader movePage failed, result=%d, error=%s", result, takeError().toUtf8().constData());
    }
    if (result == PdfReaderCoreCResultSuccess) return true;
    if (errorText) *errorText = takeError();
    return false;
}

bool PdfReaderCoreBridge::saveTo(const QString& path, QString* errorText)
{
    const QByteArray pathData = path.toUtf8();
    const int32_t result = m_handle ? pdfReaderCoreSaveTo(m_handle, pathData.constData()) : PdfReaderCoreCResultInvalidParam;
    if (result == PdfReaderCoreCResultSuccess)
    {
        LOGINFO("PdfReader saveTo completed, result=%d", result);
    }
    else
    {
        LOGERROR("PdfReader saveTo failed, result=%d, error=%s", result, takeError().toUtf8().constData());
    }
    if (result == PdfReaderCoreCResultSuccess) return true;
    if (errorText) *errorText = takeError();
    return false;
}

bool PdfReaderCoreBridge::saveToMain(QString* errorText)
{
    const int32_t result = m_handle ? pdfReaderCoreSaveToMain(m_handle) : PdfReaderCoreCResultInvalidParam;
    if (result == PdfReaderCoreCResultSuccess)
    {
        LOGINFO("PdfReader saveToMain completed, result=%d", result);
    }
    else
    {
        LOGERROR("PdfReader saveToMain failed, result=%d, error=%s", result, takeError().toUtf8().constData());
    }
    if (result == PdfReaderCoreCResultSuccess) return true;
    if (errorText) *errorText = takeError();
    return false;
}

bool PdfReaderCoreBridge::validatePageRange(const QString& range, QString* errorText)
{
    const QByteArray rangeData = range.toUtf8();
    if (m_handle && pdfReaderCoreValidatePageRange(m_handle, rangeData.constData()) == PdfReaderCoreCResultSuccess)
    {
        return true;
    }
    if (errorText) { *errorText = takeError(); }
    return false;
}

bool PdfReaderCoreBridge::savePageRange(const QString& range, const QString& path, QString* errorText)
{
    const QByteArray rangeData = range.toUtf8();
    const QByteArray pathData = path.toUtf8();
    const int32_t result = m_handle ? pdfReaderCoreSavePageRange(m_handle, rangeData.constData(), pathData.constData())
                                    : PdfReaderCoreCResultInvalidParam;
    if (result == PdfReaderCoreCResultSuccess)
    {
        LOGINFO("PdfReader savePageRange completed, result=%d", result);
    }
    else
    {
        LOGERROR("PdfReader savePageRange failed, result=%d, error=%s", result, takeError().toUtf8().constData());
    }
    if (result == PdfReaderCoreCResultSuccess) return true;
    if (errorText) *errorText = takeError();
    return false;
}

bool PdfReaderCoreBridge::saveEachPage(const QString& directory, const QString& prefix, QString* errorText, bool overwrite)
{
    const QByteArray directoryData = directory.toUtf8();
    const QByteArray prefixData = prefix.toUtf8();
    const int32_t result = m_handle ? pdfReaderCoreSaveEachPageEx(m_handle, directoryData.constData(), prefixData.constData(), overwrite ? 1 : 0)
                                    : PdfReaderCoreCResultInvalidParam;
    if (result == PdfReaderCoreCResultSuccess)
    {
        LOGINFO("PdfReader saveEachPage completed, result=%d", result);
    }
    else
    {
        LOGERROR("PdfReader saveEachPage failed, result=%d, error=%s", result, takeError().toUtf8().constData());
    }
    if (result == PdfReaderCoreCResultSuccess) return true;
    if (errorText) *errorText = takeError();
    return false;
}