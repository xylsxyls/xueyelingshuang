#include "SplitViewerTest.h"
#include "SplitViewerUiTests.h"
#include <QtWidgets/QComboBox>
#include <QtCore/QFile>
#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>

#include "SplitViewerCoreAPI.h"

#include <vector>

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

SplitViewerTest::SplitViewerTest(QWidget* parent) : QMainWindow(parent), m_output(NULL), m_allTestsPassed(false)
{
    setWindowTitle(QStringLiteral("SplitViewer测试"));
    resize(760, 520);
    QWidget* central = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(central);
    QPushButton* run = new QPushButton(QStringLiteral("运行Core测试"), central);
    m_output = new QPlainTextEdit(central);
    m_output->setReadOnly(true);
    layout->addWidget(run);
    m_uiCases=new QComboBox(central);
    m_uiCases->addItem(QStringLiteral("全部界面与兼容性回归（101—177）"),0);
    foreach (const QString& name,SplitViewerAuditCaseNames()) m_uiCases->addItem(name,name.left(3).toInt());
    layout->addWidget(m_uiCases);
    QPushButton* uiRun=new QPushButton(QStringLiteral("运行所选回归并保存报告"),central);
    layout->addWidget(uiRun);
    connect(uiRun,SIGNAL(clicked()),this,SLOT(runUiTests()));
    layout->addWidget(m_output, 1);
    setCentralWidget(central);
    connect(run, SIGNAL(clicked()), this, SLOT(runTests()));
    runTests();
}

SplitViewerTest::~SplitViewerTest()
{
}

bool SplitViewerTest::allTestsPassed() const
{
    return m_allTestsPassed;
}

void SplitViewerTest::appendResult(const QString& name, bool passed, const QString& detail)
{
    m_output->appendPlainText(QStringLiteral("[%1] %2%3").arg(passed ? QStringLiteral("通过") : QStringLiteral("失败"), name, detail.isEmpty() ? QString() : QStringLiteral("：") + detail));
}

void SplitViewerTest::runTests()
{
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
    SplitViewerCoreRect first;
    SplitViewerCoreRect splitter;
    SplitViewerCoreRect second;
    SplitViewerCoreSplitNodeRects(SplitViewerCoreRect(), document.baseRoot(), first, splitter, second);
    const bool geometryOk = first.width() > 0.0 && second.width() > 0.0 && splitter.width() > 0.0;
    appendResult(QStringLiteral("SVCORE-003 分割区域计算"), geometryOk);
    passed += geometryOk ? 1 : 0;

    ++total;
    document.baseRoot()->first->view.path = L"test-image.png";
    document.baseRoot()->first->view.hasImage = true;
    document.baseRoot()->first->view.contentKind = SPLITVIEWER_CORE_CONTENT_IMAGE;
    std::vector<uint8_t> profile;
    const bool serializeOk = SplitViewerCoreSerializeProfile(document, profile);
    SplitViewerCoreDocument restored;
    const bool deserializeOk = serializeOk && SplitViewerCoreDeserializeProfile(profile, restored) && restored.layerCount() == 1 && restored.baseRoot()->kind == SPLITVIEWER_CORE_NODE_SPLIT;
    appendResult(QStringLiteral("SVCORE-004 配置序列化和反序列化"), deserializeOk);
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
    appendResult(QStringLiteral("SVCORE-005 PNG配置包写入和提取"), packageOk);
    passed += packageOk ? 1 : 0;

    ++total;
    const bool scaleOk = SplitViewerCoreFitScale(400.0, 200.0, 200.0, 200.0) == 0.5;
    appendResult(QStringLiteral("SVCORE-006 图片自动适配比例"), scaleOk);
    passed += scaleOk ? 1 : 0;
    m_output->appendPlainText(QStringLiteral("\n结果：%1/%2 项通过").arg(passed).arg(total));
    m_allTestsPassed = passed == total;
}

void SplitViewerTest::runUiTests()
{
    const QString directory=QCoreApplication::applicationDirPath()+QStringLiteral("/reports/")+QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss-zzz"));
    const int selected=m_uiCases->itemData(m_uiCases->currentIndex()).toInt();
    hide();
    const int result=SplitViewerRunUiTests(directory,selected);
    show();
    m_allTestsPassed=result==0;
    QFile report(directory+QStringLiteral("/ui-results.txt"));
    if (report.open(QIODevice::ReadOnly)) m_output->setPlainText(QString::fromUtf8(report.readAll()));
    m_output->appendPlainText(QStringLiteral("报告目录：")+directory);
}