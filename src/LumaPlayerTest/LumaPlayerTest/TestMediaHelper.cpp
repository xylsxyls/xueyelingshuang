#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "TestMediaHelper.h"
#include "TestConfig.h"
#include "LogManager/LogManagerAPI.h"
#include <QDataStream>
#include <QDirIterator>
#include <QFileInfo>
#include <QSaveFile>
#include <QProcess>
#include <QElapsedTimer>
#include <cmath>

QByteArray TestMediaHelper::chunk(const QByteArray& tag, const QByteArray& data)
{
    QByteArray result = tag;
    QDataStream stream(&result, QIODevice::Append);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint32(data.size());
    result += data;
    if (data.size() % 2 != 0)
    {
        result += char(0);
    }
    return result;
}

QByteArray TestMediaHelper::list(const QByteArray& tag, const QByteArray& data)
{
    return chunk("LIST", tag + data);
}

QByteArray TestMediaHelper::generate(const std::atomic<bool>& exit)
{
    // 独立固定标准：160x90，25fps，200帧；首秒红色，其余蓝色，48kHz双声道PCM
    QByteArray avih;
    QDataStream a(&avih, QIODevice::WriteOnly);
    a.setByteOrder(QDataStream::LittleEndian);
    const quint32 values[] = {40000, 1272000, 0, 16, 200, 0, 2, 43200, 160, 90, 0, 0, 0, 0};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
    {
        a << values[i];
    }
    QByteArray streams[2];
    for (int32_t kind = 0; kind < 2; ++kind)
    {
        QByteArray sh = kind == 0 ? QByteArray("vidsDIB ", 8) : QByteArray("auds\0\0\0\0", 8);
        QDataStream s(&sh, QIODevice::Append);
        s.setByteOrder(QDataStream::LittleEndian);
        s << quint32(0) << quint16(0) << quint16(0) << quint32(0);
        s << quint32(kind == 0 ? 1 : 4) << quint32(kind == 0 ? 25 : 192000) << quint32(0);
        s << quint32(kind == 0 ? 200 : 384000) << quint32(kind == 0 ? 43200 : 7680);
        s << quint32(0xffffffff) << quint32(kind == 0 ? 0 : 4);
        s << qint16(0) << qint16(0) << qint16(kind == 0 ? 160 : 0) << qint16(kind == 0 ? 90 : 0);
        QByteArray format;
        QDataStream f(&format, QIODevice::WriteOnly);
        f.setByteOrder(QDataStream::LittleEndian);
        if (kind == 0)
        {
            f << quint32(40) << qint32(160) << qint32(90) << quint16(1) << quint16(24);
            f << quint32(0) << quint32(43200) << qint32(0) << qint32(0) << quint32(0) << quint32(0);
        }
        else
        {
            f << quint16(1) << quint16(2) << quint32(48000) << quint32(192000) << quint16(4) << quint16(16);
        }
        streams[kind] = list("strl", chunk("strh", sh) + chunk("strf", format));
    }
    const QByteArray header = list("hdrl", chunk("avih", avih) + streams[0] + streams[1]);
    QByteArray frames;
    QByteArray index;
    quint32 offset = 4;
    for (int32_t i = 0; i < 200; ++i)
    {
        if (exit.load())
        {
            return QByteArray();
        }
        QByteArray video(43200, char(0));
        for (int32_t pixel = 0; pixel < 14400; ++pixel)
        {
            video[pixel * 3 + (i < 25 ? 2 : 0)] = char(255);
        }
        QByteArray pcm;
        QDataStream sound(&pcm, QIODevice::WriteOnly);
        sound.setByteOrder(QDataStream::LittleEndian);
        for (int32_t sample = 0; sample < 1920; ++sample)
        {
            const qint16 value = static_cast<qint16>(1200 * std::sin(2 * 3.141592653589793 * 440 * (i * 1920 + sample) / 48000));
            sound << value << value;
        }
        for (int32_t kind = 0; kind < 2; ++kind)
        {
            const QByteArray tag = kind == 0 ? "00db" : "01wb";
            const QByteArray data = kind == 0 ? video : pcm;
            const QByteArray part = chunk(tag, data);
            index += tag;
            QDataStream entry(&index, QIODevice::Append);
            entry.setByteOrder(QDataStream::LittleEndian);
            entry << quint32(16) << offset << quint32(data.size());
            frames += part;
            offset += part.size();
        }
    }
    return chunk("RIFF", QByteArray("AVI ") + header + list("movi", frames) + chunk("idx1", index));
}

QString TestMediaHelper::prepare(const QString& directory, const QString& output,
    const QString& executable, const std::atomic<bool>& exit, QString* media, QString* fixture)
{
    const TestConfig config;
    QDirIterator files(directory, config.m_videoFilters, QDir::Files | QDir::Readable | QDir::NoSymLinks,
        QDirIterator::NoIteratorFlags);
    int32_t tried = 0;
    while (!exit.load() && files.hasNext() && tried < config.m_videoProbeLimit)
    {
        const QString candidate = files.next();
        ++tried;
        QProcess probe;
        probe.setProcessChannelMode(QProcess::ForwardedErrorChannel);
        probe.start(executable, QStringList() << "--mode" << "media-probe" << "--media" << candidate
            << "--output" << output + "/probe");
        QElapsedTimer timer;
        timer.start();
        while (!exit.load() && probe.state() != QProcess::NotRunning && timer.elapsed() < config.m_videoProbeMs)
        {
            probe.waitForFinished(20);
        }
        if (exit.load() || probe.state() != QProcess::NotRunning)
        {
            probe.kill();
            probe.waitForFinished(2000);
            if (exit.load())
            {
                return config.m_mediaCanceled;
            }
            LOGWARNING("Video probe timeout path=%s", candidate.toUtf8().constData());
            continue;
        }
        if (probe.error() == QProcess::FailedToStart)
        {
            return config.m_mediaProbeFailed + probe.errorString();
        }
        if (probe.exitStatus() == QProcess::NormalExit && probe.exitCode() == 0)
        {
            *media = QFileInfo(candidate).absoluteFilePath();
            LOGINFO("Selected real video path=%s candidates=%d recursive=0", media->toUtf8().constData(), tried);
            break;
        }
        LOGWARNING("Video probe rejected path=%s", candidate.toUtf8().constData());
    }
    if (exit.load())
    {
        return config.m_mediaCanceled;
    }
    if (media->isEmpty())
    {
        return config.m_noVideo;
    }
    if (!QDir().mkpath(output))
    {
        return config.m_fixtureFailed + output;
    }
    const QByteArray data = generate(exit);
    if (data.isEmpty() || exit.load())
    {
        return config.m_mediaCanceled;
    }
    const QStringList names = QStringList() << "scene.avi" << QStringLiteral("中文 空格.avi");
    for (int32_t i = 0; i < names.size(); ++i)
    {
        if (exit.load())
        {
            return config.m_mediaCanceled;
        }
        QSaveFile file(output + "/" + names[i]);
        if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit())
        {
            return config.m_fixtureFailed + file.fileName();
        }
    }
    *fixture = output + "/scene.avi";
    LOGINFO("Generated deterministic fixture bytes=%d path=%s", data.size(), fixture->toUtf8().constData());
    return QString();
}