#include "SplitViewerProfileStore.h"
#include <QtCore/QSaveFile>
#include <QtCore/QFile>
#include "LogManager/LogManagerAPI.h"

bool SplitViewerProfileStore::write(const QString& path, const SplitViewerCoreDocument& document, const QByteArray& thumbnail)
{
    std::vector<uint8_t> config;
    if (!SplitViewerCoreSerializeProfile(document, config))
    {
        return false;
    }
    const std::vector<uint8_t> thumb(reinterpret_cast<const uint8_t*>(thumbnail.constData()), reinterpret_cast<const uint8_t*>(thumbnail.constData()) + thumbnail.size());
    std::vector<uint8_t> package;
    if (!SplitViewerCoreBuildConfigPackage(thumb, config, package))
    {
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }
    const bool written = file.write(reinterpret_cast<const char*>(&package[0]), static_cast<qint64>(package.size())) == static_cast<qint64>(package.size()) && file.commit();
    LOGINFO("Save profile bytes=%u success=%d",static_cast<unsigned int>(package.size()),written);
    return written;
}

bool SplitViewerProfileStore::read(const QString& path, SplitViewerCoreDocument& document, QString& error)
{
    error.clear();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        error = QStringLiteral("无法读取配置：")+path;
        return false;
    }
    if (file.size()>64*1024*1024)
    {
        error = QStringLiteral("配置文件过大。");
        return false;
    }
    const QByteArray data = file.readAll();
    std::vector<uint8_t> bytes(reinterpret_cast<const uint8_t*>(data.constData()), reinterpret_cast<const uint8_t*>(data.constData()) + data.size());
    std::vector<uint8_t> config;
    if (!SplitViewerCoreExtractEmbeddedConfig(bytes, config) || !SplitViewerCoreDeserializeProfile(config, document))
    {
        error = QStringLiteral("不是有效的SplitViewer配置文件。");
        return false;
    }
    return true;
}