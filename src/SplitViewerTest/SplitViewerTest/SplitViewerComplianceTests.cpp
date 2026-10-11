#include "SplitViewerComplianceTests.h"
#include "SplitViewerTestWindow.h"
#include "SplitViewerTestUiHelper.h"
#include "SplitViewerTestCaseHelper.h"
#include "../../SplitViewer/SplitViewer/SplitViewerTaskManager.h"
#include "../../SplitViewer/SplitViewer/SplitViewerDialogSession.h"
#include "../../SplitViewer/SplitViewer/SplitViewerAboutDialogParam.h"
#include "../../SplitViewer/SplitViewer/Config.h"
#include "DialogManager/DialogManagerAPI.h"
#include <QtCore/QBuffer>
#include <QtGui/QImage>
#include <QtCore/QTimer>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QElapsedTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QtTest/QtTest>
#include <limits>

bool SplitViewerComplianceTests::run(int32_t id, const QString& directory, SplitViewerDialogSession* owner)
{
    switch (id)
    {
    case 217:
    {
        return configSnapshot();
    }
    case 218:
    {
        return selection();
    }
    case 219:
    {
        return dialogSession(owner);
    }
    case 220:
    {
        return documentCycles();
    }
    case 221:
    {
        return taskIsolation();
    }
    case 222:
    {
        return sceneSnapshot();
    }
    case 223:
    case 224:
    {
        return asyncImage(id, directory);
    }
    default:
    {
        return false;
    }
    }
}

bool SplitViewerComplianceTests::configSnapshot()
{
    SplitViewerCoreConfig policy;
    policy.m_defaultSplitRatio = 0.4;
    SplitViewerCoreDocument first;
    first.uninit();
    if (!policy.isValid() || !first.init(policy))
    {
        return false;
    }
    policy.m_defaultSplitRatio = 0.6;
    SplitViewerCoreDocument second;
    second.uninit();
    if (!second.init(policy))
    {
        return false;
    }
    first.baseRoot()->makeSplit(SPLITVIEWER_CORE_SPLIT_VERTICAL);
    second.baseRoot()->makeSplit(SPLITVIEWER_CORE_SPLIT_VERTICAL);
    const bool isolated = first.baseRoot()->ratio == 0.4 && second.baseRoot()->ratio == 0.6;
    SplitViewerCoreConfig invalid = policy;
    invalid.m_zoomStep = (std::numeric_limits<double>::quiet_NaN)();
    invalid.m_layerOffsetPeriod = 0;
    const bool rejected = !invalid.isValid() && !first.init(invalid) && first.baseRoot()->ratio == 0.4;
    std::vector<uint8_t> bytes;
    SplitViewerCoreDocument restored;
    restored.uninit();
    SplitViewerCoreConfig restorePolicy;
    restorePolicy.m_defaultSplitRatio = 0.4;
    const bool roundTrip = restored.init(restorePolicy) && SplitViewerCoreSerializeProfile(first, bytes) &&
        SplitViewerCoreDeserializeProfile(bytes, restored) && restored.config().m_defaultSplitRatio == 0.4 &&
        restored.baseRoot()->ratio == 0.4;
    first.uninit();
    first.uninit();
    const bool stopped =
        first.baseRoot() == nullptr && first.addLayer() == nullptr && first.lastResult() == SPLITVIEWER_CORE_NOT_INIT;
    const bool restarted =
        first.init(policy) && first.baseRoot()->isLeaf() && first.config().m_defaultSplitRatio == 0.6;
    QImage image(8, 8, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::blue);
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG"))
    {
        return false;
    }
    const std::vector<uint8_t> thumbnail(reinterpret_cast<const uint8_t*>(png.constData()),
        reinterpret_cast<const uint8_t*>(png.constData()) + png.size());
    std::vector<uint8_t> package;
    if (!SplitViewerCoreBuildConfigPackage(thumbnail, bytes, package, restorePolicy))
    {
        return false;
    }
    SplitViewerCoreConfig limitedPolicy = restorePolicy;
    limitedPolicy.m_maximumProfileBytes = 2;
    const std::vector<uint8_t> sentinel(1, 0x7F);
    std::vector<uint8_t> output = sentinel;
    const bool packageLimited = !SplitViewerCoreBuildConfigPackage(thumbnail, bytes, output, limitedPolicy) &&
        output == sentinel && !SplitViewerCoreExtractEmbeddedConfig(package, output, limitedPolicy) &&
        output == sentinel && !SplitViewerCoreExtractEmbeddedConfig(package, output, invalid) && output == sentinel;
    return isolated && rejected && roundTrip && stopped && restarted && packageLimited;
}

bool SplitViewerComplianceTests::selection()
{
    std::vector<int32_t> ids;
    std::wstring error;
    if (!SplitViewerTestCaseHelper::validate() ||
        !SplitViewerTestCaseHelper::parse(L"103,101,207-210,103", ids, error) || !error.empty())
    {
        return false;
    }
    const int32_t expected[] = {101, 103, 207, 208, 209, 210};
    if (ids != std::vector<int32_t>(expected, expected + 6))
    {
        return false;
    }
    const std::vector<int32_t> original = ids;
    const wchar_t* invalid[] = {L"", L"101,", L",101", L"100", L"229", L"101x", L"103-101", L"101--103", L"2147483648"};
    for (const wchar_t* input : invalid)
    {
        if (SplitViewerTestCaseHelper::parse(input, ids, error) || error.empty() || ids != original)
        {
            return false;
        }
    }
    const bool reportChecks = SplitViewerTestCaseHelper::reportSucceeded("101 sample PASS\ntotal=1 failures=0\n", 1) &&
        SplitViewerTestCaseHelper::reportSucceeded("total=2 failures=0\r\n", 2) &&
        !SplitViewerTestCaseHelper::reportSucceeded("", 1) &&
        !SplitViewerTestCaseHelper::reportSucceeded("total=0 failures=0\n", 1) &&
        !SplitViewerTestCaseHelper::reportSucceeded("total=1 failures=1\n", 1) &&
        !SplitViewerTestCaseHelper::reportSucceeded("total=2 failures=0\n", 1) &&
        !SplitViewerTestCaseHelper::reportSucceeded("total=1 failures=0 canceled=1\n", 1) &&
        !SplitViewerTestCaseHelper::reportSucceeded("total=1 failures=0\npartial", 1) &&
        !SplitViewerTestCaseHelper::reportSucceeded("total=1 failures=0\ntotal=1 failures=0\n", 1) &&
        !SplitViewerTestCaseHelper::reportSucceeded("total=2147483648 failures=0\n", 1);
    const std::vector<int32_t> pressure = SplitViewerTestCaseHelper::byType(SPLITVIEWER_TEST_PRESSURE);
    return reportChecks && pressure.size() == 1 && pressure[0] == 220;
}

bool SplitViewerComplianceTests::dialogSession(SplitViewerDialogSession* owner)
{
    if (!owner || !owner->init())
    {
        return false;
    }
    SplitViewerDialogSession duplicate;
    const bool refused = !duplicate.init();
    duplicate.uninit();
    duplicate.uninit();
    bool observed = false;
    QTimer driver;
    QObject::connect(&driver, &QTimer::timeout,
        /** 观察实际托管关于窗口并点击确认，记录内容视图是否存在
        */
        [&]()
        {
            QWidget* dialog = QApplication::activeModalWidget();
            if (dialog)
            {
                QPushButton* accept = dialog->findChild<QPushButton*>(QStringLiteral("aboutCloseButton"));
                if (accept)
                {
                    observed = dialog->findChild<QWidget*>(QStringLiteral("splitViewerAboutView")) != nullptr;
                    QTest::mouseClick(accept, Qt::LeftButton);
                }
            }
        });
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout,
        /** 验证超时后关闭当前模态窗口以收敛场景，不把超时记为通过
        */
        [&]()
        {
            if (QApplication::activeModalWidget())
            {
                QApplication::activeModalWidget()->close();
            }
        });
    driver.start(10);
    watchdog.start(5000);
    SplitViewerAboutDialogParam param;
    DialogManager::instance().makeDialog(param);
    driver.stop();
    watchdog.stop();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    return refused && observed && param.m_dialogId != 0;
}

bool SplitViewerComplianceTests::documentCycles()
{
    SplitViewerCoreDocument isolated;
    for (int32_t iteration = 0; iteration < 100; ++iteration)
    {
        SplitViewerCoreDocument document;
        SplitViewerCoreLayer* layer = document.addLayer();
        if (!layer)
        {
            return false;
        }
        layer->root->view.path = L"independent-fixture.png";
        layer->root->view.hasImage = true;
        layer->root->view.contentKind = SPLITVIEWER_CORE_CONTENT_IMAGE;
        layer->root->makeSplit(SPLITVIEWER_CORE_SPLIT_VERTICAL);
        std::vector<uint8_t> bytes;
        SplitViewerCoreDocument restored;
        if (!SplitViewerCoreSerializeProfile(document, bytes) || !SplitViewerCoreDeserializeProfile(bytes, restored) ||
            restored.layerCount() != 1 || !restored.layerAt(0)->root->first ||
            restored.layerAt(0)->root->first->view.path != L"independent-fixture.png" || isolated.layerCount() != 0 ||
            !isolated.baseRoot()->isLeaf() || isolated.baseRoot()->view.hasContent())
        {
            return false;
        }
    }
    return true;
}

bool SplitViewerComplianceTests::taskIsolation()
{
    QObject owner;
    SplitViewerTaskManager first(&owner);
    SplitViewerTaskManager second(&owner);
    int firstCompletions = 0;
    int secondCompletions = 0;
    bool valid = first.init() && second.init();
    valid = first.submit(
        false,
        /** 提供第一任务实例的短工作，读取其独立停止标志
        @param [in] exit 第一实例共享停止标志
        @return 执行时未收到停止请求
        */
        [](const std::atomic<bool>& exit)
        {
            return !exit.load();
        },
        /** 记录第一实例的完成回调次数，判定停止后是否仍交付结果
        @param [in] 未命名参数 后台结果，此回调只统计是否被调用
        */
        [&](bool)
        {
            ++firstCompletions;
        });
    first.stop();
    first.stop();
    valid = !first.submit(
                false,
                /** 为停止后提交的拒绝反例提供工作；正常拒绝时不得执行
                @param [in] 未命名参数 共享停止标志，该拒绝反例故意不使用
                @return 若错误执行则返回true，提交是否拒绝由外层独立判断
                */
                [](const std::atomic<bool>&)
                {
                    return true;
                },
                /** 为停止后拒绝提交提供空呈现回调，正常路径不应被调用
                @param [in] 未命名参数 后台实际结果，该拒绝反例不使用
                */
                [](bool)
                {

                }) &&
        valid;
    valid = second.submit(
                true,
                /** 提供第二实例的耗时队列工作，验证不受第一实例停止影响
                @param [in] exit 第二实例共享停止标志
                @return 执行时第二实例未收到停止请求
                */
                [](const std::atomic<bool>& exit)
                {
                    return !exit.load();
                },
                /** 累计第二实例成功或失败回报，用于核对独立完成次数
                @param [in] success 第二实例后台工作的实际结果
                */
                [&](bool success)
                {
                    secondCompletions += success ? 1 : -1;
                }) &&
        valid;
    const bool fullRejected = !second.submit(
        false,
        /** 为在途容量拒绝反例提供工作，拒绝时该工作不应执行
        @param [in] 未命名参数 共享停止标志，该容量反例故意不使用
        @return 若错误执行则返回true，拒绝由submit结果独立判定
        */
        [](const std::atomic<bool>&)
        {
            return true;
        },
        /** 为在途容量拒绝反例提供空回报，正常拒绝时不应交付
        @param [in] 未命名参数 后台实际结果，该容量反例不使用
        */
        [](bool)
        {

        });
    QElapsedTimer budget;
    budget.start();
    while ((first.busy() || second.busy()) && budget.elapsed() < 5000)
    {
        QTest::qWait(10);
    }
    valid = valid && fullRejected && !first.busy() && !second.busy() && firstCompletions == 0 && secondCompletions == 1;
    second.stop();
    budget.restart();
    while (second.busy() && budget.elapsed() < 5000)
    {
        QTest::qWait(10);
    }
    return valid && !second.busy();
}

bool SplitViewerComplianceTests::sceneSnapshot()
{
    SplitViewerCoreDocument source;
    source.baseRoot()->makeSplit(SPLITVIEWER_CORE_SPLIT_VERTICAL);
    source.baseRoot()->first->view.path = L"snapshot.png";
    source.baseRoot()->first->view.hasImage = true;
    source.baseRoot()->first->view.contentKind = SPLITVIEWER_CORE_CONTENT_IMAGE;
    source.baseRoot()->first->view.autoFit = false;
    source.baseRoot()->first->view.offsetX = 37.0;
    source.addLayer();
    SplitViewerCoreDocument copy;
    std::map<const SplitViewerCoreNode*, SplitViewerCoreNode*> nodes;
    if (!source.copyTo(copy, nodes) || copy.layerCount() != 1 ||
        nodes.at(source.baseRoot()->first) != copy.baseRoot()->first || copy.baseRoot() == source.baseRoot() ||
        copy.baseRoot()->first->view.offsetX != 37.0)
    {
        return false;
    }
    copy.baseRoot()->first->view.clear();
    const bool isolated =
        source.baseRoot()->first->view.path == L"snapshot.png" && source.baseRoot()->first->view.offsetX == 37.0;
    const auto mappings = nodes;
    SplitViewerCoreDocument stopped;
    stopped.uninit();
    SplitViewerCoreConfig limitedPolicy;
    limitedPolicy.m_maximumNodes = 1;
    SplitViewerCoreDocument oversized(limitedPolicy);
    oversized.setBaseRoot(new SplitViewerCoreNode);
    oversized.baseRoot()->makeSplit(SPLITVIEWER_CORE_SPLIT_VERTICAL);
    const SplitViewerCoreNode* originalRoot = copy.baseRoot();
    const bool limited = !oversized.copyTo(copy, nodes) && copy.baseRoot() == originalRoot && nodes == mappings;
    return isolated && limited && !stopped.copyTo(copy, nodes) && nodes == mappings && copy.layerCount() == 1 &&
        !copy.baseRoot()->first->view.hasContent();
}

bool SplitViewerComplianceTests::asyncImage(int32_t id, const QString& directory)
{
    const QString path = QDir(directory).filePath(QStringLiteral("async-image.png"));
    const QString invalid = QDir(directory).filePath(QStringLiteral("async-invalid.png"));
    QImage image(1024, 1024, QImage::Format_RGB32);
    const QColor red(230, 30, 40);
    image.fill(red);
    if (!image.save(path))
    {
        return false;
    }
    SplitViewerTestWindow window;
    QWidget* canvas = window.centralWidget();
    const QPoint center = SplitViewerTestUiHelper::expectedStage(canvas).center().toPoint();
    window.canvasFileDropped(path);
    if (id == 224)
    {
        window.close();
        QElapsedTimer budget;
        budget.start();
        while (window.isVisible() && budget.elapsed() < 5000)
        {
            QTest::qWait(10);
        }
        return !window.isVisible() && !window.processing() && window.m_errors.isEmpty();
    }
    if (!SplitViewerTestUiHelper::settle(&window) || !SplitViewerTestUiHelper::pixel(canvas, center, red))
    {
        return false;
    }
    QFile corrupt(invalid);
    if (!corrupt.open(QIODevice::WriteOnly) || corrupt.write("not an image") != 12)
    {
        return false;
    }
    corrupt.close();
    return SplitViewerTestUiHelper::drop(canvas, center, invalid) && !window.m_errors.isEmpty() &&
        SplitViewerTestUiHelper::pixel(canvas, center, red);
}