#include "SplitViewerTest.h"
#include "SplitViewerUiTests.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "QtControls/ComboBox.h"
#include "QtControls/PlainTextEdit.h"
#include "QtControls/PushButton.h"
#include "QtControls/Widget.h"
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>

#include "SplitViewerCore/SplitViewerCoreAPI.h"

#include <vector>

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

SplitViewerTest::SplitViewerTest(QWidget* parent) :
MainWindow(parent),
m_output(nullptr),
m_allTestsPassed(false),
m_uiCases(nullptr),
m_coreRun(nullptr),
m_uiRun(nullptr),
m_uiProcess(new QProcess(this)),
m_running(false),
m_closeRequested(false)
{
    setWindowTitle(g_testConfig.m_title);
    resize(g_testConfig.m_windowSize);
    Widget* central = new Widget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);
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
        m_uiCases->setItemData(m_uiCases->count() - 1, name.left(3).toInt());
    }
    layout->addWidget(m_uiCases);
    m_uiRun = new PushButton(central);
    m_uiRun->setObjectName(QStringLiteral("uiRun"));
    m_uiRun->setText(g_testConfig.m_uiRunText);
    m_uiRun->setClickBreathTime(0);
    layout->addWidget(m_uiRun);
    connect(m_uiRun, SIGNAL(clicked()), this, SLOT(runUiTests()));
    layout->addWidget(m_output, 1);
    setCentralWidget(central);
    connect(m_coreRun, SIGNAL(clicked()), this, SLOT(runTests()));
    connect(m_uiProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
        this, SLOT(uiProcessFinished(int,QProcess::ExitStatus)));
    connect(m_uiProcess, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(uiProcessError(QProcess::ProcessError)));
    runTests();
}

SplitViewerTest::~SplitViewerTest()
{

}

bool SplitViewerTest::allTestsPassed() const
{
    return m_allTestsPassed && !m_running;
}

void SplitViewerTest::appendResult(const QString& name, bool passed, const QString& detail)
{
    const std::wstring suffix = (detail.isEmpty() ? QString() : QStringLiteral("：") + detail).toStdWString();
    m_output->appendPlainText(QString::fromStdWString(CStringManager::Format(g_testConfig.m_resultFormat.c_str(),
        passed ? L"通过" : L"失败", name.toStdWString().c_str(), suffix.c_str())));
}

void SplitViewerTest::runTests()
{
    if (m_running)
    {
        return;
    }
    m_output->clear();
    int passed = 0;
    int total = 0;
    SplitViewerCoreDocument document;
    ++total;
    document.baseRoot()->makeSplit(SPLITVIEWER_CORE_SPLIT_VERTICAL);
    const bool splitOk = document.baseRoot()->kind == SPLITVIEWER_CORE_NODE_SPLIT && document.baseRoot()->first && document.baseRoot()->second;
    appendResult(QStringLiteral("SVCORE-001 基础树创建和垂直分割"), splitOk);
    passed += splitOk ? 1 : 0;

    ++total;
    SplitViewerCoreLayer* layer = document.addLayer();
    const bool layerOk = layer && document.layerCount() == 1 && document.selectedLayer() == 0;
    appendResult(QStringLiteral("SVCORE-002 浮动图层创建"), layerOk);
    passed += layerOk ? 1 : 0;

    ++total;
    SplitViewerCoreDocument layerDeleteDocument;
    SplitViewerCoreLayer* deletableLayer = layerDeleteDocument.addLayer();
    if (deletableLayer)
    {
        deletableLayer->root->makeSplit(SPLITVIEWER_CORE_SPLIT_HORIZONTAL);
    }
    const bool deleteLayerOk = deletableLayer != NULL && layerDeleteDocument.deleteLayer(0) &&
        layerDeleteDocument.layerCount() == 0 && layerDeleteDocument.selectedLayer() == -1;
    appendResult(QStringLiteral("SVCORE-003 整层删除释放嵌套分屏"), deleteLayerOk);
    passed += deleteLayerOk ? 1 : 0;

    ++total;
    SplitViewerCoreRect first;
    SplitViewerCoreRect splitter;
    SplitViewerCoreRect second;
    SplitViewerCoreSplitNodeRects(SplitViewerCoreRect(), document.baseRoot(), first, splitter, second);
    const bool geometryOk = first.width() > 0.0 && second.width() > 0.0 && splitter.width() > 0.0;
    appendResult(QStringLiteral("SVCORE-004 分割区域计算"), geometryOk);
    passed += geometryOk ? 1 : 0;

    ++total;
    document.baseRoot()->first->view.path = L"test-image.png";
    document.baseRoot()->first->view.hasImage = true;
    document.baseRoot()->first->view.contentKind = SPLITVIEWER_CORE_CONTENT_IMAGE;
    std::vector<uint8_t> profile;
    const bool serializeOk = SplitViewerCoreSerializeProfile(document, profile);
    SplitViewerCoreDocument restored;
    const bool deserializeOk = serializeOk && SplitViewerCoreDeserializeProfile(profile, restored) && restored.layerCount() == 1 && restored.baseRoot()->kind == SPLITVIEWER_CORE_NODE_SPLIT;
    appendResult(QStringLiteral("SVCORE-005 配置序列化和反序列化"), deserializeOk);
    passed += deserializeOk ? 1 : 0;

    ++total;
    QImage thumbnail(16, 16, QImage::Format_ARGB32_Premultiplied);
    thumbnail.fill(QColor(32, 128, 192));
    QByteArray pngBytes;
    QBuffer pngBuffer(&pngBytes);
    pngBuffer.open(QIODevice::WriteOnly);
    thumbnail.save(&pngBuffer, "PNG");
    std::vector<uint8_t> png(reinterpret_cast<const uint8_t*>(pngBytes.constData()), reinterpret_cast<const uint8_t*>(pngBytes.constData()) + pngBytes.size());
    std::vector<uint8_t> package;
    std::vector<uint8_t> extracted;
    const bool packageOk = SplitViewerCoreBuildConfigPackage(png, profile, package) && SplitViewerCoreExtractEmbeddedConfig(package, extracted) && extracted == profile;
    appendResult(QStringLiteral("SVCORE-006 PNG配置包写入和提取"), packageOk);
    passed += packageOk ? 1 : 0;

    ++total;
    const bool scaleOk = SplitViewerCoreFitScale(400.0, 200.0, 200.0, 200.0) == 0.5;
    appendResult(QStringLiteral("SVCORE-007 图片自动适配比例"), scaleOk);
    passed += scaleOk ? 1 : 0;
    m_output->appendPlainText(QString::fromStdWString(CStringManager::Format(g_testConfig.m_summaryFormat.c_str(), passed, total)));
    m_allTestsPassed = passed == total;
}

void SplitViewerTest::runUiTests()
{
    if (m_running || m_closeRequested)
    {
        return;
    }
    m_reportDirectory = QDir(QCoreApplication::applicationDirPath()).filePath(
        QStringLiteral("reports/") + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss-zzz-")) +
        QString::fromStdString(CSystem::uuid()));
    QStringList arguments;
    arguments << QStringLiteral("--ui-audit") << m_reportDirectory;
    const int selected = m_uiCases->itemData(m_uiCases->currentIndex()).toInt();
    if (selected != 0)
    {
        arguments << QStringLiteral("--case=") + QString::number(selected);
    }
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
    m_uiProcess->start(QCoreApplication::applicationFilePath(), arguments);
}

void SplitViewerTest::setRunning(bool running)
{
    m_running = running;
    m_coreRun->setEnabled(!running);
    m_uiRun->setEnabled(!running);
    m_uiCases->setEnabled(!running);
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
        result.contains("total=") && result.contains(" failures=0\n");
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