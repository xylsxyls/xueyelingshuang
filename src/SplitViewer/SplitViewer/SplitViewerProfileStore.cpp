#include "SplitViewerProfileStore.h"
#include "Config.h"
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
        error = g_config.m_readProfileError+path;
        return false;
    }
    if (file.size()>g_config.m_profileMaximumBytes)
    {
        error = g_config.m_profileTooLargeError;
        return false;
    }
    const QByteArray data = file.readAll();
    std::vector<uint8_t> bytes(reinterpret_cast<const uint8_t*>(data.constData()), reinterpret_cast<const uint8_t*>(data.constData()) + data.size());
    std::vector<uint8_t> config;
    if (!SplitViewerCoreExtractEmbeddedConfig(bytes, config) || !SplitViewerCoreDeserializeProfile(config, document))
    {
        error = g_config.m_invalidProfileError;
        return false;
    }
    return true;
}