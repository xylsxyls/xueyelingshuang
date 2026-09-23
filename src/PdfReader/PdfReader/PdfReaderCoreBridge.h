#pragma once

#include "Config.h"
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtGui/QImage>

#define PDFREADERCORE_USE_C_API
#include "PdfReaderCore/PdfReaderCoreAPI.h"


class PdfReaderCoreBridge
{
public:
    explicit PdfReaderCoreBridge(const Config& config = Config());
    ~PdfReaderCoreBridge();

    bool init(QString* errorText = nullptr);
    /** 初始化时复制 Core 参数；之后修改调用方配置不影响现有实例 */
    bool init(const PdfReaderCoreCConfig& config, QString* errorText = nullptr);
    void shutdown();
    bool isOpen() const;
    int pageCount() const;
    QString lastError() const;

    bool open(const QString& path, const QString& password, QString* errorText = nullptr);
    void close();
    bool pageInfo(int index, PdfReaderCoreCPageInfo* info, QString* errorText = nullptr) const;
    QImage renderPage(int index, int width, int height, QString* errorText = nullptr) const;
    bool insertDocument(const QString& path, const QString& password, int index, QString* errorText = nullptr);
    bool movePage(int from, int to, QString* errorText = nullptr);
    bool saveTo(const QString& path, QString* errorText = nullptr);
    bool saveToMain(QString* errorText = nullptr);
    bool validatePageRange(const QString& range, QString* errorText = nullptr);
    bool savePageRange(const QString& range, const QString& path, QString* errorText = nullptr);
    bool saveEachPage(const QString& directory, const QString& prefix, QString* errorText = nullptr, bool overwrite = false);

private:
    QString takeError() const;

private:
    const Config m_config;
    uint64_t m_maxRenderPixels;
    PdfReaderCoreHandle m_handle;
};