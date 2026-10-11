#include "SplitViewerUiTests.h"
#include "SplitViewerTestCaseHelper.h"
#include "SplitViewerCoreTests.h"
#include "SplitViewerTestWindow.h"
#include "SplitViewerComplianceTests.h"
#include "SplitViewerTestUiHelper.h"
#include "SplitViewerTestVideoHelper.h"
#include "SplitViewerTestNativeHelper.h"
#include "SplitViewerDialogTests.h"
#include "SplitViewerShadowTests.h"
#include "SplitViewerConfigTests.h"
#include "SplitViewerHarnessTests.h"
#include "SplitViewerMoveVideoTests.h"
#include "SplitViewerMoveUiTests.h"
#include "../../SplitViewer/SplitViewer/SplitViewerDialogHelper.h"
#include "../../SplitViewer/SplitViewer/SplitViewer.h"
#include "../../SplitViewer/SplitViewer/SplitViewerDialogSession.h"
#include "../../SplitViewer/SplitViewer/SplitViewerPlatform.h"
#include "DialogManager/DialogManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "QtControls/Widget.h"
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QDateTime>
#include <QtCore/QDataStream>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QCryptographicHash>
#include <QtGui/QScreen>
#include <QtGui/QContextMenuEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolBar>
#include <QtTest/QtTest>
#include <QtCore/QMimeData>
#include <QtCore/QUrl>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QLabel>
#include <algorithm>
#include <cmath>
#include <memory>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

int SplitViewerRunUiTests(const QString& reportDirectory, int selectedCase)
{
#ifdef Q_OS_WIN
    WNDCLASSW fixtureClass = {0};
    fixtureClass.lpfnWndProc = SplitViewerTestNativeHelper::splitViewerNativeFixtureProc;
    fixtureClass.style = CS_DBLCLKS;
    fixtureClass.hInstance = GetModuleHandleW(nullptr);
    fixtureClass.lpszClassName = L"SplitViewerNativeTestFixture";
    fixtureClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&fixtureClass);
#endif
    if (!SplitViewerTestCaseHelper::validate() ||
        (selectedCase != 0 &&
            SplitViewerTestCaseHelper::kCases.find(static_cast<SplitViewerTestCaseId>(selectedCase)) ==
                SplitViewerTestCaseHelper::kCases.end()))
    {
        return 2;
    }
    LogManager::instance().set(true, false);
    LogManager::instance().init();
    DialogManager::setLogCallback(SplitViewerDialogHelper::forwardLog);
    std::unique_ptr<SplitViewerDialogSession> dialogs(new SplitViewerDialogSession());
    if (!dialogs->init())
    {
        DialogManager::instance().uninit();
        DialogManager::releaseInstance();
        DialogManager::setLogCallback(nullptr);
        return 1;
    }
    QDir().mkpath(reportDirectory);
    QFile report(reportDirectory + QStringLiteral("/ui-results.txt"));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        dialogs.reset();
        DialogManager::setLogCallback(nullptr);
        return 1;
    }
    QTextStream out(&report);
    out.setCodec("UTF-8");
    int failures = 0, total = 0, nextId = 100;
    /** 登记一项独立断言的实际结果并更新报告计数
    @param [in] name 历史调用点名称，当前报告以固定用例目录名称为准
    @param [in] success 本项已计算出的实际判定
    */
    auto check = [&](const char* name, bool success)
    {
        const int id = ++nextId;
        if (selectedCase && selectedCase != id)
        {
            return;
        }
        ++total;
        Q_UNUSED(name);
        out << id << " "
            << QString::fromStdWString(
                   SplitViewerTestCaseHelper::kCases.at(static_cast<SplitViewerTestCaseId>(id)).m_name)
            << " " << (success ? "PASS" : "FAIL") << "\n";
        out.flush();
        failures += !success;
    };
    const QColor red(230, 30, 40), blue(20, 80, 230), green(30, 200, 70), gray(128, 128, 128), plus(70, 70, 70);
    const QString redPath = reportDirectory + QStringLiteral("/红色.png");
    const QString bluePath = reportDirectory + QStringLiteral("/蓝色.png");
    const QString greenPath = reportDirectory + QStringLiteral("/绿色.png");
    QImage input(120, 120, QImage::Format_RGB32);
    input.fill(red);
    input.save(redPath);
    input.fill(blue);
    input.save(bluePath);
    input.fill(green);
    input.save(greenPath);
    if (selectedCase == 0 || (selectedCase > 100 && selectedCase <= 117))
    {
        nextId = 100;
        SplitViewerTestWindow window;
        QWidget* canvas = window.centralWidget();
        const QRectF stage = SplitViewerTestUiHelper::expectedStage(canvas), inside = stage.adjusted(1, 1, -1, -1);
        const QPoint center = inside.center().toPoint();
        check("empty colors and plus",
            SplitViewerTestUiHelper::pixel(canvas, center, plus) &&
                SplitViewerTestUiHelper::pixel(canvas, center + QPoint(60, 0), gray));
        SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, center);
        check("plus cancel keeps empty",
            window.m_browseCount == 1 && SplitViewerTestUiHelper::pixel(canvas, center, plus));
        check("horizontal context split",
            SplitViewerTestUiHelper::chooseMenu(canvas, center + QPoint(70, 0), QStringLiteral("水平分割")));
        const QPoint upper(center.x(), qRound(inside.top() + (inside.height() - 2) / 4));
        const QPoint lower(center.x(), qRound(inside.bottom() - (inside.height() - 2) / 4));
        check("both split plus visible",
            SplitViewerTestUiHelper::pixel(canvas, upper, plus) && SplitViewerTestUiHelper::pixel(canvas, lower, plus));
        window.m_nextFile = redPath;
        SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, upper);
        check("upper plus loads visible image",
            window.m_browseCount == 2 && SplitViewerTestUiHelper::pixel(canvas, upper, red));
        window.m_nextFile = bluePath;
        SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, lower);
        check("lower plus loads visible image",
            window.m_browseCount == 3 && SplitViewerTestUiHelper::pixel(canvas, lower, blue) &&
                SplitViewerTestUiHelper::pixel(canvas, upper, red));
        SplitViewerTestUiHelper::wheel(canvas, upper, 120 * 100);
        check("zoom clipped to own split", SplitViewerTestUiHelper::pixel(canvas, lower, blue));
        SplitViewerTestUiHelper::drag(canvas, upper, upper + QPoint(30, 20));
        const QString profile = reportDirectory + QStringLiteral("/split.sv");
        window.m_nextFile = profile;
        SplitViewerTestUiHelper::toolbar(window, QStringLiteral("保存配置"));
        SplitViewerCoreDocument state;
        const bool loaded = SplitViewerTestUiHelper::readDocument(profile, state);
        check("save profile reflects real UI",
            loaded && state.baseRoot()->first && state.baseRoot()->first->view.path == redPath.toStdWString() &&
                state.baseRoot()->second->view.path == bluePath.toStdWString());
        check("image pan uses pixels",
            loaded && state.baseRoot()->first && std::abs(state.baseRoot()->first->view.offsetX - 30) < 0.01 &&
                std::abs(state.baseRoot()->first->view.offsetY - 20) < 0.01);
        check("nested vertical split", SplitViewerTestUiHelper::chooseMenu(canvas, lower, QStringLiteral("垂直分割")));
        const QPoint left(qRound(inside.left() + (inside.width() - 2) / 4), lower.y());
        const QPoint right(qRound(inside.right() - (inside.width() - 2) / 4), lower.y());
        check("nested content retained and plus visible",
            SplitViewerTestUiHelper::pixel(canvas, left, blue) && SplitViewerTestUiHelper::pixel(canvas, right, plus));
        check("drop uses actual target",
            SplitViewerTestUiHelper::drop(canvas, right, greenPath) &&
                SplitViewerTestUiHelper::pixel(canvas, right, green) &&
                SplitViewerTestUiHelper::pixel(canvas, left, blue));
        check("delete base split leaf",
            SplitViewerTestUiHelper::chooseMenu(canvas, left, QStringLiteral("删除")) &&
                SplitViewerTestUiHelper::pixel(canvas, lower, green));
        SplitViewerTestUiHelper::drag(canvas, center, center + QPoint(0, -70));
        SplitViewerTestUiHelper::toolbar(window, QStringLiteral("保存配置"));
        SplitViewerCoreDocument moved;
        check("splitter drag persists",
            SplitViewerTestUiHelper::readDocument(profile, moved) && moved.baseRoot()->ratio < 0.45 &&
                moved.baseRoot()->ratio > 0.1);
        check("delete collapse preserves image",
            SplitViewerTestUiHelper::chooseMenu(canvas, upper - QPoint(0, 100), QStringLiteral("删除")) &&
                SplitViewerTestUiHelper::pixel(canvas, center, green));
        check("root delete disabled",
            !SplitViewerTestUiHelper::chooseMenu(canvas, center, QStringLiteral("删除")) &&
                SplitViewerTestUiHelper::pixel(canvas, center, green));
        const int errors = window.m_errors.size();
        SplitViewerTestUiHelper::drop(canvas, center, reportDirectory + QStringLiteral("/missing.png"));
        check("bad image retains document",
            window.m_errors.size() == errors + 1 && SplitViewerTestUiHelper::pixel(canvas, center, green));
        canvas->grab().save(reportDirectory + QStringLiteral("/base.png"));
    }
    if (selectedCase == 0 || (selectedCase > 117 && selectedCase <= 131))
    {
        nextId = 117;
        SplitViewerTestWindow window;
        QWidget* canvas = window.centralWidget();
        const QRectF stage = SplitViewerTestUiHelper::expectedStage(canvas),
                     layer(stage.left() + stage.width() * .22, stage.top() + stage.height() * .22, stage.width() * .46,
                         stage.height() * .46);
        const QPoint center = layer.center().toPoint(), outside(qRound(stage.left() + stage.width() * .75), center.y());
        SplitViewerTestUiHelper::drop(canvas, center, redPath);
        check("new layer toolbar",
            SplitViewerTestUiHelper::toolbar(window, QStringLiteral("新建图层")) &&
                SplitViewerTestUiHelper::pixel(canvas, center, plus));
        window.m_nextFile = bluePath;
        SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, center);
        SplitViewerTestUiHelper::wheel(canvas, center, 120 * 100);
        const QImage zoomShot = canvas->grab().toImage();
        const QColor zoomCenter = QColor(zoomShot.pixel(center));
        const QColor zoomOutside = QColor(zoomShot.pixel(outside));
        check("floating zoom cannot escape layer", zoomCenter == blue && zoomOutside == red);
        SplitViewerTestUiHelper::drag(canvas, center, center + QPoint(100, 40));
        check("floating pan cannot escape layer", SplitViewerTestUiHelper::pixel(canvas, outside, red));
        canvas->grab().save(reportDirectory + QStringLiteral("/layer-clipping.png"));
        SplitViewerTestUiHelper::drag(canvas, center, center + QPoint(35, -25), Qt::ControlModifier);
        const QString profile = reportDirectory + QStringLiteral("/layer.sv");
        window.m_nextFile = profile;
        SplitViewerTestUiHelper::toolbar(window, QStringLiteral("保存配置"));
        SplitViewerCoreDocument state;
        bool saved = SplitViewerTestUiHelper::readDocument(profile, state);
        check("ctrl drag moves layer only",
            saved && state.layerCount() == 1 &&
                std::abs(state.layerAt(0)->rect.left - (.22 + 35 / stage.width())) < .001 &&
                std::abs(state.layerAt(0)->root->view.offsetX - 100) < .01);
        const QPoint edge(qRound(layer.right() + 35 - 3), qRound(layer.center().y() - 25));
        SplitViewerTestUiHelper::drag(canvas, edge, edge + QPoint(55, 0));
        SplitViewerTestUiHelper::toolbar(window, QStringLiteral("保存配置"));
        SplitViewerCoreDocument resized;
        check("layer right edge resize",
            SplitViewerTestUiHelper::readDocument(profile, resized) && resized.layerCount() == 1 &&
                resized.layerAt(0)->rect.width() > 0.51);
        check("layer split",
            SplitViewerTestUiHelper::chooseMenu(canvas, center + QPoint(35, -25), QStringLiteral("水平分割")));
        const QPoint low(center.x() + 35 + qRound(55.0 / 2), qRound(layer.top() - 25 + layer.height() * .75));
        check("layer split lower plus", SplitViewerTestUiHelper::pixel(canvas, low, plus));
        window.m_nextFile = greenPath;
        SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, low);
        check("layer split lower image", SplitViewerTestUiHelper::pixel(canvas, low, green));
        SplitViewerTestUiHelper::toolbar(window, QStringLiteral("保存配置"));
        const QString exportPath = reportDirectory + QStringLiteral("/export.png");
        window.m_nextFile = exportPath;
        SplitViewerTestUiHelper::toolbar(window, QStringLiteral("另存图片"));
        const QImage exported(exportPath);
        const QSize screen = QApplication::desktop()->screenGeometry(&window).size();
        const double aspect = static_cast<double>(QApplication::desktop()->screenGeometry(&window).width()) /
            static_cast<double>(QApplication::desktop()->screenGeometry(&window).height());
        QSize target(4000, static_cast<int>(4000 / aspect));
        target.scale(screen, Qt::KeepAspectRatio);
        check("export uses screen-fit stage only",
            !exported.isNull() && exported.size() == target &&
                QColor(exported.pixel(exported.width() / 2, exported.height() / 8)) == red);
        const QImage thumb(profile);
        const int thumbnailHeight = qRound(320.0 /
            (static_cast<double>(QApplication::desktop()->screenGeometry(&window).width()) /
                static_cast<double>(QApplication::desktop()->screenGeometry(&window).height())));
        check("sv PNG thumbnail bounded and stage only",
            !thumb.isNull() && thumb.width() == 320 && std::abs(thumb.height() - thumbnailHeight) <= 1);
        SplitViewerTestWindow restored;
        check("drop sv restores layers",
            SplitViewerTestUiHelper::drop(
                restored.centralWidget(), restored.centralWidget()->rect().center(), profile));
        check("loaded layer image visible", SplitViewerTestUiHelper::pixel(restored.centralWidget(), low, green));
        const int errors = restored.m_errors.size();
        QFile invalid(reportDirectory + QStringLiteral("/invalid.sv"));
        invalid.open(QIODevice::WriteOnly);
        invalid.write("bad");
        invalid.close();
        SplitViewerTestUiHelper::drop(restored.centralWidget(), low, invalid.fileName());
        check("invalid profile keeps existing layout",
            restored.m_errors.size() == errors + 1 &&
                SplitViewerTestUiHelper::pixel(restored.centralWidget(), low, green));
        SplitViewerTestUiHelper::drag(restored.centralWidget(), low, low + QPoint(10, 10));
        check("interaction after reload remains valid", restored.m_errors.size() == errors + 1);
    }
    if (selectedCase == 0 || (selectedCase > 131 && selectedCase <= 137))
    {
        nextId = 131;
        SplitViewerTestWindow window;
        QWidget* canvas = window.centralWidget();
        const QRect before = window.geometry();
        QTest::mouseDClick(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
        check("double click margin does not fullscreen", !window.isFullScreen());
        QTest::mouseDClick(canvas, Qt::LeftButton, Qt::NoModifier, canvas->rect().center() + QPoint(70, 0));
        QTest::qWait(50);
        check("double click fullscreen content only",
            window.isFullScreen() && !window.findChild<QToolBar*>()->isVisible() && canvas->size() == window.size());
        window.grab().save(reportDirectory + QStringLiteral("/fullscreen.png"));
        SplitViewerTestUiHelper::key(&window, Qt::Key_Escape);
        QTest::qWait(30);
        check("escape restores geometry and toolbar",
            !window.isFullScreen() && window.findChild<QToolBar*>()->isVisible() && window.geometry() == before);
        window.showMaximized();
        QTest::qWait(100);
        SplitViewerTestUiHelper::key(&window, Qt::Key_F11);
        QTest::qWait(30);
        check("F11 enters fullscreen", window.isFullScreen() && !window.findChild<QToolBar*>()->isVisible());
        SplitViewerTestUiHelper::key(&window, Qt::Key_F11);
        QTest::qWait(30);
        check("F11 restores maximized state", window.isMaximized() && window.findChild<QToolBar*>()->isVisible());
        window.showNormal();
        QTest::qWait(30);
        const QPoint center = SplitViewerTestUiHelper::expectedStage(canvas).center().toPoint();
        SplitViewerTestUiHelper::drop(canvas, center, redPath);
        SplitViewerTestUiHelper::chooseMenu(canvas, center, QStringLiteral("水平分割"));
        SplitViewerTestUiHelper::key(&window, Qt::Key_F1);
        const QString profile = reportDirectory + QStringLiteral("/border.sv");
        window.m_nextFile = profile;
        SplitViewerTestUiHelper::toolbar(window, QStringLiteral("保存配置"));
        SplitViewerCoreDocument state;
        check("F1 hides borders in persisted state",
            SplitViewerTestUiHelper::readDocument(profile, state) && !state.borderVisible());
    }
#ifdef Q_OS_WIN
    if (selectedCase == 0 || (selectedCase > 137 && selectedCase <= 140))
    {
        nextId = 137;
        SplitViewerTestWindow window;
        HWND fixture = CreateWindowExW(WS_EX_TOOLWINDOW, L"SplitViewerNativeTestFixture", L"SplitViewer native fixture",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, 60, 80, 240, 180, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        RECT before = {0};
        GetWindowRect(fixture, &before);
        const LONG_PTR style = GetWindowLongPtrW(fixture, GWL_STYLE);
        QWidget* container = SplitViewerEmbedForeignWindow(reinterpret_cast<WId>(fixture), window.centralWidget());
        if (container)
        {
            container->setGeometry(50, 50, 180, 130);
            container->show();
        }
        QTest::qWait(30);
        check("native embed creates child",
            container && IsWindow(fixture) && (GetWindowLongPtrW(fixture, GWL_STYLE) & WS_CHILD) &&
                GetParent(fixture) != nullptr);
        check("native snapshot", container && !SplitViewerForeignWindowSnapshot(container).isNull());
        SplitViewerDetachForeignWindow(container);
        QTest::qWait(30);
        RECT after = {0};
        GetWindowRect(fixture, &after);
        check("native detach restores surviving original",
            IsWindow(fixture) && GetParent(fixture) == nullptr && GetWindowLongPtrW(fixture, GWL_STYLE) == style &&
                EqualRect(&before, &after));
        DestroyWindow(fixture);
    }
#endif
    if (selectedCase == 0 || (selectedCase > 140 && selectedCase <= 153))
    {
        nextId = 140;
        // 独立手写旧版配置，不调用产品序列化器生成预期。
        const QString fixture = QStringLiteral(
            "[SplitViewer]\r\nVersion=2\r\nStageAspect=1.3333333333\r\nBorderVisible=0\r\nLayerCount=0\r\n[Base]\r\nRoot=0\r\n[BaseNode0]\r\nKind=Leaf\r\nPath=中文 path.png\r\nHasImage=1\r\nAutoFit=0\r\nScale=1.5\r\nOffsetX=37\r\nOffsetY=-21\r\n");
        SplitViewerCoreDocument document;
        check("legacy fields decoded independently",
            SplitViewerCoreDeserializeProfile(SplitViewerTestUiHelper::utf16(fixture), document) &&
                document.baseRoot()->view.path == L"中文 path.png" && document.baseRoot()->view.offsetX == 37 &&
                document.baseRoot()->view.offsetY == -21 && !document.borderVisible());
        const std::wstring original = document.baseRoot()->view.path;
        QString cycle = fixture;
        cycle.replace(QStringLiteral("Kind=Leaf"), QStringLiteral("Kind=Split\r\nFirst=0\r\nSecond=0"));
        check("cyclic profile rejected without replacing state",
            !SplitViewerCoreDeserializeProfile(SplitViewerTestUiHelper::utf16(cycle), document) &&
                document.baseRoot()->view.path == original);
        QString absent = cycle;
        absent.replace(QStringLiteral("First=0"), QStringLiteral("First=99"));
        check("missing node rejected",
            !SplitViewerCoreDeserializeProfile(SplitViewerTestUiHelper::utf16(absent), document) &&
                document.baseRoot()->view.path == original);
        std::vector<uint8_t> odd = SplitViewerTestUiHelper::utf16(fixture);
        odd.pop_back();
        check("odd UTF16 rejected", !SplitViewerCoreDeserializeProfile(odd, document));
        check("unsupported profile version rejected",
            !SplitViewerCoreDeserializeProfile(SplitViewerTestUiHelper::utf16(QString(fixture).replace(
                                                   QStringLiteral("Version=2"), QStringLiteral("Version=999"))),
                document));
        QImage thumb(8, 6, QImage::Format_RGB32);
        thumb.fill(red);
        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        thumb.save(&buffer, "PNG");
        std::vector<uint8_t> package, extracted, pngVector(png.begin(), png.end()),
            config = SplitViewerTestUiHelper::utf16(fixture);
        SplitViewerCoreBuildConfigPackage(pngVector, config, package);
        bool crcRejected = false;
        for (size_t i = 4; i + 4 < package.size(); ++i)
        {
            if (package[i] == 's' && package[i + 1] == 'v' && package[i + 2] == 'C' && package[i + 3] == 'f')
            {
                package[i + 4] ^= 1;
                crcRejected = !SplitViewerCoreExtractEmbeddedConfig(package, extracted);
                break;
            }
        }
        check("corrupt svCf CRC rejected", crcRejected);
        const QByteArray marker("\r\n--SPLITVIEWER_CONFIG_UTF16LE--\r\n");
        std::vector<uint8_t> legacy(pngVector);
        legacy.insert(legacy.end(), marker.begin(), marker.end());
        legacy.insert(legacy.end(), config.begin(), config.end());
        check("old trailing marker accepted",
            SplitViewerCoreExtractEmbeddedConfig(legacy, extracted) && extracted == config);
        SplitViewerCoreNode node;
        node.makeSplit(SPLITVIEWER_CORE_SPLIT_HORIZONTAL);
        SplitViewerCoreRect a, line, b;
        SplitViewerCoreSplitNodeRects(SplitViewerCoreRect(10, 20, 810, 620), &node, a, line, b);
        check("split geometry exact absolute bounds",
            a.left == 10 && a.top == 20 && a.bottom == 319 && line.top == 319 && line.bottom == 321 && b.top == 321 &&
                b.bottom == 620);
        SplitViewerCoreSplitNodeRects(SplitViewerCoreRect(10, 20, 810, 620), &node, a, line, b, 0);
        check("borderless geometry has no separator gap", a.bottom == 320 && line.height() == 0 && b.top == 320);
        SplitViewerCoreLeafState view;
        view.hasImage = true;
        SplitViewerCoreZoom(view, 2, 240, false);
        check("wheel notch arithmetic", std::abs(view.scale - 2 * 1.05 * 1.05) < 1e-9 && !view.autoFit);
        SplitViewerCoreZoom(view, 2, 120, true);
        check("ctrl fine wheel arithmetic", std::abs(view.scale - 2 * 1.05 * 1.05 * 1.01) < 1e-9);
        SplitViewerCoreZoom(view, 2, 120000, false);
        const bool maximum = view.scale == 100;
        SplitViewerCoreZoom(view, 2, -120000, false);
        check("zoom min max bounds", maximum && view.scale == .1);
        view.scale = 2;
        view.offsetX = 30;
        view.offsetY = -10;
        SplitViewerCoreResizeView(view, 100, 100, 200, 300);
        check("manual view resize preserves pixel proportions",
            view.scale == 5 && view.offsetX == 60 && view.offsetY == -30);
    }
#ifdef Q_OS_WIN
    if (selectedCase == 0 || (selectedCase > 153 && selectedCase <= 156))
    {
        nextId = 153;
        SplitViewerTestWindow window;
        QWidget* canvas = window.centralWidget();
        SplitViewerTestUiHelper::chooseMenu(canvas, canvas->rect().center(), QStringLiteral("垂直分割"));
        const QRectF stage = SplitViewerTestUiHelper::expectedStage(canvas);
        const QPoint target =
            canvas->mapToGlobal(QPoint(qRound(stage.left() + stage.width() * .25), qRound(stage.center().y())));
        POINT savedCursor;
        GetCursorPos(&savedCursor);
        HWND fixture = CreateWindowExW(WS_EX_TOOLWINDOW, L"SplitViewerNativeTestFixture", L"SplitViewer drag fixture",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, 30, 40, 240, 180, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        const LONG_PTR style = GetWindowLongPtrW(fixture, GWL_STYLE);
        SetWindowPos(
            reinterpret_cast<HWND>(window.winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        SetWindowPos(fixture, HWND_TOPMOST, 30, 40, 240, 180, SWP_SHOWWINDOW);
        QTest::qWait(50);
        check("native hook drag in to empty leaf",
            SplitViewerTestNativeHelper::nativeDrag(QPoint(80, 110), target) && GetParent(fixture) != nullptr &&
                (GetWindowLongPtrW(fixture, GWL_STYLE) & WS_CHILD));
        const bool wasEmbedded = GetParent(fixture) != nullptr;
        const bool draggedOut = SplitViewerTestNativeHelper::nativeDrag(target, QPoint(180, 100));
        check("native drag out restores top level",
            wasEmbedded && draggedOut && IsWindow(fixture) && GetParent(fixture) == nullptr &&
                GetWindowLongPtrW(fixture, GWL_STYLE) == style);
        RECT detached = {0};
        GetWindowRect(fixture, &detached);
        const QPoint from((detached.left + detached.right) / 2, (detached.top + detached.bottom) / 2);
        SplitViewerTestNativeHelper::nativeDrag(from, target);
        const bool embeddedAgain = GetParent(fixture) != nullptr;
        window.close();
        QTest::qWait(30);
        check("closing viewer restores dragged window",
            embeddedAgain && IsWindow(fixture) && GetParent(fixture) == nullptr &&
                GetWindowLongPtrW(fixture, GWL_STYLE) == style);
        DestroyWindow(fixture);
        SetCursorPos(savedCursor.x, savedCursor.y);
    }
#endif
    if (selectedCase == 0 || selectedCase == 157 || selectedCase == 158)
    {
        nextId = 156;
        SplitViewerTestWindow window;
        QWidget* canvas = window.centralWidget();
        const QPoint center = canvas->rect().center();
        SplitViewerTestUiHelper::drop(canvas, center, redPath);
        bool formats = true;
        foreach (const QString& extension, QStringList() << "png" << "jpg" << "bmp" << "tiff")
        {
            const QString file = reportDirectory + "/format." + extension;
            window.m_nextFile = file;
            SplitViewerTestUiHelper::toolbar(window, QStringLiteral("另存图片"));
            const QImage decoded(file);
            if (decoded.isNull())
            {
                formats = false;
            }
            else
            {
                const QColor color(decoded.pixel(decoded.width() / 2, decoded.height() / 2));
                formats = formats && std::abs(color.red() - red.red()) <= 3 &&
                    std::abs(color.green() - red.green()) <= 3 && std::abs(color.blue() - red.blue()) <= 3;
            }
        }
        const int errors = window.m_errors.size();
        window.m_nextFile = reportDirectory + "/unsupported.gif";
        SplitViewerTestUiHelper::toolbar(window, QStringLiteral("另存图片"));
        check("Qt image format exports and unsupported GIF feedback",
            formats && window.m_errors.size() == errors + 1 && SplitViewerTestUiHelper::pixel(canvas, center, red));
        SplitViewerTestUiHelper::wheel(canvas, center, 120);
        QLabel* tip = canvas->findChild<QLabel*>();
        const bool shown = tip && tip->isVisible() && tip->text() == QStringLiteral("105%");
        QTest::qWait(1100);
        check("zoom percentage popup expires", shown && !tip->isVisible());
    }
#ifdef Q_OS_WIN
    if (selectedCase == 0 || selectedCase == 159 || selectedCase == 160)
    {
        nextId = 158;
        SplitViewerTestWindow window;
        QWidget* canvas = window.centralWidget();
        SplitViewerTestUiHelper::chooseMenu(canvas, canvas->rect().center(), QStringLiteral("垂直分割"));
        const QRectF stage = SplitViewerTestUiHelper::expectedStage(canvas);
        const QPoint left =
            canvas->mapToGlobal(QPoint(qRound(stage.left() + stage.width() * .25), qRound(stage.center().y())));
        const QPoint right =
            canvas->mapToGlobal(QPoint(qRound(stage.left() + stage.width() * .75), qRound(stage.center().y())));
        POINT savedCursor;
        GetCursorPos(&savedCursor);
        SetWindowPos(
            reinterpret_cast<HWND>(window.winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        HWND first = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"SplitViewerNativeTestFixture",
            L"SplitViewer first fixture", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 30, 40, 240, 180, nullptr, nullptr,
            GetModuleHandleW(nullptr), nullptr);
        QTest::qWait(20);
        SplitViewerTestNativeHelper::nativeDrag(QPoint(80, 110), left);
        HWND second = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"SplitViewerNativeTestFixture",
            L"SplitViewer second fixture", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 30, 40, 240, 180, nullptr, nullptr,
            GetModuleHandleW(nullptr), nullptr);
        QTest::qWait(20);
        SplitViewerTestNativeHelper::nativeDrag(QPoint(80, 110), right);
        const bool both =
            GetParent(first) != nullptr && GetParent(second) != nullptr && IsWindow(first) && IsWindow(second);
        const bool videoFilledBefore = SplitViewerTestNativeHelper::splitViewerNativeVideoFills(first) &&
            SplitViewerTestNativeHelper::splitViewerNativeVideoFills(second);
        RECT firstBefore = {0};
        RECT secondBefore = {0};
        GetWindowRect(first, &firstBefore);
        GetWindowRect(second, &secondBefore);
        const bool nativeDoubleClick = SplitViewerTestNativeHelper::nativeDoubleClick(
            QPoint((firstBefore.left + firstBefore.right) / 2, (firstBefore.top + firstBefore.bottom) / 2));
        QTest::qWait(120);
        RECT firstFullscreen = {0};
        RECT secondFullscreen = {0};
        GetWindowRect(first, &firstFullscreen);
        GetWindowRect(second, &secondFullscreen);
        const bool embeddedPaneScaled =
            firstFullscreen.right - firstFullscreen.left > firstBefore.right - firstBefore.left &&
            secondFullscreen.right - secondFullscreen.left > secondBefore.right - secondBefore.left;
        const bool videoFilledFullscreen = SplitViewerTestNativeHelper::splitViewerNativeVideoFills(first) &&
            SplitViewerTestNativeHelper::splitViewerNativeVideoFills(second);
        const bool nativePlayerStayedEmbedded = GetParent(first) != nullptr && GetParent(second) != nullptr;
        window.leaveFullscreen();
        QTest::qWait(40);
        const bool videoFilledAfter = SplitViewerTestNativeHelper::splitViewerNativeVideoFills(first) &&
            SplitViewerTestNativeHelper::splitViewerNativeVideoFills(second);
        window.close();
        QTest::qWait(30);
        check("two native panes restored independently on close",
            both && IsWindow(first) && IsWindow(second) && GetParent(first) == nullptr && GetParent(second) == nullptr);
        check("embedded panes scale with content fullscreen",
            nativeDoubleClick && embeddedPaneScaled && nativePlayerStayedEmbedded && videoFilledBefore &&
                videoFilledFullscreen && videoFilledAfter);
        DestroyWindow(first);
        DestroyWindow(second);
        SetCursorPos(savedCursor.x, savedCursor.y);
    }
#endif
#ifdef Q_OS_WIN
    if (selectedCase == 0 || (selectedCase >= 161 && selectedCase <= 168))
    {
        nextId = 160;
        SplitViewerTestWindow window;
        window.setGeometry(800, 130, 880, 580);
        QWidget* canvas = window.centralWidget();
        const HWND viewer = reinterpret_cast<HWND>(window.winId());
        ShowWindow(viewer, SW_SHOWNORMAL);
        SetWindowPos(viewer, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        QString playerPath = QDir(QApplication::applicationDirPath())
                                 .absoluteFilePath(QStringLiteral("../LumaPlayer/LumaPlayer1.0.exe"));
        if (!QFile::exists(playerPath))
        {
            playerPath = QDir(QApplication::applicationDirPath())
                             .absoluteFilePath(QStringLiteral("../common/LumaPlayer/LumaPlayer1.0.exe"));
        }
        foreach (const QString& argument, QApplication::arguments())
        {
            if (argument.startsWith(QStringLiteral("--player=")))
            {
                playerPath = argument.mid(9);
            }
        }
        const QString videoPath = QDir(reportDirectory).absoluteFilePath(QStringLiteral("quadrants.avi"));
        const bool fixtureReady = SplitViewerTestVideoHelper::writeVideoFixture(videoPath);
        QProcess player;
        if (QApplication::arguments().contains(QStringLiteral("--native-trace")))
        {
            QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
            environment.insert(QStringLiteral("QT_LOGGING_RULES"), QStringLiteral("qt.qpa.events=true"));
            environment.insert(
                QStringLiteral("QT_MESSAGE_PATTERN"), QStringLiteral("%{time yyyy-MM-dd hh:mm:ss.zzz} %{message}"));
            player.setProcessEnvironment(environment);
            player.setStandardErrorFile(reportDirectory + QStringLiteral("/player-qpa.log"));
        }
        player.setWorkingDirectory(QFileInfo(playerPath).absolutePath());
        if (fixtureReady)
        {
            player.start(playerPath, QStringList() << videoPath);
        }
        const bool started = fixtureReady && player.waitForStarted(5000);
        QPair<DWORD, HWND> found(started ? player.pid()->dwProcessId : 0, nullptr);
        for (int i = 0; started && !found.second && i < 50; ++i)
        {
            QTest::qWait(100);
            found.second = CSystem::findVisibleWindowByProcessId(found.first);
        }
        const HWND hwnd = found.second;
        out << "player=" << playerPath << " pid=" << found.first << " fixture=" << videoPath << "\n";
        QFile binary(playerPath);
        if (binary.open(QIODevice::ReadOnly))
        {
            out << "player_sha256=" << QCryptographicHash::hash(binary.readAll(), QCryptographicHash::Sha256).toHex()
                << "\n";
        }
        POINT originalCursor;
        GetCursorPos(&originalCursor);
        if (hwnd)
        {
            QTest::qWait(2000);
            SetWindowPos(hwnd, HWND_TOPMOST, 30, 100, 640, 400, SWP_SHOWWINDOW);
            QTest::qWait(500);
            SetCursorPos(300, 102);
            QTest::qWait(650);
        }
        QRect pane = SplitViewerTestUiHelper::expectedStage(canvas).adjusted(1, 1, -1, -1).toAlignedRect();
        pane.moveTopLeft(canvas->mapToGlobal(pane.topLeft()));
        const POINT sourcePoint = {300, 120};
        RECT playerRect = {0};
        GetWindowRect(hwnd, &playerRect);
        const HWND sourceAtPoint = GetAncestor(WindowFromPoint(sourcePoint), GA_ROOT);
        out << "source=" << reinterpret_cast<quintptr>(hwnd) << " hit=" << reinterpret_cast<quintptr>(sourceAtPoint)
            << " rect=" << playerRect.left << "," << playerRect.top << "," << playerRect.right << ","
            << playerRect.bottom << "\n";
        const bool dragged =
            hwnd && sourceAtPoint == hwnd && SplitViewerTestNativeHelper::nativeDrag(QPoint(300, 120), pane.center());
        SetCursorPos(20, 900);
        QTest::qWait(600);
        SplitViewerTestVideoHelper::screenPane(pane).save(reportDirectory + QStringLiteral("/161-embedded.png"));
        check("real player drag shows fitted video pixels",
            dragged && GetParent(hwnd) &&
                SplitViewerTestVideoHelper::videoPixelsFill(SplitViewerTestVideoHelper::screenPane(pane)) &&
                SplitViewerTestVideoHelper::videoIsAdvancing(pane));

        const bool doubled = hwnd && GetParent(hwnd) && SplitViewerTestNativeHelper::nativeDoubleClick(pane.center());
        SetCursorPos(20, 900);
        QTest::qWait(600);
        const QRect full = QApplication::desktop()->screenGeometry(&window).adjusted(1, 1, -1, -1);
        SplitViewerTestVideoHelper::screenPane(full).save(reportDirectory + QStringLiteral("/162-fullscreen.png"));
        check("real player double click scales video and keeps playing",
            doubled && window.isFullScreen() && GetParent(hwnd) &&
                SplitViewerTestVideoHelper::videoPixelsFill(SplitViewerTestVideoHelper::screenPane(full)) &&
                SplitViewerTestVideoHelper::videoIsAdvancing(full));

        const bool doubledBack =
            hwnd && GetParent(hwnd) && SplitViewerTestNativeHelper::nativeDoubleClick(full.center());
        QTest::qWait(300);
        SplitViewerTestVideoHelper::screenPane(pane).save(reportDirectory + QStringLiteral("/163-restored.png"));
        check("real player leaves fullscreen with visible video",
            doubledBack && hwnd && !window.isFullScreen() && GetParent(hwnd) &&
                SplitViewerTestVideoHelper::videoPixelsFill(SplitViewerTestVideoHelper::screenPane(pane)));

        window.setGeometry(760, 150, 1000, 650);
        QTest::qWait(300);
        pane = SplitViewerTestUiHelper::expectedStage(canvas).adjusted(1, 1, -1, -1).toAlignedRect();
        pane.moveTopLeft(canvas->mapToGlobal(pane.topLeft()));
        check("real player follows host move and resize",
            hwnd && SplitViewerTestVideoHelper::videoPixelsFill(SplitViewerTestVideoHelper::screenPane(pane)));

        SplitViewerTestUiHelper::chooseMenu(canvas, canvas->rect().center(), QStringLiteral("水平分割"));
        const QRectF splitStage = SplitViewerTestUiHelper::expectedStage(canvas).adjusted(1, 1, -1, -1);
        QRect upper(qRound(splitStage.left()), qRound(splitStage.top()), qRound(splitStage.width()),
            qRound((splitStage.height() - 2) / 2));
        upper.moveTopLeft(canvas->mapToGlobal(upper.topLeft()));
        SplitViewerTestUiHelper::drop(canvas,
            QPoint(qRound(splitStage.center().x()), qRound(splitStage.top() + splitStage.height() * .75)), bluePath);
        SetCursorPos(20, 900);
        QTest::qWait(1300);
        SplitViewerTestVideoHelper::screenPane(pane).save(reportDirectory + QStringLiteral("/165-split.png"));
        const QImage splitScreen = SplitViewerTestVideoHelper::screenPane(pane);
        check("real player split video and neighbor image stay visible",
            hwnd && SplitViewerTestVideoHelper::videoPixelsFill(SplitViewerTestVideoHelper::screenPane(upper)) &&
                !splitScreen.isNull() &&
                QColor(splitScreen.pixel(splitScreen.width() / 2, splitScreen.height() * 3 / 4)) == blue);

        const bool paused = hwnd && SplitViewerTestNativeHelper::nativePlayerClick(hwnd, upper.center());
        SetCursorPos(20, 900);
        QTest::qWait(1300);
        SplitViewerTestVideoHelper::screenPane(upper).save(reportDirectory + QStringLiteral("/166-paused.png"));
        const bool stoppedFrames = paused &&
            SplitViewerTestVideoHelper::videoPixelsFill(SplitViewerTestVideoHelper::screenPane(upper)) &&
            !SplitViewerTestVideoHelper::videoIsAdvancing(upper);
        const bool resumed = hwnd && SplitViewerTestNativeHelper::nativePlayerClick(hwnd, upper.center());
        // Both the host and LumaPlayer defer a click to distinguish double clicks.
        // Observe state until that documented delay plus one full fixture cycle.
        const int resumeBudget = GetDoubleClickTime() + QApplication::doubleClickInterval() + 800;
        QElapsedTimer resumeTimer;
        resumeTimer.start();
        const bool resumedFrames = resumed && SplitViewerTestVideoHelper::videoIsAdvancing(upper, resumeBudget, &out);
        out << "166 resume budget=" << resumeBudget << " elapsed=" << resumeTimer.elapsed() << "ms\n";
        SplitViewerTestVideoHelper::screenPane(upper).save(reportDirectory + QStringLiteral("/166-resume.png"));
        const bool repeatEnter =
            hwnd && GetParent(hwnd) && SplitViewerTestNativeHelper::nativeDoubleClick(upper.center());
        QTest::qWait(400);
        window.leaveFullscreen();
        QTest::qWait(400);
        SetCursorPos(20, 900);
        QTest::qWait(1300);
        SplitViewerTestVideoHelper::screenPane(upper).save(reportDirectory + QStringLiteral("/166-final.png"));
        const bool finalPixels =
            SplitViewerTestVideoHelper::videoPixelsFill(SplitViewerTestVideoHelper::screenPane(upper));
        const bool finalPlaying = SplitViewerTestVideoHelper::videoIsAdvancing(upper);
        out << "166 details pauseInput=" << paused << " stopped=" << stoppedFrames << " resumeInput=" << resumed
            << " resumed=" << resumedFrames << " doubled=" << repeatEnter << " pixels=" << finalPixels
            << " playing=" << finalPlaying << "\n";
        check("real player single click and repeated fullscreen retain playback",
            stoppedFrames && resumedFrames && repeatEnter && hwnd && finalPixels && finalPlaying);

        const bool detached =
            hwnd && GetParent(hwnd) && SplitViewerTestNativeHelper::nativeDrag(upper.center(), QPoint(380, 650));
        for (int i = 0; player.state() == QProcess::Running && i < 120; ++i)
        {
            QTest::qWait(50);
        }
        check("real player drag out closes configured process",
            detached && player.state() == QProcess::NotRunning && !IsWindow(hwnd));

        player.start(playerPath, QStringList() << videoPath);
        const bool restarted = player.waitForStarted(5000);
        HWND secondWindow = nullptr;
        for (int i = 0; restarted && !secondWindow && i < 50; ++i)
        {
            secondWindow = CSystem::findVisibleWindowByProcessId(player.pid()->dwProcessId);
            QTest::qWait(100);
        }
        if (secondWindow)
        {
            SetWindowPos(secondWindow, HWND_TOPMOST, 30, 100, 640, 400, SWP_SHOWWINDOW);
            SetCursorPos(300, 102);
            QTest::qWait(650);
            if (GetAncestor(WindowFromPoint(sourcePoint), GA_ROOT) == secondWindow)
            {
                SplitViewerTestNativeHelper::nativeDrag(QPoint(300, 120), upper.center());
            }
        }
        const bool reembedded = secondWindow && GetParent(secondWindow);
        window.close();
        for (int i = 0; player.state() == QProcess::Running && i < 120; ++i)
        {
            QTest::qWait(50);
        }
        const bool playerClosed = player.state() == QProcess::NotRunning;
        if (!playerClosed)
        {
            player.kill();
            player.waitForFinished(2000);
            out << "player cleanup required termination\n";
        }
        check("real player viewer close closes only embedded player",
            reembedded && playerClosed && !IsWindow(secondWindow) && player.exitStatus() == QProcess::NormalExit &&
                player.exitCode() == 0);
        SetCursorPos(originalCursor.x, originalCursor.y);
    }
#endif
    if (selectedCase == 0 || (selectedCase >= 169 && selectedCase <= 171))
    {
        nextId = 168;
        SplitViewerCoreDocument document;
        document.baseRoot()->makeSplit(SPLITVIEWER_CORE_SPLIT_VERTICAL);
        document.baseRoot()->first->view.path = L"ownership-marker";
        SplitViewerCoreNode* original = document.baseRoot();
        document.setBaseRoot(original);
        check("reassign owned root preserves tree and content",
            document.baseRoot() == original && !document.baseRoot()->isLeaf() &&
                document.baseRoot()->first->view.path == L"ownership-marker");
        SplitViewerCoreLayer* layer = document.addLayer();
        layer->root->view.path = L"layer-marker";
        document.appendLayer(layer);
        check("append owned layer is idempotent and reset releases once",
            document.layerCount() == 1 && document.layerAt(0)->root->view.path == L"layer-marker");
        document.reset();
        Widget foreign;
        foreign.setGeometry(20, 20, 160, 100);
        foreign.show();
        const WId foreignId = foreign.winId();
        const QRect originalGeometry = foreign.geometry();
        check("embed rejects missing host without touching foreign window",
            SplitViewerEmbedForeignWindow(foreignId, nullptr) == nullptr && foreign.isVisible() &&
                foreign.geometry() == originalGeometry && foreign.parentWidget() == nullptr);
    }
    if (selectedCase == 0 || (selectedCase >= 178 && selectedCase <= 182))
    {
        nextId = 177;
        SplitViewerTestWindow window;
        QWidget* canvas = window.centralWidget();
        const QRectF stage = SplitViewerTestUiHelper::expectedStage(canvas);
        const QPoint baseCenter = stage.center().toPoint();
        SplitViewerTestUiHelper::drop(canvas, baseCenter, redPath);
        check("base layer delete action disabled",
            !SplitViewerTestUiHelper::chooseMenu(canvas, baseCenter, QStringLiteral("删除图层")) &&
                SplitViewerTestUiHelper::pixel(canvas, baseCenter, red));

        const bool created = SplitViewerTestUiHelper::toolbar(window, QStringLiteral("新建图层"));
        const QRectF layer(stage.left() + stage.width() * .22, stage.top() + stage.height() * .22, stage.width() * .46,
            stage.height() * .46);
        const QPoint layerCenter = layer.center().toPoint();
        window.m_nextFile = bluePath;
        SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, layerCenter);
        const bool split = SplitViewerTestUiHelper::chooseMenu(canvas, layerCenter, QStringLiteral("水平分割"));
        const QPoint lower(layerCenter.x(), qRound(layer.top() + layer.height() * .75));
        const bool deletedByMenu = SplitViewerTestUiHelper::chooseMenu(canvas, lower, QStringLiteral("删除图层"));
        check("context menu deletes complete floating layer",
            created && split && deletedByMenu && SplitViewerTestUiHelper::pixel(canvas, baseCenter, red) &&
                SplitViewerTestUiHelper::pixel(canvas, layerCenter, red));

        SplitViewerTestUiHelper::toolbar(window, QStringLiteral("新建图层"));
        window.m_nextFile = greenPath;
        SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, layerCenter);
        SplitViewerTestUiHelper::key(canvas, Qt::Key_Delete);
        check("Delete key removes focused floating layer", SplitViewerTestUiHelper::pixel(canvas, layerCenter, red));
        SplitViewerTestUiHelper::key(canvas, Qt::Key_Delete);
        check("Delete key leaves base layer intact", SplitViewerTestUiHelper::pixel(canvas, baseCenter, red));

        SplitViewerTestUiHelper::toolbar(window, QStringLiteral("新建图层"));
        const QPoint topLeft = QPoint(qRound(layer.left()) + 3, qRound(layer.top()) + 3);
        QMouseEvent hover(
            QEvent::MouseMove, topLeft, canvas->mapToGlobal(topLeft), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(canvas, &hover);
        QApplication::processEvents();
        const bool moveCursor = canvas->cursor().shape() == Qt::SizeAllCursor;
        SplitViewerTestUiHelper::drag(canvas, topLeft, topLeft + QPoint(24, 16));
        const QString movedProfile = reportDirectory + QStringLiteral("/layer-move.sv");
        window.m_nextFile = movedProfile;
        SplitViewerTestUiHelper::toolbar(window, QStringLiteral("保存配置"));
        SplitViewerCoreDocument moved;
        const bool movedSaved = SplitViewerTestUiHelper::readDocument(movedProfile, moved);
        check("floating layer upper-left uses move cursor and moves whole layer",
            moveCursor && movedSaved && moved.layerCount() == 1 && moved.layerAt(0)->rect.left > .22);
    }
    nextId = 171;
    for (int id = 172; id <= 177; ++id)
    {
        if (selectedCase == 0 || selectedCase == id)
        {
            nextId = id - 1;
            check(SplitViewerAuditCaseNames().at(id - 101).mid(4).toUtf8().constData(),
                SplitViewerDialogTests::runCase(id, reportDirectory));
        }
    }
    if (selectedCase == 0 || selectedCase == 183)
    {
        nextId = 182;
        check(SplitViewerAuditCaseNames().at(183 - 101).mid(4).toUtf8().constData(),
            SplitViewerDialogTests::runCase(183, reportDirectory));
    }
    for (int id = 184; id <= 188; ++id)
    {
        if (selectedCase == 0 || selectedCase == id)
        {
            nextId = id - 1;
            check(SplitViewerAuditCaseNames().at(id - 101).mid(4).toUtf8().constData(),
                SplitViewerShadowTests::runCase(id, reportDirectory));
        }
    }
    if (selectedCase == 0 || selectedCase == 189)
    {
        nextId = 188;
        SplitViewerTestWindow window;
        QWidget* canvas = window.centralWidget();
        SplitViewerTestUiHelper::toolbar(window, QStringLiteral("新建图层"));
        const QRectF stage = SplitViewerTestUiHelper::expectedStage(canvas);
        const QPoint corner(qCeil(stage.left() + stage.width() * .22), qCeil(stage.top() + stage.height() * .22));
        // Coordinates independently select the previously missed 7-to-8px strip.
        const QPoint target = corner + QPoint(7, 7);
        QMouseEvent hover(
            QEvent::MouseMove, target, canvas->mapToGlobal(target), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(canvas, &hover);
        const bool cursor = canvas->cursor().shape() == Qt::SizeAllCursor;
        SplitViewerTestUiHelper::drag(canvas, target, target + QPoint(24, 16));
        window.m_nextFile = reportDirectory + QStringLiteral("/edge-config.sv");
        SplitViewerTestUiHelper::toolbar(window, QStringLiteral("保存配置"));
        SplitViewerCoreDocument document;
        const bool loaded =
            SplitViewerTestUiHelper::readDocument(reportDirectory + QStringLiteral("/edge-config.sv"), document);
        const SplitViewerCoreLayer* layer = loaded ? document.layerAt(0) : nullptr;
        check(QStringLiteral("浮动层8像素热区光标与拖动一致").toUtf8().constData(),
            cursor && layer != nullptr && layer->rect.left > .22 && std::abs(layer->rect.width() - .46) < .0001 &&
                std::abs(layer->rect.height() - .46) < .0001);
    }
    if (selectedCase == 0 || selectedCase == 190)
    {
        nextId = 189;
        SplitViewerTestWindow window;
        QWidget* canvas = window.centralWidget();
        const QPoint center = SplitViewerTestUiHelper::expectedStage(canvas).center().toPoint();
        SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, center);
        const QString plusFilter = window.m_lastFilter;
        const QString loadText = plusFilter.contains(QStringLiteral("*.avi"))
            ? QStringLiteral("载入图片/视频到当前分屏")
            : QStringLiteral("载入图片到当前分屏");
        const bool selected = SplitViewerTestUiHelper::chooseMenu(canvas, center, loadText);
        check(QStringLiteral("加号与右键图片过滤器一致且支持TIFF").toUtf8().constData(),
            selected && window.m_browseCount == 2 && plusFilter == window.m_lastFilter &&
                plusFilter.contains(QStringLiteral("*.tif")) && plusFilter.contains(QStringLiteral("*.tiff")) &&
                window.m_errors.isEmpty());
    }
    for (int id = 191; id <= 201; ++id)
    {
        if (id != 200 && (selectedCase == 0 || selectedCase == id))
        {
            nextId = id - 1;
            check(SplitViewerAuditCaseNames().at(id - 101).mid(4).toUtf8().constData(),
                SplitViewerConfigTests::runCase(id, reportDirectory));
        }
    }
    if (selectedCase == 0 || selectedCase == 200)
    {
        nextId = 199;
        SplitViewerTestWindow window;
        window.canvasFileDropped(bluePath);
        SplitViewerTestUiHelper::settle(&window);
        window.newLayer();
        SplitViewerTestUiHelper::settle(&window);
        QWidget* canvas = window.centralWidget();
        canvas->setFocus();
        bool deleted = false;
        window.m_nextFile = redPath;
        /** 文件选择期间删除目标图层，验证返回后不会访问失效叶子
        */
        window.m_duringBrowse = [&window, &deleted]()
        {
            deleted = window.deleteSelectedLayer();
        };
        const QRectF stage = SplitViewerTestUiHelper::expectedStage(canvas);
        // 独立预期：新层从0.22开始、宽高0.46，加号位于舞台的0.45处。
        const QPoint plusPoint =
            QPointF(stage.left() + stage.width() * .45, stage.top() + stage.height() * .45).toPoint();
        SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, plusPoint);
        const QString saved = QDir(reportDirectory).filePath(QStringLiteral("stale-file-result.sv"));
        window.m_nextFile = saved;
        window.saveProfile();
        SplitViewerTestUiHelper::settle(&window);
        SplitViewerCoreDocument state;
        check(QStringLiteral("文件框返回时目标已删除则忽略旧结果").toUtf8().constData(),
            deleted && window.m_browseCount == 2 && window.m_errors.isEmpty() &&
                SplitViewerTestUiHelper::readDocument(saved, state) && state.layerCount() == 0 &&
                state.baseRoot()->view.hasImage && state.baseRoot()->view.path == bluePath.toStdWString());
    }
    if (selectedCase == 0 || selectedCase == 202)
    {
        nextId = 201;
        check(QStringLiteral("Test主界面异步批次与关闭收尾").toUtf8().constData(),
            SplitViewerHarnessTests::run(reportDirectory));
    }
    for (int id = 203; id <= 206; ++id)
    {
        if (selectedCase == 0 || selectedCase == id)
        {
            nextId = id - 1;
            check(QString::fromStdWString(
                      SplitViewerTestCaseHelper::kCases.at(static_cast<SplitViewerTestCaseId>(id)).m_name)
                      .toUtf8()
                      .constData(),
                SplitViewerMoveVideoTests::run(id, reportDirectory, out));
        }
    }
    for (int index = 207; index <= 220; ++index)
    {
        const int id = index >= 217 ? index + 8 : index;
        if (selectedCase == 0 || selectedCase == id)
        {
            nextId = id - 1;
            QString playerPath = QDir(QApplication::applicationDirPath())
                                     .absoluteFilePath(QStringLiteral("../common/LumaPlayer/LumaPlayer1.0.exe"));
            foreach (const QString& argument, QApplication::arguments())
            {
                if (argument.startsWith(QStringLiteral("--player=")))
                {
                    playerPath = argument.mid(9);
                }
            }
            const QString fixture = QDir(reportDirectory).absoluteFilePath(QStringLiteral("quadrants.avi"));
#ifdef Q_OS_WIN
            const bool ready = SplitViewerTestVideoHelper::writeVideoFixture(fixture);
#else
            const bool ready = true;
#endif
            check(QString::fromStdWString(
                      SplitViewerTestCaseHelper::kCases.at(static_cast<SplitViewerTestCaseId>(id)).m_name)
                      .toUtf8()
                      .constData(),
                ready && SplitViewerMoveUiTests::run(id, reportDirectory, fixture, playerPath, out));
        }
    }
    for (int32_t id = 217; id <= 224; ++id)
    {
        if (selectedCase == id || (selectedCase == 0 && id != 220))
        {
            nextId = id - 1;
            check("compliance", SplitViewerComplianceTests::run(id, reportDirectory, dialogs.get()));
        }
    }
    if (selectedCase >= 1 && selectedCase <= 7)
    {
        nextId = selectedCase - 1;
        check("core", SplitViewerCoreTests::run(selectedCase));
    }
    bool complete = total > 0 && (selectedCase == 0 || total == 1);
#ifdef Q_OS_WIN
    complete = complete &&
        (selectedCase != 0 ||
            total == static_cast<int>(SplitViewerTestCaseHelper::byType(SPLITVIEWER_TEST_INSTANT).size()) - 7);
#endif
    if (!complete)
    {
        ++failures;
        out << "ERROR missing expected cases\n";
    }
    out << "total=" << total << " failures=" << failures << "\n";
    dialogs.reset();
    DialogManager::setLogCallback(nullptr);
    return failures;
}

QStringList SplitViewerAuditCaseNames()
{
    QStringList names;
    for (const auto& item : SplitViewerTestCaseHelper::kCases)
    {
        names.append(QString::number(static_cast<int32_t>(item.first)) + QLatin1Char(' ') +
            QString::fromStdWString(item.second.m_name));
    }
    return names;
}