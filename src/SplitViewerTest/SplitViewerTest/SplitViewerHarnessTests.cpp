#include "SplitViewerHarnessTests.h"
#include "SplitViewerTest.h"
#include "QtControls/ComboBox.h"
#include "QtControls/PushButton.h"
#include "QtControls/PlainTextEdit.h"
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtTest/QtTest>

bool SplitViewerHarnessTests::run(const QString& directory)
{
    SplitViewerTest window;
    window.show();
    QTest::qWait(40);
    ComboBox* choices = window.findChild<ComboBox*>(QStringLiteral("uiCases"));
    PushButton* run = window.findChild<PushButton*>(QStringLiteral("uiRun"));
    PushButton* core = window.findChild<PushButton*>(QStringLiteral("coreRun"));
    QPlainTextEdit* output = window.findChild<QPlainTextEdit*>(QStringLiteral("testOutput"));
    QProcess* process = window.findChild<QProcess*>();
    if (choices == nullptr || run == nullptr || core == nullptr ||
        dynamic_cast<PlainTextEdit*>(output) == nullptr || process == nullptr)
    {
        return false;
    }
    const int selected = choices->findData(194);
    if (selected < 0)
    {
        return false;
    }
    choices->setCurrentIndex(selected);
    const bool screenshot = window.grab().save(QDir(directory).filePath(QStringLiteral("test-harness.png")));
    QSignalSpy starts(process, SIGNAL(started()));
    int beats = 0;
    QTimer heartbeat;
    QObject::connect(&heartbeat, &QTimer::timeout, [&beats]() { ++beats; });
    heartbeat.start(1);
    QTest::mouseClick(run, Qt::LeftButton);
    const bool locked = !run->isEnabled() && !core->isEnabled() && !choices->isEnabled() && !window.allTestsPassed();
    QTest::mouseClick(run, Qt::LeftButton);
    const bool waitsForClose = !window.close() && window.isVisible();
    QElapsedTimer elapsed;
    elapsed.start();
    while (window.isVisible() && elapsed.elapsed() < 15000)
    {
        QTest::qWait(10);
    }
    heartbeat.stop();
    const bool finished = process->state() == QProcess::NotRunning;
    const bool passed = screenshot && locked && waitsForClose && !window.isVisible() && finished &&
        starts.count() == 1 && beats > 0 && window.allTestsPassed() &&
        output->toPlainText().contains(QStringLiteral("194 ")) &&
        output->toPlainText().contains(QStringLiteral("total=1 failures=0"));
    if (!finished)
    {
        // 本项只启动纯数值解析194，不持有播放器，也不修改系统设置。
        process->kill();
        process->waitForFinished(2000);
    }
    QFile report(QDir(directory).filePath(QStringLiteral("harness-results.txt")));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }
    QTextStream(&report) << "passed=" << passed << " locked=" << locked << " closeWait=" << waitsForClose <<
        " starts=" << starts.count() << " heartbeats=" << beats << " finished=" << finished << "\n" <<
        output->toPlainText();
    return passed;
}