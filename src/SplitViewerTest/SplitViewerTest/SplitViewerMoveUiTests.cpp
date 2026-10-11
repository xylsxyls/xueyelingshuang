#include "SplitViewerTestUiHelper.h"
#include "SplitViewerTestWindow.h"
#include "SplitViewerMoveUiTests.h"
#include "SplitViewerUiTests.h"
#include "SplitViewerTestConfig.h"
#include "../../SplitViewer/SplitViewer/Config.h"
#include "CSystem/CSystemAPI.h"
#include "QtControls/Widget.h"
#include "DialogManager/DialogManagerAPI.h"
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtCore/QTextStream>
#include <QtCore/QVariant>
#include <QtCore/QProcess>
#include <QtCore/QBuffer>
#include <QtGui/QScreen>
#include <QtGui/QContextMenuEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtTest/QtTest>
#include <algorithm>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

QRectF SplitViewerMoveUiTests::stage(QWidget* canvas)
{
    const QRect desktop = QApplication::desktop()->screenGeometry(canvas);
    const double aspect = double(desktop.width()) / desktop.height();
    QSizeF size(canvas->width() - 20, canvas->height() - 20);
    if (size.width() / size.height() > aspect)
    {
        size.setWidth(size.height() * aspect);
    }
    else
    {
        size.setHeight(size.width() / aspect);
    }
    return QRectF(QPointF((canvas->width() - size.width()) / 2, (canvas->height() - size.height()) / 2), size)
        .adjusted(1, 1, -1, -1);
}

bool SplitViewerMoveUiTests::menu(
    QWidget* canvas, const QPoint& point, const QString& text, bool native, QTextStream* log)
{
    if (!SplitViewerTestUiHelper::settle(canvas))
    {
        return false;
    }
    bool clicked = false;
    int attempts = 0;
    QTimer driver;
    QObject::connect(&driver, &QTimer::timeout,
        /** 查找实际弹出菜单并点击指定可用项，重试和失败由外层记录
        */
        [&]()
        {
            QMenu* popup = qobject_cast<QMenu*>(QApplication::activePopupWidget());
            if (popup)
            {
                if (log)
                {
                    *log << "popup=" << popup->winId() << " target=" << text << "\n";
                }
                foreach (QAction* action, popup->actions())
                {
                    if (log)
                    {
                        *log << " action=" << action->text() << " enabled=" << action->isEnabled() << "\n";
                    }
                    if (action->text() == text && action->isEnabled())
                    {
                        clicked = true;
                        driver.stop();
                        QTest::mouseClick(
                            popup, Qt::LeftButton, Qt::NoModifier, popup->actionGeometry(action).center());
                        return;
                    }
                }
                driver.stop();
                popup->close();
            }
            else if (++attempts > 100)
            {
                driver.stop();
            }
        });
    driver.start(10);
#ifdef Q_OS_WIN
    if (native)
    {
        const QPoint global = canvas->mapToGlobal(point);
        const HWND host = reinterpret_cast<HWND>(canvas->window()->winId());
        SetWindowPos(host, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        SetForegroundWindow(host);
        const POINT position = {global.x(), global.y()};
        const HWND target = WindowFromPoint(position);
        // 实际系统输入会激活命中的窗口。前台锁不等于目标错误；
        // 必须核对命中宿主身份，不能把输入发给遮在上面的其他应用。
        if (GetAncestor(target, GA_ROOT) != host)
        {
            driver.stop();
            if (log)
            {
                *log << "native menu target is outside owned host; target=" << reinterpret_cast<quintptr>(target)
                     << " host=" << reinterpret_cast<quintptr>(host) << "\n";
                log->flush();
            }
            return false;
        }
        if (log)
        {
            *log << "native menu point=" << global.x() << "," << global.y()
                 << " target=" << reinterpret_cast<quintptr>(target)
                 << " root=" << reinterpret_cast<quintptr>(GetAncestor(target, GA_ROOT))
                 << " host=" << canvas->window()->winId()
                 << " foreground=" << reinterpret_cast<quintptr>(GetForegroundWindow()) << "\n";
            log->flush();
        }
        // 将定位和按键放入同一输入序列，避免SetCursorPos与另一个进程的激活消息交错。
        const int virtualLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
        const int virtualTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
        const int virtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        const int virtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        if (virtualWidth <= 1 || virtualHeight <= 1)
        {
            driver.stop();
            return false;
        }
        INPUT inputs[3] = {};
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
        inputs[0].mi.dx = MulDiv(global.x() - virtualLeft, 65535, virtualWidth - 1);
        inputs[0].mi.dy = MulDiv(global.y() - virtualTop, 65535, virtualHeight - 1);
        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        inputs[2].type = INPUT_MOUSE;
        inputs[2].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        const UINT sent = SendInput(3, inputs, sizeof(INPUT));
        if (log)
        {
            *log << "SendInput=" << sent << " error=" << GetLastError() << "\n";
            log->flush();
        }
        if (sent != 3)
        {
            driver.stop();
            return false;
        }
        while (driver.isActive())
        {
            QTest::qWait(10);
        }
    }
    else
#else
    Q_UNUSED(native);
#endif
    {
        QContextMenuEvent event(QContextMenuEvent::Mouse, point, canvas->mapToGlobal(point));
        QApplication::sendEvent(canvas, &event);
    }
    driver.stop();
    QApplication::processEvents();
    if (log)
    {
        *log << "menu clicked=" << clicked << " attempts=" << attempts << "\n";
        log->flush();
    }
    return SplitViewerTestUiHelper::settle(canvas) && clicked;
}

bool SplitViewerMoveUiTests::snapshot(
    SplitViewerTestWindow& window, const QString& path, SplitViewerCoreDocument& document)
{
    if (!SplitViewerTestUiHelper::settle(&window))
    {
        return false;
    }
    window.m_nextFile = path;
    window.saveProfile();
    SplitViewerTestUiHelper::settle(&window);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }
    const QByteArray bytes = file.readAll();
    const std::vector<uint8_t> package(bytes.begin(), bytes.end());
    std::vector<uint8_t> config;
    return SplitViewerCoreExtractEmbeddedConfig(package, config) && SplitViewerCoreDeserializeProfile(config, document);
}

bool SplitViewerMoveUiTests::waitEmbedded(SplitViewerTestWindow& window, int expected)
{
    for (int attempt = 0; attempt < 120; ++attempt)
    {
        int count = 0;
        foreach (QWidget* widget, window.centralWidget()->findChildren<QWidget*>())
        {
            if (widget->property("foreignId").isValid())
            {
                ++count;
            }
        }
        if (count == expected)
        {
            return true;
        }
        QTest::qWait(50);
    }
    return false;
}

bool SplitViewerMoveUiTests::videoPixels(QWidget* canvas, const QRectF& rect, const QString& output)
{
    QTest::qWait(350);
    const QPoint global = canvas->mapToGlobal(rect.topLeft().toPoint());
    const QImage image = QGuiApplication::primaryScreen()
                             ->grabWindow(0, global.x(), global.y(), qRound(rect.width()), qRound(rect.height()))
                             .toImage();
    image.save(output);
    if (image.isNull())
    {
        return false;
    }
    const double scale = (std::min)(image.width() / 160.0, image.height() / 96.0);
    const double left = (image.width() - 160 * scale) / 2, top = (image.height() - 96 * scale) / 2;
    for (int quadrant = 0; quadrant < 4; ++quadrant)
    {
        const QColor color(image.pixel(
            qRound(left + (quadrant % 2 ? 136 : 24) * scale), qRound(top + (quadrant / 2 ? 72 : 24) * scale)));
        const bool red = color.red() > 60 && color.green() < 60 && color.blue() < 60;
        const bool green = color.red() < 60 && color.green() > 60 && color.blue() < 60;
        const bool blue = color.red() < 60 && color.green() < 60 && color.blue() > 60;
        const bool yellow = color.red() > 60 && color.green() > 60 && color.blue() < 60;
        if (!(quadrant == 0 ? red : quadrant == 1 ? green : quadrant == 2 ? blue : yellow))
        {
            return false;
        }
    }
    return true;
}

bool SplitViewerMoveUiTests::videoAdvances(QWidget* canvas, const QRectF& rect)
{
    const QPoint global = canvas->mapToGlobal(rect.topLeft().toPoint());
    int firstFrame = -1;
    for (int attempt = 0; attempt < 20; ++attempt)
    {
        QTest::qWait(100);
        const QImage image = QGuiApplication::primaryScreen()
                                 ->grabWindow(0, global.x(), global.y(), qRound(rect.width()), qRound(rect.height()))
                                 .toImage();
        if (image.isNull())
        {
            return false;
        }
        const double scale = (std::min)(image.width() / 160.0, image.height() / 96.0);
        const double left = (image.width() - 160 * scale) / 2, top = (image.height() - 96 * scale) / 2;
        int frame = 0;
        for (int bit = 0; bit < 9; ++bit)
        {
            const QColor color(image.pixel(qRound(left + (13 + 6 * bit) * scale), qRound(top + 32 * scale)));
            if (color.red() > 160)
            {
                frame |= 1 << bit;
            }
        }
        if (firstFrame < 0)
        {
            firstFrame = frame;
        }
        else if (frame != firstFrame)
        {
            return true;
        }
    }
    return false;
}

bool SplitViewerMoveUiTests::displacedWindowReturns(QWidget* container, quintptr windowId, QTextStream& log)
{
#ifdef Q_OS_WIN
    const HWND window = reinterpret_cast<HWND>(windowId);
    const HWND parent = container ? reinterpret_cast<HWND>(container->winId()) : nullptr;
    if (!IsWindow(window) || !IsWindow(parent) || GetParent(window) != parent ||
        !SetWindowPos(window, nullptr, 123, 87, 198, 126, SWP_NOZORDER | SWP_NOACTIVATE))
    {
        return false;
    }
    RECT displaced = {};
    GetWindowRect(window, &displaced);
    MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&displaced), 2);
    log << "225 displaced=" << displaced.left << "," << displaced.top << "," << displaced.right - displaced.left << ","
        << displaced.bottom - displaced.top << "\n";
    log.flush();
    for (int attempt = 0; attempt < 50; ++attempt)
    {
        QTest::qWait(20);
        RECT client = {}, current = {};
        if (!IsWindow(window) || GetParent(window) != parent || !GetClientRect(parent, &client) ||
            !GetWindowRect(window, &current))
        {
            return false;
        }
        MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&current), 2);
        if (current.left == 0 && current.top == 0 && current.right == client.right && current.bottom == client.bottom)
        {
            log << "225 restored=" << current.left << "," << current.top << "," << current.right << ","
                << current.bottom << " wait=" << (attempt + 1) * 20 << "ms\n";
            log.flush();
            return true;
        }
    }
    log << "225 did not recover while idle\n";
    log.flush();
    return false;
#else
    Q_UNUSED(container);
    Q_UNUSED(windowId);
    Q_UNUSED(log);
    return false;
#endif
}

int SplitViewerMoveUiTests::runLifetimeProbe()
{
    QApplication::setQuitOnLastWindowClosed(false);
    Widget* probe = new Widget;
    probe->setAttribute(Qt::WA_DeleteOnClose);
    probe->setWindowTitle(QStringLiteral("SplitViewer lifetime probe"));
    probe->resize(320, 180);
    probe->show();
    return QApplication::exec();
}

int SplitViewerMoveUiTests::runStartupProbe(const QString& path)
{
    QFile marker(path + QStringLiteral(".pid"));
    if (!marker.open(QIODevice::WriteOnly))
    {
        return 2;
    }
    marker.write(QByteArray::number(QCoreApplication::applicationPid()));
    marker.close();
    QTimer::singleShot(20000, qApp, SLOT(quit()));
    return qApp->exec();
}

bool SplitViewerMoveUiTests::run(
    int id, const QString& directory, const QString& videoPath, const QString& playerPath, QTextStream& log)
{
    const bool hadEnvironment = qEnvironmentVariableIsSet("LUMAPLAYER");
    const QByteArray previousEnvironment = qgetenv("LUMAPLAYER");
    qputenv("LUMAPLAYER", playerPath.toLocal8Bit());
    const QString redPath = QDir(directory).filePath(QStringLiteral("move-red.png"));
    const QString bluePath = QDir(directory).filePath(QStringLiteral("move-blue.png"));
    QImage image(160, 96, QImage::Format_RGB32);
    image.fill(QColor(230, 30, 40));
    image.save(redPath);
    image.fill(QColor(30, 70, 230));
    image.save(bluePath);
    SplitViewerTestWindow window;
    window.setGeometry(720, 160, 900, 640);
#ifdef Q_OS_WIN
    SetWindowPos(
        reinterpret_cast<HWND>(window.winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
#endif
    QTest::qWait(100);
    QWidget* canvas = window.centralWidget();
    const QRectF area = stage(canvas);
    const QPoint left(qRound(area.left() + area.width() * .25), qRound(area.center().y()));
    const QPoint right(qRound(area.left() + area.width() * .75), qRound(area.center().y()));
    const QPoint top(qRound(area.center().x()), qRound(area.top() + area.height() * .25));
    const QPoint bottom(qRound(area.center().x()), qRound(area.top() + area.height() * .75));
    const QString statePath = QDir(directory).filePath(QStringLiteral("move-state.sv"));
    bool ok = true;
    if (id == 207)
    {
        window.canvasFileDropped(redPath);
        SplitViewerTestUiHelper::settle(&window);
        ok = menu(canvas, area.center().toPoint(), QStringLiteral("垂直分割"));
        ok = menu(canvas, left, QStringLiteral("移动到右边")) && ok;
        SplitViewerCoreDocument state;
        ok = snapshot(window, statePath, state) && !state.baseRoot()->isLeaf() &&
            !state.baseRoot()->first->view.hasContent() &&
            state.baseRoot()->second->view.path == redPath.toStdWString() && ok;
        window.m_nextFile.clear();
        SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, left);
        ok = window.m_browseCount == 2 && ok;
        ok = menu(canvas, right, QStringLiteral("移动到左边")) && ok;
        ok = menu(canvas, right, QStringLiteral("删除当前分屏")) && ok;
        ok = menu(canvas, area.center().toPoint(), QStringLiteral("水平分割")) && ok;
        ok = menu(canvas, top, QStringLiteral("移动到下面")) && ok;
        ok = menu(canvas, bottom, QStringLiteral("移动到上面")) && ok;
        SplitViewerCoreDocument finalState;
        ok = snapshot(window, statePath, finalState) && !finalState.baseRoot()->isLeaf() &&
            finalState.baseRoot()->first->view.path == redPath.toStdWString() &&
            !finalState.baseRoot()->second->view.hasContent() && ok;
        ok = QColor(canvas->grab().toImage().pixel(top)) == QColor(230, 30, 40) && ok;
        canvas->grab().save(QDir(directory).filePath(QStringLiteral("207-moved.png")));
    }
    else if (id == 208)
    {
        window.canvasFileDropped(redPath);
        SplitViewerTestUiHelper::settle(&window);
        menu(canvas, area.center().toPoint(), QStringLiteral("垂直分割"));
        window.canvasFileDroppedAt(bluePath, right);
        SplitViewerTestUiHelper::settle(&window);
        bool accept = false, observed = false;
        QTimer confirm;
        QObject::connect(&confirm, &QTimer::timeout,
            /** 观察覆盖分屏确认框并按当前反例选择真实确认或取消按钮
            */
            [&]()
            {
                QWidget* modal = QApplication::activeModalWidget();
                if (!modal)
                {
                    return;
                }
                observed = modal->windowTitle() == QStringLiteral("覆盖分屏内容");
                foreach (QPushButton* button, modal->findChildren<QPushButton*>())
                {
                    if (button->isVisible() &&
                        button->text() == (accept ? QStringLiteral("确认") : QStringLiteral("取消")))
                    {
                        QTest::mouseClick(button, Qt::LeftButton);
                        return;
                    }
                }
            });
        QTimer watchdog;
        watchdog.setSingleShot(true);
        QObject::connect(&watchdog, &QTimer::timeout,
            /** 覆盖确认场景超时后记录失败，并通过DialogManager关闭托管窗口
            */
            [&]()
            {
                ok = false;
                DestroyAllOperateParam close;
                DialogManager::instance().operateDialog(close);
            });
        watchdog.start(5000);
        confirm.start(10);
        ok = menu(canvas, left, QStringLiteral("移动到右边")) && ok;
        SplitViewerCoreDocument before;
        ok = snapshot(window, statePath, before) && !before.baseRoot()->isLeaf() &&
            before.baseRoot()->first->view.path == redPath.toStdWString() &&
            before.baseRoot()->second->view.path == bluePath.toStdWString() && observed && ok;
        accept = true;
        observed = false;
        ok = menu(canvas, left, QStringLiteral("移动到右边")) && ok;
        confirm.stop();
        watchdog.stop();
        SplitViewerCoreDocument after;
        ok = snapshot(window, statePath, after) && !after.baseRoot()->isLeaf() &&
            !after.baseRoot()->first->view.hasContent() &&
            after.baseRoot()->second->view.path == redPath.toStdWString() && observed && ok;
    }
    else if (id == 209)
    {
        window.canvasFileDropped(bluePath);
        SplitViewerTestUiHelper::settle(&window);
        window.newLayer();
        SplitViewerTestUiHelper::settle(&window);
        window.canvasFileDroppedAt(redPath, area.center().toPoint());
        SplitViewerTestUiHelper::settle(&window);
        ok = !menu(canvas, area.center().toPoint(), QStringLiteral("移动到右边"));
        menu(canvas, area.center().toPoint(), QStringLiteral("垂直分割"));
        // Floating layers initially occupy the central half of the stage.
        const QPoint layerLeft(qRound(area.center().x() - area.width() * .125), qRound(area.center().y()));
        const QPoint layerRight(qRound(area.center().x() + area.width() * .125), qRound(area.center().y()));
        ok = menu(canvas, layerLeft, QStringLiteral("移动到右边")) && ok;
        SplitViewerCoreDocument state;
        ok = snapshot(window, statePath, state) && state.layerCount() == 1 &&
            state.baseRoot()->view.path == bluePath.toStdWString() && !state.layerAt(0)->root->isLeaf() &&
            !state.layerAt(0)->root->first->view.hasContent() &&
            state.layerAt(0)->root->second->view.path == redPath.toStdWString() && ok;
        ok = !menu(canvas, layerRight, QStringLiteral("移动到下面")) && ok;
    }
    else if (id == 210)
    {
        qunsetenv("LUMAPLAYER");
        SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, area.center().toPoint());
        ok = window.m_lastFilter.contains(QStringLiteral("*.jfif")) &&
            !window.m_lastFilter.contains(QStringLiteral("*.mp4"));
        qputenv("LUMAPLAYER", QDir(directory).filePath(QStringLiteral("missing.exe")).toLocal8Bit());
        SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, area.center().toPoint());
        ok = !window.m_lastFilter.contains(QStringLiteral("*.mp4")) && ok;
        qputenv("LUMAPLAYER", playerPath.toLocal8Bit());
        SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, area.center().toPoint());
        ok = window.m_lastFilter.contains(QStringLiteral("*.mp4")) &&
            window.m_lastFilter.contains(QStringLiteral("*.jfif")) && ok;
        window.canvasFileDropped(redPath);
        SplitViewerTestUiHelper::settle(&window);
        window.canvasFileDropped(QDir(directory).filePath(QStringLiteral("missing.avi")));
        SplitViewerTestUiHelper::settle(&window);
        SplitViewerCoreDocument state;
        ok = !window.m_errors.isEmpty() && snapshot(window, statePath, state) &&
            state.baseRoot()->view.path == redPath.toStdWString() && ok;
    }
    else if (id == 211 || id == 212 || id == 213 || (id >= 225 && id <= 228))
    {
#ifdef Q_OS_WIN
        const std::set<std::wstring> previousCloseNames = g_config.m_closeEmbeddedProcessNames;
        const int previousCloseTimeout = g_config.m_videoCloseTimeoutMs;
        if (id == 227 || id == 228)
        {
            const QString program = QApplication::applicationFilePath();
            const QString probe = QDir(directory).filePath(g_testConfig.m_lifetimeProbeFileName);
            QFile file(probe);
            ok = file.open(QIODevice::WriteOnly) && ok;
            file.write("lifetime probe");
            file.close();
            g_config.m_closeEmbeddedProcessNames.insert(QFileInfo(program).fileName().toLower().toStdWString());
            g_config.m_videoCloseTimeoutMs = 300;
            qputenv("LUMAPLAYER", program.toLocal8Bit());
            window.m_nextFile = probe;
        }
        else
        {
            window.m_nextFile = videoPath;
        }
        SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, area.center().toPoint());
        ok = waitEmbedded(window, 1) && window.m_errors.isEmpty();
        QWidget* embedded = nullptr;
        foreach (QWidget* widget, canvas->findChildren<QWidget*>())
        {
            if (widget->property("foreignId").isValid())
            {
                embedded = widget;
            }
        }
        HWND hwnd = embedded ? reinterpret_cast<HWND>(embedded->property("foreignId").toULongLong()) : nullptr;
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        HANDLE identity = pid ? OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pid) : nullptr;
        log << "linked player pid=" << pid << " window=" << reinterpret_cast<quintptr>(hwnd) << "\n";
        log.flush();
        if (id >= 226 && id <= 228)
        {
            ok = identity && IsWindow(hwnd) && ok;
            const bool requested = PostMessageW(hwnd, WM_CLOSE, 0, 0) != FALSE;
            bool destroyedWhileRunning = false;
            for (int i = 0; identity && i < 120; ++i)
            {
                QTest::qWait(10);
                const bool exited = WaitForSingleObject(identity, 0) == WAIT_OBJECT_0;
                if (!IsWindow(hwnd) && !exited)
                {
                    destroyedWhileRunning = true;
                    if (id == 228)
                    {
                        window.close();
                    }
                    break;
                }
                if (exited)
                {
                    break;
                }
            }
            for (int i = 0; identity && WaitForSingleObject(identity, 0) == WAIT_TIMEOUT && i < 120; ++i)
            {
                QTest::qWait(25);
            }
            const bool exited = identity && WaitForSingleObject(identity, 0) == WAIT_OBJECT_0;
            log << "lifetime id=" << id << " closeRequested=" << requested
                << " destroyedWhileRunning=" << destroyedWhileRunning << " processExited=" << exited
                << " hostVisible=" << window.isVisible() << "\n";
            ok = requested && exited && (id == 226 || destroyedWhileRunning) && ok;
            if (id != 228)
            {
                ok = window.isVisible() && ok;
                window.canvasFileDropped(redPath);
                SplitViewerTestUiHelper::settle(&window);
                ok = QColor(canvas->grab().toImage().pixel(area.center().toPoint())) == QColor(230, 30, 40) && ok;
            }
            if (identity && !exited)
            {
                // 只清理本用例创建并已持有身份的失败子进程，不能把清理计作产品通过。
                log << "owned failed fixture cleanup; original assertion stays failed\n";
                TerminateProcess(identity, 1);
            }
        }
        else if (id == 211)
        {
            const bool loaded = videoPixels(canvas, area, QDir(directory).filePath(QStringLiteral("211-loaded.png")));
            const bool split = menu(canvas, area.center().toPoint(), QStringLiteral("垂直分割"), true, &log);
            const bool moved = menu(canvas, left, QStringLiteral("移动到右边"), true, &log);
            ok = loaded && split && moved && ok;
            QRectF rightRect(area.left() + (area.width() + 2) / 2, area.top(), (area.width() - 2) / 2, area.height());
            const bool movedPixels =
                videoPixels(canvas, rightRect, QDir(directory).filePath(QStringLiteral("211-moved.png")));
            const bool advances = videoAdvances(canvas, rightRect);
            ok = movedPixels && advances && ok;
            DWORD after = 0;
            GetWindowThreadProcessId(hwnd, &after);
            ok = after == pid && GetParent(hwnd) && ok;
            SplitViewerCoreDocument state;
            ok = snapshot(window, statePath, state) && !state.baseRoot()->isLeaf() &&
                !state.baseRoot()->first->view.hasContent() &&
                state.baseRoot()->second->view.contentKind == SPLITVIEWER_CORE_CONTENT_VIDEO &&
                state.baseRoot()->second->view.path == videoPath.toStdWString() && ok;
            window.toggleFullscreen();
            QTest::qWait(300);
            const QRectF fullRight(canvas->width() / 2.0 + 1, 1, canvas->width() / 2.0 - 2, canvas->height() - 2);
            const bool fullscreenPixels = IsWindow(hwnd) && GetParent(hwnd) &&
                videoPixels(canvas, fullRight, QDir(directory).filePath(QStringLiteral("211-fullscreen.png")));
            log << "211 loaded=" << loaded << " split=" << split << " moved=" << moved << " movedPixels=" << movedPixels
                << " advances=" << advances << " fullscreenPixels=" << fullscreenPixels << "\n";
            log.flush();
            ok = fullscreenPixels && ok;
            window.leaveFullscreen();
        }
        else if (id == 225)
        {
            ok = displacedWindowReturns(embedded, reinterpret_cast<quintptr>(hwnd), log) && ok;
            ok = videoPixels(canvas, area, QDir(directory).filePath(QStringLiteral("225-idle.png"))) &&
                videoAdvances(canvas, area) && ok;
            ok = menu(canvas, area.center().toPoint(), QStringLiteral("垂直分割"), true, &log) && ok;
            ok = menu(canvas, left, QStringLiteral("移动到右边"), true, &log) && ok;
            const QRectF rightRect(
                area.left() + (area.width() + 2) / 2, area.top(), (area.width() - 2) / 2, area.height());
            ok = displacedWindowReturns(embedded, reinterpret_cast<quintptr>(hwnd), log) && ok;
            ok = videoPixels(canvas, rightRect, QDir(directory).filePath(QStringLiteral("225-moved-idle.png"))) &&
                videoAdvances(canvas, rightRect) && ok;
            window.toggleFullscreen();
            QTest::qWait(300);
            ok = displacedWindowReturns(embedded, reinterpret_cast<quintptr>(hwnd), log) && ok;
            const QRectF fullRight(canvas->width() / 2.0 + 1, 1, canvas->width() / 2.0 - 2, canvas->height() - 2);
            ok = videoPixels(canvas, fullRight, QDir(directory).filePath(QStringLiteral("225-fullscreen-idle.png"))) &&
                videoAdvances(canvas, fullRight) && ok;
            window.leaveFullscreen();
        }
        else if (id == 212)
        {
            ok = videoPixels(canvas, area, QDir(directory).filePath(QStringLiteral("212-before-menu.png"))) && ok;
            const bool split = menu(canvas, area.center().toPoint(), QStringLiteral("垂直分割"), true, &log);
            const bool moved = menu(canvas, left, QStringLiteral("移动到右边"), true, &log);
            ok = split && moved && ok;
            log << "212 split=" << split << " moved=" << moved << "\n";
            log.flush();
            window.canvasFileDroppedAt(redPath, left);
            SplitViewerTestUiHelper::settle(&window);
            SplitViewerCoreDocument saved;
            ok = snapshot(window, statePath, saved) && !saved.baseRoot()->isLeaf() &&
                saved.baseRoot()->second->view.path == videoPath.toStdWString() && ok;
            window.m_nextFile = statePath;
            window.openProfile();
            SplitViewerTestUiHelper::settle(&window);
            ok = waitEmbedded(window, 1) && window.m_errors.isEmpty() && ok;
            HWND restored = nullptr;
            foreach (QWidget* widget, canvas->findChildren<QWidget*>())
            {
                if (widget->property("foreignId").isValid())
                {
                    restored = reinterpret_cast<HWND>(widget->property("foreignId").toULongLong());
                }
            }
            DWORD restoredPid = 0;
            GetWindowThreadProcessId(restored, &restoredPid);
            for (int i = 0; identity && WaitForSingleObject(identity, 0) == WAIT_TIMEOUT && i < 120; ++i)
            {
                QTest::qWait(50);
            }
            ok = restoredPid != pid && restoredPid != 0 && identity &&
                WaitForSingleObject(identity, 0) == WAIT_OBJECT_0 && ok;
            log << "restored player pid=" << restoredPid << "\n";
            const QRectF rightRect(
                area.left() + (area.width() + 2) / 2, area.top(), (area.width() - 2) / 2, area.height());
            ok = videoPixels(canvas, rightRect, QDir(directory).filePath(QStringLiteral("212-restored.png"))) && ok;
            window.close();
            for (int i = 0; window.isVisible() && i < 120; ++i)
            {
                QTest::qWait(50);
            }
            SplitViewerTestWindow fresh;
            fresh.m_nextFile = statePath;
            fresh.openProfile();
            SplitViewerTestUiHelper::settle(&fresh);
            ok = waitEmbedded(fresh, 1) && fresh.m_errors.isEmpty() && ok;
            QWidget* freshCanvas = fresh.centralWidget();
            const QRectF freshStage = stage(freshCanvas);
            SetWindowPos(reinterpret_cast<HWND>(fresh.winId()), HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            const QRectF freshRight(freshStage.left() + (freshStage.width() + 2) / 2, freshStage.top(),
                (freshStage.width() - 2) / 2, freshStage.height());
            ok = videoPixels(freshCanvas, freshRight, QDir(directory).filePath(QStringLiteral("212-new-host.png"))) &&
                videoAdvances(freshCanvas, freshRight) && ok;
            fresh.close();
            for (int i = 0; fresh.isVisible() && i < 120; ++i)
            {
                QTest::qWait(50);
            }
            ok = !fresh.isVisible() && ok;
        }
        else
        {
            QProcess independent;
            independent.start(playerPath, QStringList() << videoPath);
            ok = independent.waitForStarted(5000) && ok;
            window.detachExternalWindow();
            for (int i = 0; identity && WaitForSingleObject(identity, 0) == WAIT_TIMEOUT && i < 120; ++i)
            {
                QTest::qWait(50);
            }
            ok = identity && WaitForSingleObject(identity, 0) == WAIT_OBJECT_0 &&
                independent.state() == QProcess::Running && ok;
            const DWORD otherPid = independent.pid() ? independent.pid()->dwProcessId : 0;
            HWND other = nullptr;
            for (int i = 0; !other && independent.state() == QProcess::Running && i < 50; ++i)
            {
                other = CSystem::findVisibleWindowByProcessId(otherPid);
                QTest::qWait(100);
            }
            ok = other && IsWindow(other) && otherPid != pid && ok;
            if (other)
            {
                PostMessageW(other, WM_CLOSE, 0, 0);
            }
            if (!independent.waitForFinished(5000))
            {
                independent.kill();
                independent.waitForFinished(2000);
                ok = false;
            }
            log << "unrelated player pid=" << otherPid
                << " preserved=" << (WaitForSingleObject(identity, 0) == WAIT_OBJECT_0) << "\n";
        }
        window.close();
        for (int i = 0; window.isVisible() && i < 120; ++i)
        {
            QTest::qWait(50);
        }
        ok = !window.isVisible() && ok;
        if (identity)
        {
            CloseHandle(identity);
        }
        g_config.m_closeEmbeddedProcessNames = previousCloseNames;
        g_config.m_videoCloseTimeoutMs = previousCloseTimeout;
#else
        ok = false;
#endif
    }
    else if (id == 214 || id == 215)
    {
        const QString probe = QDir(directory).filePath(QStringLiteral("startup-probe.avi"));
        QFile file(probe);
        file.open(QIODevice::WriteOnly);
        file.write("probe");
        file.close();
        const int oldStart = g_config.m_videoStartTimeoutMs, oldClose = g_config.m_videoCloseTimeoutMs;
        g_config.m_videoStartTimeoutMs = 1000;
        g_config.m_videoCloseTimeoutMs = 100;
        qputenv("LUMAPLAYER", QApplication::applicationFilePath().toLocal8Bit());
        if (id == 214)
        {
            window.newLayer();
            SplitViewerTestUiHelper::settle(&window);
            window.m_nextFile = probe;
            SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, area.center().toPoint());
            ok = window.deleteSelectedLayer();
            QTest::qWait(200);
            ok = !QFile::exists(probe + QStringLiteral(".pid")) && ok;
            window.canvasFileDropped(probe);
            SplitViewerTestUiHelper::settle(&window);
            QTest::qWait(250);
            window.canvasFileDropped(redPath);
            SplitViewerTestUiHelper::settle(&window);
        }
        else
        {
            window.canvasFileDropped(redPath);
            SplitViewerTestUiHelper::settle(&window);
            window.canvasFileDropped(probe);
            SplitViewerTestUiHelper::settle(&window);
            QTest::qWait(1400);
            ok = !window.m_errors.isEmpty();
        }
        QFile marker(probe + QStringLiteral(".pid"));
        ok = marker.open(QIODevice::ReadOnly) && ok;
        const qint64 pid = marker.readAll().toLongLong();
#ifdef Q_OS_WIN
        HANDLE identity = pid ? OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid)) : nullptr;
        for (int i = 0; identity && WaitForSingleObject(identity, 0) == WAIT_TIMEOUT && i < 120; ++i)
        {
            QTest::qWait(25);
        }
        ok = pid > 0 && (!identity || WaitForSingleObject(identity, 0) == WAIT_OBJECT_0) && ok;
        if (identity)
        {
            CloseHandle(identity);
        }
#endif
        SplitViewerCoreDocument state;
        ok = snapshot(window, statePath, state) && state.baseRoot()->view.path == redPath.toStdWString() && ok;
        log << "startup probe pid=" << pid << " canceled/timeout=" << id << "\n";
        g_config.m_videoStartTimeoutMs = oldStart;
        g_config.m_videoCloseTimeoutMs = oldClose;
    }
    else if (id == 216)
    {
        SplitViewerCoreDocument document;
        document.baseRoot()->view.contentKind = SPLITVIEWER_CORE_CONTENT_VIDEO;
        document.baseRoot()->view.path = QDir(directory).filePath(QStringLiteral("missing-video.avi")).toStdWString();
        std::vector<uint8_t> bytes;
        ok = SplitViewerCoreSerializeProfile(document, bytes);
        QByteArray thumbnail;
        QBuffer buffer(&thumbnail);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        buffer.close();
        std::vector<uint8_t> package;
        ok = SplitViewerCoreBuildConfigPackage(
                 std::vector<uint8_t>(thumbnail.begin(), thumbnail.end()), bytes, package) &&
            ok;
        QFile file(statePath);
        ok = file.open(QIODevice::WriteOnly) && ok;
        file.write(reinterpret_cast<const char*>(package.data()), package.size());
        file.close();
        qunsetenv("LUMAPLAYER");
        window.m_nextFile = statePath;
        window.openProfile();
        SplitViewerTestUiHelper::settle(&window);
        ok = !window.m_errors.isEmpty() && ok;
        SplitViewerCoreDocument retained;
        ok = snapshot(window, statePath, retained) &&
            retained.baseRoot()->view.contentKind == SPLITVIEWER_CORE_CONTENT_VIDEO &&
            retained.baseRoot()->view.path == document.baseRoot()->view.path && ok;
        window.m_nextFile = redPath;
        SplitViewerTestUiHelper::click(canvas, Qt::LeftButton, Qt::NoModifier, stage(canvas).center().toPoint());
        SplitViewerCoreDocument replacement;
        ok = snapshot(window, statePath, replacement) && replacement.baseRoot()->view.path == redPath.toStdWString() &&
            replacement.baseRoot()->view.hasImage && ok;
    }
    window.close();
    for (int i = 0; window.isVisible() && i < 120; ++i)
    {
        QTest::qWait(50);
    }
    ok = !window.isVisible() && ok;
    if (hadEnvironment)
    {
        qputenv("LUMAPLAYER", previousEnvironment);
    }
    else
    {
        qunsetenv("LUMAPLAYER");
    }
    log << "move UI case=" << id << " success=" << ok << " errors=" << window.m_errors.join(QStringLiteral(" | "))
        << "\n";
    return ok;
}