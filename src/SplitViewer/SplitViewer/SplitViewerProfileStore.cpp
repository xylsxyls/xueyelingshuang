#include "SplitViewerProfileStore.h"
#include "CSystem/CSystemAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <QtCore/QSaveFile>
#include <limits>

bool SplitViewerProfileStore::write(
    const QString& path, const SplitViewerCoreDocument& document, const QByteArray& thumbnail)
{
    std::vector<uint8_t> config;
    if (!SplitViewerCoreSerializeProfile(document, config))
    {
        return false;
    }
    const std::vector<uint8_t> thumb(reinterpret_cast<const uint8_t*>(thumbnail.constData()),
        reinterpret_cast<const uint8_t*>(thumbnail.constData()) + thumbnail.size());
    std::vector<uint8_t> package;
    if (!SplitViewerCoreBuildConfigPackage(thumb, config, package, document.config()))
    {
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }
    const bool written = file.write(reinterpret_cast<const char*>(&package[0]), static_cast<qint64>(package.size())) ==
            static_cast<qint64>(package.size()) &&
        file.commit();
    LOGINFO("Save profile bytes=%u success=%d", static_cast<unsigned int>(package.size()), written);
    return written;
}

bool SplitViewerProfileStore::read(const QString& path, SplitViewerCoreDocument& document, QString& error,
    qint64 maximumBytes, const QString& readError, const QString& invalidError, const QString& limitError)
{
    error.clear();
    if (maximumBytes <= 0 || static_cast<quint64>(maximumBytes) > (std::numeric_limits<size_t>::max)())
    {
        error = limitError;
        return false;
    }
    std::vector<uint8_t> bytes;
    if (!CSystem::readBinaryFile(path.toStdWString(), static_cast<size_t>(maximumBytes), bytes))
    {
        error = readError + path;
        return false;
    }
    std::vector<uint8_t> config;
    if (!SplitViewerCoreExtractEmbeddedConfig(bytes, config, document.config()) ||
        !SplitViewerCoreDeserializeProfile(config, document))
    {
        error = invalidError;
        return false;
    }
    return true;
}