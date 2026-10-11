#include "SplitViewerTest.h"
#include "SplitViewerUiTests.h"
#include "SplitViewerTestCaseHelper.h"
#include "QtControls/CheckBox.h"
#include "QtControls/LineEdit.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "QtControls/ComboBox.h"
#include "QtControls/PlainTextEdit.h"
#include "QtControls/PushButton.h"
#include "QtControls/Widget.h"
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <vector>

SplitViewerTest::SplitViewerTest(QWidget* parent) :
MainWindow(parent),
m_output(nullptr),
m_allTestsPassed(false),
m_uiCases(nullptr),
m_coreRun(nullptr),
m_uiRun(nullptr),
m_uiProcess(new QProcess(this)),
m_running(false),
m_closeRequested(false),
m_caseSelection(nullptr),
m_instantRun(nullptr),
m_pressureRun(nullptr),
m_allRun(nullptr),
m_cancelRun(nullptr),
m_releasePath(nullptr),
m_debugPath(nullptr),
m_useDebug(nullptr),
m_expectedCaseCount(0)
{
    setWindowTitle(g_testConfig.m_title);
    resize(g_testConfig.m_windowSize);
    Widget* central = new Widget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);
    m_releasePath = new LineEdit(central);
    m_releasePath->setObjectName(QStringLiteral("releaseProgramPath"));
    m_releasePath->setText(g_testConfig.m_releaseProgramPath);
    m_releasePath->setReadOnly(true);
    layout->addWidget(m_releasePath);
    m_debugPath = new LineEdit(central);
    m_debugPath->setObjectName(QStringLiteral("debugProgramPath"));
    m_debugPath->setText(g_testConfig.m_debugProgramPath);
    m_debugPath->setReadOnly(true);
    layout->addWidget(m_debugPath);
    m_useDebug = new CheckBox(central);
    m_useDebug->setObjectName(QStringLiteral("useDebug"));
    m_useDebug->setText(g_testConfig.m_debugText);
#ifdef _DEBUG
    m_useDebug->setChecked(true);
#endif
    layout->addWidget(m_useDebug);
    m_coreRun = new PushButton(central);
    m_coreRun->setObjectName(QStringLiteral("coreRun"));
    m_coreRun->setText(g_testConfig.m_coreRunText);
    m_coreRun->setClickBreathTime(0);
    m_output = new PlainTextEdit(central);
    m_output->setObjectName(QStringLiteral("testOutput"));
    m_output->setReadOnly(true);
    m_output->setMaximumBlockCount(g_testConfig.m_maximumOutputBlocks);
    layout->addWidget(m_coreRun);
    m_uiCases = new ComboBox(central);
    m_uiCases->setObjectName(QStringLiteral("uiCases"));
    m_uiCases->addItem(g_testConfig.m_allCasesText);
    m_uiCases->setItemData(0, 0);
    foreach (const QString& name, SplitViewerAuditCaseNames())
    {
        m_uiCases->addItem(name);
        m_uiCases->setItemData(m_uiCases->count() - 1, name.section(QLatin1Char(' '), 0, 0).toInt());
    }
    layout->addWidget(m_uiCases);
    m_caseSelection = new LineEdit(central);
    m_caseSelection->setObjectName(QStringLiteral("caseSelection"));
    m_caseSelection->setPlaceholderText(g_testConfig.m_selectionHint);
    m_caseSelection->setMaxLength(g_testConfig.m_selectionMaximumLength);
    layout->addWidget(m_caseSelection);
    m_uiRun = new PushButton(central);
    m_uiRun->setObjectName(QStringLiteral("uiRun"));
    m_uiRun->setText(g_testConfig.m_uiRunText);
    m_uiRun->setClickBreathTime(0);
    layout->addWidget(m_uiRun);
    connect(m_uiRun, SIGNAL(clicked()), this, SLOT(runUiTests()));
    m_instantRun = new PushButton(central);
    m_instantRun->setText(g_testConfig.m_instantText);
    m_instantRun->setClickBreathTime(0);
    layout->addWidget(m_instantRun);
    connect(m_instantRun, SIGNAL(clicked()), this, SLOT(runInstantTests()));
    m_pressureRun = new PushButton(central);
    m_pressureRun->setText(g_testConfig.m_pressureText);
    m_pressureRun->setClickBreathTime(0);
    layout->addWidget(m_pressureRun);
    connect(m_pressureRun, SIGNAL(clicked()), this, SLOT(runPressureTests()));
    m_allRun = new PushButton(central);
    m_allRun->setText(g_testConfig.m_allText);
    m_allRun->setClickBreathTime(0);
    layout->addWidget(m_allRun);
    connect(m_allRun, SIGNAL(clicked()), this, SLOT(runAllTests()));
    m_cancelRun = new PushButton(central);
    m_cancelRun->setText(g_testConfig.m_cancelText);
    m_cancelRun->setClickBreathTime(0);
    m_cancelRun->setEnabled(false);
    layout->addWidget(m_cancelRun);
    connect(m_cancelRun, SIGNAL(clicked()), this, SLOT(cancelTests()));
    layout->addWidget(m_output, 1);
    setCentralWidget(central);
    connect(m_coreRun, SIGNAL(clicked()), this, SLOT(runTests()));
    connect(m_uiProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this,
        SLOT(uiProcessFinished(int, QProcess::ExitStatus)));
    connect(m_uiProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(uiProcessError(QProcess::ProcessError)));
}

SplitViewerTest::~SplitViewerTest()
{

}

bool SplitViewerTest::allTestsPassed() const
{
    return m_allTestsPassed && !m_running;
}

void SplitViewerTest::runTests()
{
    startBatch(QStringLiteral("1-7"));
}

void SplitViewerTest::runUiTests()
{
    QString selection = m_caseSelection->text().trimmed();
    if (selection.isEmpty())
    {
        const int32_t selected = m_uiCases->itemData(m_uiCases->currentIndex()).toInt();
        if (selected == 0)
        {
            runInstantTests();
            return;
        }
        selection = QString::number(selected);
    }
    startBatch(selection);
}

void SplitViewerTest::runInstantTests()
{
    QStringList ids;
    for (int32_t id : SplitViewerTestCaseHelper::byType(SPLITVIEWER_TEST_INSTANT))
    {
        ids.append(QString::number(id));
    }
    startBatch(ids.join(QLatin1Char(',')));
}

void SplitViewerTest::runPressureTests()
{
    QStringList ids;
    for (int32_t id : SplitViewerTestCaseHelper::byType(SPLITVIEWER_TEST_PRESSURE))
    {
        ids.append(QString::number(id));
    }
    startBatch(ids.join(QLatin1Char(',')));
}

void SplitViewerTest::runAllTests()
{
    QStringList ids;
    for (const auto& entry : SplitViewerTestCaseHelper::kCases)
    {
        ids.append(QString::number(static_cast<int32_t>(entry.first)));
    }
    startBatch(ids.join(QLatin1Char(',')));
}

void SplitViewerTest::cancelTests()
{
    if (!m_running)
    {
        return;
    }
    QFile request(QDir(m_reportDirectory).filePath(QStringLiteral("cancel.request")));
    if (!request.open(QIODevice::WriteOnly) || request.write("cancel") != 6)
    {
        m_output->appendPlainText(g_testConfig.m_failedText);
        return;
    }
    request.close();
    m_cancelRun->setEnabled(false);
    m_output->appendPlainText(g_testConfig.m_cancelPendingText);
}

void SplitViewerTest::startBatch(const QString& selection)
{
    if (m_running || m_closeRequested)
    {
        return;
    }
    std::vector<int32_t> ids;
    std::wstring error;
    if (!SplitViewerTestCaseHelper::parse(selection.toStdWString(), ids, error))
    {
        m_allTestsPassed = false;
        m_output->setPlainText(QString::fromStdWString(error));
        return;
    }
    const QString program =
        m_useDebug->isChecked() ? g_testConfig.m_debugProgramPath : g_testConfig.m_releaseProgramPath;
    const QString runner = m_useDebug->isChecked() ? g_testConfig.m_debugTestPath : g_testConfig.m_releaseTestPath;
    if (!QFileInfo(program).isFile() || !QFileInfo(runner).isFile())
    {
        m_allTestsPassed = false;
        m_output->setPlainText(g_testConfig.m_missingProgramText + program + QLatin1Char('\n') + runner);
        return;
    }
    m_reportDirectory = QDir(QCoreApplication::applicationDirPath())
                            .filePath(QStringLiteral("reports/") +
                                QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss-zzz-")) +
                                QString::fromStdString(CSystem::uuid()));
    QStringList arguments;
    arguments << QStringLiteral("--ui-audit") << m_reportDirectory;
    arguments << QStringLiteral("--cases=") + selection;
    foreach (const QString& argument, QCoreApplication::arguments())
    {
        if (argument.startsWith(QStringLiteral("--player=")) || argument == QStringLiteral("--native-trace"))
        {
            arguments << argument;
        }
    }
    m_allTestsPassed = false;
    m_output->clear();
    setRunning(true);
    m_uiProcess->setWorkingDirectory(QCoreApplication::applicationDirPath());
    m_uiProcess->setStandardOutputFile(QDir(m_reportDirectory).filePath(QStringLiteral("stdout.txt")));
    m_uiProcess->setStandardErrorFile(QDir(m_reportDirectory).filePath(QStringLiteral("stderr.txt")));
    if (!QDir().mkpath(m_reportDirectory))
    {
        m_output->setPlainText(g_testConfig.m_failedText);
        setRunning(false);
        return;
    }
    m_expectedCaseCount = static_cast<int32_t>(ids.size());
    m_uiProcess->start(runner, arguments);
}

void SplitViewerTest::setRunning(bool running)
{
    m_running = running;
    m_coreRun->setEnabled(!running);
    m_uiRun->setEnabled(!running);
    m_uiCases->setEnabled(!running);
    m_caseSelection->setEnabled(!running);
    m_instantRun->setEnabled(!running);
    m_pressureRun->setEnabled(!running);
    m_allRun->setEnabled(!running);
    m_cancelRun->setEnabled(running);
    m_useDebug->setEnabled(!running);
}

void SplitViewerTest::uiProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (!m_running)
    {
        return;
    }
    QFile report(QDir(m_reportDirectory).filePath(QStringLiteral("ui-results.txt")));
    const bool readable = report.open(QIODevice::ReadOnly | QIODevice::Text);
    const QByteArray result = readable ? report.readAll() : QByteArray();
    m_output->setPlainText(QString::fromUtf8(result));
    m_allTestsPassed = status == QProcess::NormalExit && exitCode == 0 && readable &&
        SplitViewerTestCaseHelper::reportSucceeded(result.toStdString(), m_expectedCaseCount);
    m_output->appendPlainText(g_testConfig.m_reportPrefix + m_reportDirectory);
    if (!m_allTestsPassed)
    {
        m_output->appendPlainText(g_testConfig.m_failedText);
    }
    setRunning(false);
    if (m_closeRequested)
    {
        close();
    }
}

void SplitViewerTest::uiProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart && m_running)
    {
        m_output->setPlainText(g_testConfig.m_startErrorPrefix + m_uiProcess->errorString());
        m_allTestsPassed = false;
        setRunning(false);
        if (m_closeRequested)
        {
            close();
        }
    }
}

void SplitViewerTest::closeEvent(QCloseEvent* event)
{
    if (m_running)
    {
        // 用例会临时修改系统拖动设置，必须让子进程完成自己的恢复路径。
        event->ignore();
        if (!m_closeRequested)
        {
            m_closeRequested = true;
            m_output->appendPlainText(g_testConfig.m_closePendingText);
        }
        return;
    }
    MainWindow::closeEvent(event);
}