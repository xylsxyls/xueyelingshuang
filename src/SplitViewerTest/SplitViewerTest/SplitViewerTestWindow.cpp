#include "SplitViewerTestWindow.h"
#include <QtTest/QtTest>

SplitViewerTestWindow::SplitViewerTestWindow() :
m_browseCount(0)
{
    resize(900, 700);
    show();
    activateWindow();
    QTest::qWait(30);
}

QString SplitViewerTestWindow::browseFile(
    bool save, const QString& title, const QString& initial, const QString& filter)
{
    Q_UNUSED(save);
    Q_UNUSED(title);
    Q_UNUSED(initial);
    m_lastFilter = filter;
    ++m_browseCount;
    const QString result = m_nextFile;
    m_nextFile.clear();
    const std::function<void()> action = m_duringBrowse;
    m_duringBrowse = std::function<void()>();
    if (action)
    {
        action();
    }
    return result;
}

void SplitViewerTestWindow::reportError(const QString& message)
{
    m_errors.append(message);
}