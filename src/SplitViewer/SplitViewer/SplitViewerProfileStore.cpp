#include "SplitViewerProfileStore.h"
#include "Config.h"
#include "CSystem/CSystemAPI.h"
#include <QtCore/QSaveFile>
#include <limits>
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
    if (g_config.m_profileMaximumBytes <= 0 ||
        static_cast<quint64>(g_config.m_profileMaximumBytes) > (std::numeric_limits<size_t>::max)())
    {
        error = g_config.m_profileTooLargeError;
        return false;
    }
    std::vector<uint8_t> bytes;
    if (!CSystem::readBinaryFile(path.toStdWString(), static_cast<size_t>(g_config.m_profileMaximumBytes), bytes))
    {
        error = g_config.m_readProfileError + path;
        return false;
    }
    std::vector<uint8_t> config;
    if (!SplitViewerCoreExtractEmbeddedConfig(bytes, config) || !SplitViewerCoreDeserializeProfile(config, document))
    {
        error = g_config.m_invalidProfileError;
        return false;
    }
    return true;
}