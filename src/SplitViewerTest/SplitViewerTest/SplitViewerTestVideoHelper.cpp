#include "SplitViewerTestVideoHelper.h"
#include <QtCore/QFile>
#include <QtCore/QDataStream>
#include <QtCore/QByteArray>
#include <QtGui/QScreen>
#include <QtGui/QGuiApplication>
#include <QtTest/QtTest>
#include <algorithm>
#include <cmath>

#ifdef Q_OS_WIN
bool SplitViewerTestVideoHelper::writeVideoFixture(const QString& path)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    const int width = 160, height = 96, frames = 400, frameBytes = width * height * 3;
    stream.writeRawData("RIFF", 4);
    stream << quint32(0);
    stream.writeRawData("AVI LIST", 8);
    stream << quint32(192);
    stream.writeRawData("hdrlavih", 8);
    stream << quint32(56);
    stream << quint32(100000) << quint32(frameBytes * 10) << quint32(0) << quint32(16) << quint32(frames) << quint32(0)
           << quint32(1) << quint32(frameBytes) << quint32(width) << quint32(height) << quint32(0) << quint32(0)
           << quint32(0) << quint32(0);
    stream.writeRawData("LIST", 4);
    stream << quint32(116);
    stream.writeRawData("strlstrh", 8);
    stream << quint32(56);
    stream.writeRawData("vidsDIB ", 8);
    stream << quint32(0) << quint16(0) << quint16(0) << quint32(0) << quint32(1) << quint32(10) << quint32(0)
           << quint32(frames) << quint32(frameBytes) << quint32(0xffffffff) << quint32(0) << qint16(0) << qint16(0)
           << qint16(width) << qint16(height);
    stream.writeRawData("strf", 4);
    stream << quint32(40) << quint32(40) << qint32(width) << qint32(height) << quint16(1) << quint16(24) << quint32(0)
           << quint32(frameBytes) << qint32(0) << qint32(0) << quint32(0) << quint32(0);
    stream.writeRawData("LIST", 4);
    stream << quint32(4 + frames * (frameBytes + 8));
    stream.writeRawData("movi", 4);
    for (int frame = 0; frame < frames; ++frame)
    {
        stream.writeRawData("00db", 4);
        stream << quint32(frameBytes);
        for (int y = height - 1; y >= 0; --y)
        {
            for (int x = 0; x < width; ++x)
            {
                const int pulse = (frame / 3) % 2 ? 220 : 140;
                QColor color = y < height / 2 ? (x < width / 2 ? QColor(pulse, 30, 30) : QColor(30, pulse, 30))
                                              : (x < width / 2 ? QColor(30, 30, pulse) : QColor(pulse, pulse, 30));
                // Nine bit frame number in a small red-quadrant band. Unlike a
                // two-level pulse this cannot alias with desktop capture cadence.
                if (y >= 30 && y < 35 && x >= 10 && x < 64)
                {
                    const int bit = (x - 10) / 6;
                    color = QColor((frame & (1 << bit)) ? 240 : 80, 30, 30);
                }
                stream << quint8(color.blue()) << quint8(color.green()) << quint8(color.red());
            }
        }
    }
    stream.writeRawData("idx1", 4);
    stream << quint32(frames * 16);
    for (int frame = 0; frame < frames; ++frame)
    {
        stream.writeRawData("00db", 4);
        stream << quint32(16) << quint32(4 + frame * (frameBytes + 8)) << quint32(frameBytes);
    }
    stream.device()->seek(4);
    stream << quint32(bytes.size() - 8);
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}

QImage SplitViewerTestVideoHelper::screenPane(const QRect& pane)
{
    return QGuiApplication::primaryScreen()->grabWindow(0, pane.x(), pane.y(), pane.width(), pane.height()).toImage();
}

bool SplitViewerTestVideoHelper::videoPixelsFill(const QImage& screen)
{
    if (screen.isNull())
    {
        return false;
    }
    const double scale = (std::min)(screen.width() / 160.0, screen.height() / 96.0);
    const double left = (screen.width() - 160 * scale) / 2;
    const double top = (screen.height() - 96 * scale) / 2;
    for (int quadrant = 0; quadrant < 4; ++quadrant)
    {
        const QColor color(screen.pixel(
            qRound(left + (quadrant % 2 ? 136 : 24) * scale), qRound(top + (quadrant / 2 ? 72 : 24) * scale)));
        const bool red = quadrant == 0 || quadrant == 3;
        const bool green = quadrant == 1 || quadrant == 3;
        const bool blue = quadrant == 2;
        if ((red ? color.red() < 100 : color.red() > 65) || (green ? color.green() < 100 : color.green() > 65) ||
            (blue ? color.blue() < 100 : color.blue() > 65))
        {
            return false;
        }
    }
    return true;
}

int SplitViewerTestVideoHelper::videoFrameNumber(const QImage& screen)
{
    if (!SplitViewerTestVideoHelper::videoPixelsFill(screen))
    {
        return -1;
    }
    const double scale = (std::min)(screen.width() / 160.0, screen.height() / 96.0);
    const double left = (screen.width() - 160 * scale) / 2;
    const double top = (screen.height() - 96 * scale) / 2;
    int frame = 0;
    for (int bit = 0; bit < 9; ++bit)
    {
        const QColor color(screen.pixel(qRound(left + (13 + bit * 6) * scale), qRound(top + 32 * scale)));
        if (color.green() > 65 || color.blue() > 65 || color.red() < 65)
        {
            return -1;
        }
        if (color.red() > 160)
        {
            frame |= 1 << bit;
        }
    }
    return frame < 400 ? frame : -1;
}

bool SplitViewerTestVideoHelper::videoIsAdvancing(const QRect& pane, int observationMilliseconds, QTextStream* trace)
{
    QElapsedTimer timer;
    timer.start();
    int first = SplitViewerTestVideoHelper::videoFrameNumber(SplitViewerTestVideoHelper::screenPane(pane));
    if (trace)
    {
        *trace << "frame initial=" << first << " epoch=" << QDateTime::currentMSecsSinceEpoch() << "\n";
    }
    while (timer.elapsed() < observationMilliseconds)
    {
        QTest::qWait(100);
        const int next = SplitViewerTestVideoHelper::videoFrameNumber(SplitViewerTestVideoHelper::screenPane(pane));
        if (trace)
        {
            *trace << "frame t=" << timer.elapsed() << " value=" << next
                   << " epoch=" << QDateTime::currentMSecsSinceEpoch() << "\n";
        }
        if (first >= 0 && next >= 0 && first != next)
        {
            return true;
        }
        if (first < 0)
        {
            first = next;
        }
    }
    return false;
}
#endif