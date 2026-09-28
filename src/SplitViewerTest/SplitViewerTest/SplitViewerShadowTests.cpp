#include "SplitViewerShadowTests.h"
#include "../../SplitViewer/SplitViewer/SplitViewer.h"
#include "DialogManager/DialogManagerAPI.h"
#include "QtControls/DialogBase.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QPointer>
#include <QProcess>
#include <QScreen>
#include <QTextStream>
#include <QTimer>
#include <QtTest/QtTest>
#include <climits>
#ifdef Q_OS_WIN
#include <Windows.h>
#endif

bool SplitViewerShadowTests::runCase(int id, const QString& directory)
{
    if (id == 184)
    {
        return checkParameters(directory);
    }
    if (id == 187)
    {
        return checkLifetime(directory);
    }
    return checkNativeDrag(directory, id == 186, id == 188);
}

bool SplitViewerShadowTests::checkParameters(const QString& directory)
{
    DialogBase dialog;
    dialog.setGeometry(350, 280, 520, 340);
    dialog.setCustomerTitleBarHeight(42);
    dialog.show();
    QTest::qWait(100);
    const QRect original = dialog.geometry();
    dialog.setWindowShadow(false, 2);
    bool valid = !dialog.windowShadowEnabled() && dialog.windowShadowSize() == 0;
    dialog.setWindowShadow(true, 0);
    valid = valid && !dialog.windowShadowEnabled();
    dialog.setWindowShadow(true, -1);
    valid = valid && !dialog.windowShadowEnabled() && dialog.geometry() == original;
    dialog.setWindowShadow(true, 2);
    QTest::qWait(100);
    QWidget* shadow = dialog.findChild<QWidget*>(QStringLiteral("qtControlsDialogShadow"));
    valid = valid && shadow != nullptr && dialog.windowShadowEnabled() && dialog.windowShadowSize() == 2;
    if (shadow == nullptr)
    {
        return false;
    }
    const QImage image = shadow->grab().toImage();
    image.save(QDir(directory).filePath(QStringLiteral("shadow-alpha.png")));
    const int ratio = shadow->devicePixelRatio();
    const QPoint body = shadow->mapFromGlobal(dialog.mapToGlobal(QPoint(0, 0))) * ratio;
    const int width = dialog.width() * ratio;
    const int height = dialog.height() * ratio;
    const int cx = body.x() + width / 2;
    const int cy = body.y() + height / 2;
    const int distances[] = { 1, 4, 8, 16, 26 };
    int previous = 256;
    QFile file(QDir(directory).filePath(QStringLiteral("shadow-parameters.txt")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }
    QTextStream log(&file);
    for (int i = 0; i < 5; ++i)
    {
        const int alpha = qAlpha(image.pixel(body.x() + width + distances[i] * ratio - 1, cy));
        log << "distance=" << distances[i] << " alpha=" << alpha << "\n";
        valid = valid && alpha <= previous;
        previous = alpha;
    }
    const int side = qAlpha(image.pixel(body.x() - ratio, cy));
    const int top = qAlpha(image.pixel(cx, body.y() - ratio));
    const int bottom = qAlpha(image.pixel(cx, body.y() + height + ratio));
    valid = valid && side > 0 && side < 100 && top > 0 && bottom > top && bottom < 120 && previous <= 1 &&
        qAlpha(image.pixel(cx, cy)) == 0 && qAlpha(image.pixel(0, cy)) <= 1 &&
        dialog.geometry() == original && !dialog.testAttribute(Qt::WA_TranslucentBackground) &&
        shadow->windowFlags().testFlag(Qt::WindowTransparentForInput);
    log << "side=" << side << " top=" << top << " bottom=" << bottom << " geometry=" << (dialog.geometry() == original) << "\n";
    dialog.setWindowShadow(true, INT_MAX);
    QTest::qWait(50);
    valid = valid && shadow->width() <= 1000 && shadow->height() <= 850 && dialog.geometry() == original;
    dialog.setWindowShadow(false, 2);
    valid = valid && !shadow->isVisible() && dialog.geometry() == original;
    dialog.setWindowShadow(true, 2);
    dialog.move(20, 20);
    dialog.resize(620, 440);
    QTest::qWait(50);
    valid = valid && shadow->size() == QSize(676, 496) && shadow->pos() == QPoint(-8, -8);
#ifdef Q_OS_WIN
    // A top-level shadow is not a title-bar child control, even near screen origin.
    const QPoint title = dialog.mapToGlobal(QPoint(180, 21));
    const LRESULT hit = SendMessage(reinterpret_cast<HWND>(dialog.winId()), WM_NCHITTEST, 0,
        MAKELPARAM(static_cast<WORD>(title.x()), static_cast<WORD>(title.y())));
    valid = valid && hit == HTCAPTION;
    log << "near origin title hit=" << hit << "\n";
#endif
    log << "passed=" << valid << "\n";
    return valid;
}

bool SplitViewerShadowTests::checkLifetime(const QString& directory)
{
    QPointer<QWidget> observer;
    bool valid = true;
    {
        DialogBase dialog;
        dialog.setGeometry(350, 280, 520, 340);
        dialog.setWindowShadow(true, 2);
        observer = dialog.findChild<QWidget*>(QStringLiteral("qtControlsDialogShadow"));
        if (observer.isNull())
        {
            return false;
        }
        for (int i = 0; i < 3; ++i)
        {
            dialog.show();
            dialog.activateWindow();
            QTest::qWait(80);
            valid = valid && observer->isVisible() && QApplication::activeWindow() == &dialog;
            dialog.hide();
            QTest::qWait(20);
            valid = valid && !observer->isVisible();
        }
        dialog.show();
        dialog.showMinimized();
        QTest::qWait(100);
        valid = valid && !observer->isVisible();
        dialog.showNormal();
        QTest::qWait(100);
        valid = valid && observer->isVisible();
        dialog.setWindowShadow(true, 0);
        valid = valid && !observer->isVisible();
        dialog.setWindowShadow(true, 2);
        QTest::qWait(30);
        QTest::keyClick(&dialog, Qt::Key_Escape);
        valid = valid && !dialog.isVisible() && !observer->isVisible();
    }
    valid = valid && observer.isNull();
    QFile file(QDir(directory).filePath(QStringLiteral("shadow-lifetime.txt")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }
    QTextStream(&file) << "three show/hide cycles, minimize/restore, zero, Esc, destruction=" << valid << "\n";
    return valid;
}

bool SplitViewerShadowTests::checkNativeDrag(const QString& directory, bool fullWindowDrag, bool cancelMove)
{
#ifdef Q_OS_WIN
    BOOL previous = FALSE;
    if (!SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &previous, 0) ||
        !SystemParametersInfo(SPI_SETDRAGFULLWINDOWS, fullWindowDrag ? TRUE : FALSE, nullptr, 0))
    {
        return false;
    }
    SplitViewer window;
    window.resize(1000, 720);
    window.show();
    window.activateWindow();
    QTest::qWait(150);
    QProcess driver;
    // Grant only our child the foreground permission owned by this test process.
    QObject::connect(&driver, &QProcess::started, [&driver]()
    {
        AllowSetForegroundWindow(static_cast<DWORD>(driver.processId()));
    });
    bool started = false;
    bool timedOut = false;
    QElapsedTimer time;
    time.start();
    QTimer poll;
    // 参数和输出仅传给本Test子进程；结束后通过真实Esc关闭产品关于框。
    QObject::connect(&poll, &QTimer::timeout, [&]()
    {
        QWidget* modal = QApplication::activeModalWidget();
        if (time.elapsed() > 20000)
        {
            timedOut = true;
            if (modal != nullptr)
            {
                modal->close();
            }
            return;
        }
        if (modal == nullptr)
        {
            return;
        }
        if (!started)
        {
            started = true;
            QStringList arguments;
            arguments << QStringLiteral("--shadow-drag-driver") << QString::number(static_cast<qulonglong>(modal->winId())) <<
                QString::number(QCoreApplication::applicationPid()) << directory << (cancelMove ? QStringLiteral("cancel") :
                fullWindowDrag ? QStringLiteral("full") : QStringLiteral("outline"));
            driver.start(QCoreApplication::applicationFilePath(), arguments);
        }
        else if (driver.state() == QProcess::NotRunning)
        {
            QTest::keyClick(modal, Qt::Key_Escape);
        }
    });
    poll.start(30);
    QAction* about = window.findChild<QAction*>(QStringLiteral("aboutAction"));
    if (about != nullptr)
    {
        about->trigger();
    }
    poll.stop();
    if (driver.state() != QProcess::NotRunning)
    {
        driver.kill();
        driver.waitForFinished(2000);
    }
    const bool restored = SystemParametersInfo(SPI_SETDRAGFULLWINDOWS, previous, nullptr, 0) != FALSE;
    DialogCountOperateParam count;
    DialogManager::instance().operateDialog(count);
    return restored && started && !timedOut && driver.exitStatus() == QProcess::NormalExit &&
        driver.exitCode() == 0 && count.m_count == 0;
#else
    Q_UNUSED(directory);
    Q_UNUSED(fullWindowDrag);
    Q_UNUSED(cancelMove);
    return false;
#endif
}

bool SplitViewerShadowTests::hasDarkOutline(const QImage& desktop, const QRect& target)
{
    if (!desktop.rect().contains(target.adjusted(-3, -3, 3, 3)))
    {
        return false;
    }
    for (int edge = 0; edge < 4; ++edge)
    {
        for (int part = 1; part <= 9; ++part)
        {
            const int x = target.left() + target.width() * part / 10;
            const int y = target.top() + target.height() * part / 10;
            bool dark = false;
            for (int offset = -2; offset <= 2; ++offset)
            {
                const QPoint point = edge == 0 ? QPoint(target.left() + offset, y) :
                    edge == 1 ? QPoint(target.right() + offset, y) :
                    edge == 2 ? QPoint(x, target.top() + offset) : QPoint(x, target.bottom() + offset);
                const QColor color(desktop.pixel(point));
                dark = dark || (color.red() < 40 && color.green() < 40 && color.blue() < 40);
            }
            if (!dark)
            {
                return false;
            }
        }
    }
    return true;
}

bool SplitViewerShadowTests::hasSoftShadow(const QImage& desktop, const QRect& body)
{
    if (!desktop.rect().contains(body.adjusted(-35, -35, 35, 35)))
    {
        return false;
    }
    const int x = body.center().x();
    const int y = body.center().y();
    const int backdrop = qGray(desktop.pixel(body.right() + 35, y));
    const int nearSide = qGray(desktop.pixel(body.right() + 3, y));
    const int farSide = qGray(desktop.pixel(body.right() + 18, y));
    const int top = qGray(desktop.pixel(x, body.top() - 3));
    const int bottom = qGray(desktop.pixel(x, body.bottom() + 3));
    return nearSide < farSide && farSide <= backdrop && backdrop - farSide <= 3 &&
        nearSide > backdrop - 65 && bottom < top && top < backdrop;
}

int SplitViewerShadowTests::runDragDriver(const QStringList& arguments)
{
#ifdef Q_OS_WIN
    if (arguments.size() != 6 || QGuiApplication::primaryScreen() == nullptr)
    {
        return 2;
    }
    bool handleValid = false;
    const HWND target = reinterpret_cast<HWND>(arguments.at(2).toULongLong(&handleValid));
    const DWORD expectedPid = arguments.at(3).toUInt();
    DWORD actualPid = 0;
    GetWindowThreadProcessId(target, &actualPid);
    if (!handleValid || expectedPid == 0 || actualPid != expectedPid || !IsWindowVisible(target))
    {
        return 2;
    }
    const QString directory = arguments.at(4);
    const bool full = arguments.at(5) == QStringLiteral("full");
    const bool cancel = arguments.at(5) == QStringLiteral("cancel");
    QFile file(QDir(directory).filePath(QStringLiteral("native-drag.txt")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return 1;
    }
    QTextStream log(&file);
    POINT savedCursor = {};
    GetCursorPos(&savedCursor);
    RECT before = {};
    GetWindowRect(target, &before);
    const QRect body(before.left, before.top, before.right - before.left, before.bottom - before.top);
    const QRect desired = body.translated(96, 56);
    const bool wasTopmost = (GetWindowLongPtr(target, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
    SetWindowPos(target, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetForegroundWindow(target);
    QTest::qWait(100);
    // Sequential CLI processes may lose foreground permission. Activate by a real
    // click only after WindowFromPoint proves that the point belongs to our target.
    bool activationClick = false;
    if (GetForegroundWindow() != target)
    {
        POINT titlePoint = { before.left + 180, before.top + 21 };
        if (GetAncestor(WindowFromPoint(titlePoint), GA_ROOT) == target)
        {
            SetCursorPos(titlePoint.x, titlePoint.y);
            INPUT click[2] = {};
            click[0].type = INPUT_MOUSE;
            click[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            click[1] = click[0];
            click[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
            activationClick = SendInput(2, click, sizeof(INPUT)) == 2;
        }
    }
    SetWindowPos(target, wasTopmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    // Exclude a title double-click from the subsequent drag, and let DWM present.
    QTest::qWait(activationClick ? GetDoubleClickTime() + 30 : 250);
    const HWND foreground = GetForegroundWindow();
    WCHAR foregroundTitle[256] = {};
    GetWindowTextW(foreground, foregroundTitle, 256);
    BOOL actualFull = FALSE;
    SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &actualFull, 0);
    log << "foregroundMatches=" << (foreground == target) << " actualFull=" << actualFull <<
        " foregroundTitle=" << QString::fromWCharArray(foregroundTitle) << "\n";
    if (foreground != target || (actualFull != FALSE) != full)
    {
        SetCursorPos(savedCursor.x, savedCursor.y);
        return 1;
    }
    const QImage rest = QGuiApplication::primaryScreen()->grabWindow(0).toImage();
    rest.save(QDir(directory).filePath(QStringLiteral("desktop-rest.png")));
    QGuiApplication::primaryScreen()->grabWindow(0, body.left() - 40, body.top() - 40,
        body.width() + 80, body.height() + 80).save(QDir(directory).filePath(QStringLiteral("about-shadow.png")));
    bool valid = hasSoftShadow(rest, body) && !hasDarkOutline(rest, desired);
    const int x = before.left + 180;
    const int y = before.top + 21;
    SetCursorPos(x, y);
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    valid = SendInput(1, &input, sizeof(INPUT)) == 1 && valid;
    QTest::qWait(100);
    for (int step = 1; step <= 8; ++step)
    {
        SetCursorPos(x + step * 12, y + step * 7);
        QTest::qWait(50);
    }
    QTest::qWait(150);
    RECT during = {};
    GetWindowRect(target, &during);
    const QImage moving = QGuiApplication::primaryScreen()->grabWindow(0).toImage();
    moving.save(QDir(directory).filePath(QStringLiteral("desktop-moving.png")));
    const bool outline = hasDarkOutline(moving, desired);
    log << "full=" << full << " before=" << before.left << "," << before.top <<
        " during=" << during.left << "," << during.top << " outline=" << outline <<
        " initialShadow=" << hasSoftShadow(rest, body) << "\n";
    valid = valid && (full ? (during.left == before.left + 96 && during.top == before.top + 56 && !outline) :
        (during.left == before.left && during.top == before.top && outline));
    if (cancel)
    {
        INPUT escape[2] = {};
        escape[0].type = INPUT_KEYBOARD;
        escape[0].ki.wVk = VK_ESCAPE;
        escape[1] = escape[0];
        escape[1].ki.dwFlags = KEYEVENTF_KEYUP;
        valid = SendInput(2, escape, sizeof(INPUT)) == 2 && valid;
        QTest::qWait(100);
    }
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    valid = SendInput(1, &input, sizeof(INPUT)) == 1 && valid;
    QTest::qWait(200);
    RECT after = {};
    GetWindowRect(target, &after);
    const QImage moved = QGuiApplication::primaryScreen()->grabWindow(0).toImage();
    moved.save(QDir(directory).filePath(QStringLiteral("desktop-moved.png")));
    const QRect finalBody = cancel ? body : desired;
    GUITHREADINFO thread = {};
    thread.cbSize = sizeof(thread);
    const bool released = GetGUIThreadInfo(GetWindowThreadProcessId(target, nullptr), &thread) && thread.hwndCapture == nullptr;
    valid = valid && IsWindowVisible(target) && after.left == finalBody.left() && after.top == finalBody.top() &&
        hasSoftShadow(moved, finalBody) && !hasDarkOutline(moved, desired) && released;
    SetCursorPos(savedCursor.x, savedCursor.y);
    log << "after=" << after.left << "," << after.top << " finalShadow=" << hasSoftShadow(moved, finalBody) <<
        " noRemainingFrame=" << !hasDarkOutline(moved, desired) << " captureReleased=" << released <<
        " cancel=" << cancel << " passed=" << valid << "\n";
    return valid ? 0 : 1;
#else
    Q_UNUSED(arguments);
    return 2;
#endif
}