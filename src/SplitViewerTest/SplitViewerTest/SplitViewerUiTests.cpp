#include "SplitViewerUiTests.h"
#include "../../SplitViewer/SplitViewer/SplitViewer.h"
#include "SplitViewerDialogTests.h"
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
#include "../../SplitViewer/SplitViewer/SplitViewerPlatform.h"
#include <algorithm>
#include <cmath>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

/** 从实际右键事件打开菜单并点击指定项
@param [in] canvas 产品工作区
@param [in] point 菜单命中点
@param [in] label 菜单文案
@return 菜单项已被实际点击
*/
static bool ChooseMenu(QWidget* canvas, const QPoint& point, const QString& label)
{
    bool clicked = false;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]()
    {
        QMenu* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (!menu)
        {
            return;
        }
        foreach (QAction* action, menu->actions())
        {
            if (action->text().contains(label) && action->isEnabled())
            {
                timer.stop();
                QTest::mouseClick(menu, Qt::LeftButton, Qt::NoModifier, menu->actionGeometry(action).center());
                clicked = true;
                return;
            }
        }
        timer.stop();
        menu->close();
    });
    timer.start(10);
    QContextMenuEvent event(QContextMenuEvent::Mouse, point, canvas->mapToGlobal(point));
    QApplication::sendEvent(canvas, &event);
    timer.stop();
    QApplication::processEvents();
    return clicked;
}

SplitViewerTestWindow::SplitViewerTestWindow() : browseCount(0)
{
    resize(900,700);
    show();
    activateWindow();
    QTest::qWait(30);
}

QString SplitViewerTestWindow::browseFile(bool save, const QString& title, const QString& initial, const QString& filter)
{
    Q_UNUSED(save); Q_UNUSED(title); Q_UNUSED(initial); Q_UNUSED(filter);
    ++browseCount;
    const QString result=nextFile;
    nextFile.clear();
    return result;
}

void SplitViewerTestWindow::reportError(const QString& message)
{
    errors.append(message);
}

// 预期直接来自旧版的10像素边距、桌面比例舞台、1像素边框和2像素分隔线。
static QRectF ExpectedStage(QWidget* canvas)
{
    const QRect screen=QApplication::desktop()->screenGeometry(canvas);
    const double aspect=static_cast<double>(screen.width())/static_cast<double>(screen.height());
    QSizeF size(canvas->width()-20,canvas->height()-20);
    if (size.width()/size.height()>aspect) size.setWidth(size.height()*aspect);
    else size.setHeight(size.width()/aspect);
    return QRectF(QPointF((canvas->width()-size.width())/2,(canvas->height()-size.height())/2),size);
}

static bool Pixel(QWidget* widget,const QPoint& point,const QColor& expected)
{
    QApplication::processEvents();
    const QImage image=widget->grab().toImage();
    return image.rect().contains(point) && QColor(image.pixel(point))==expected;
}

static void Drag(QWidget* widget,const QPoint& from,const QPoint& to,Qt::KeyboardModifiers modifiers=Qt::NoModifier)
{
    QTest::mousePress(widget,Qt::LeftButton,modifiers,from);
    QMouseEvent move(QEvent::MouseMove,to,widget->mapToGlobal(to),Qt::NoButton,Qt::LeftButton,modifiers);
    QApplication::sendEvent(widget,&move);
    QTest::mouseRelease(widget,Qt::LeftButton,modifiers,to);
    QApplication::processEvents();
}

static void Wheel(QWidget* widget,const QPoint& point,int delta,Qt::KeyboardModifiers modifiers=Qt::NoModifier)
{
    QWheelEvent event(point,widget->mapToGlobal(point),QPoint(),QPoint(0,delta),delta,Qt::Vertical,Qt::NoButton,modifiers);
    QApplication::sendEvent(widget,&event);
    QApplication::processEvents();
}

static bool Drop(QWidget* widget,const QPoint& point,const QString& path)
{
    QMimeData mime;
    mime.setUrls(QList<QUrl>() << QUrl::fromLocalFile(path));
    QDragEnterEvent enter(point,Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(widget,&enter);
    QDropEvent drop(point,Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(widget,&drop);
    QApplication::processEvents();
    return enter.isAccepted() && drop.isAccepted();
}

static bool Toolbar(SplitViewer& window,const QString& label)
{
    QToolBar* bar=window.findChild<QToolBar*>();
    foreach (QAction* action,bar->actions())
    {
        if (action->text()==label || action->toolTip()==label || action->objectName()==QStringLiteral("aboutAction"))
        {
            QWidget* button=bar->widgetForAction(action);
            QTest::mouseClick(button,Qt::LeftButton);
            QApplication::processEvents();
            return true;
        }
    }
    return false;
}

static bool ReadDocument(const QString& path,SplitViewerCoreDocument& document)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    const QByteArray bytes=file.readAll();
    std::vector<uint8_t> package(bytes.begin(),bytes.end()),config;
    return SplitViewerCoreExtractEmbeddedConfig(package,config) && SplitViewerCoreDeserializeProfile(config,document);
}

static std::vector<uint8_t> Utf16(const QString& text)
{
    std::vector<uint8_t> bytes;
    bytes.push_back(255); bytes.push_back(254);
    for (int i=0;i<text.size();++i)
    {
        const ushort c=text.at(i).unicode();
        bytes.push_back(c & 255); bytes.push_back(c >> 8);
    }
    return bytes;
}

#ifdef Q_OS_WIN
/** Generate an indexed uncompressed AVI with independent colored quadrants
@param [in] path Output test fixture path
@return Complete fixture was written
*/
static bool WriteVideoFixture(const QString& path)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    const int width = 160, height = 96, frames = 400, frameBytes = width * height * 3;
    stream.writeRawData("RIFF", 4); stream << quint32(0); stream.writeRawData("AVI LIST", 8);
    stream << quint32(192); stream.writeRawData("hdrlavih", 8); stream << quint32(56);
    stream << quint32(100000) << quint32(frameBytes * 10) << quint32(0) << quint32(16)
        << quint32(frames) << quint32(0) << quint32(1) << quint32(frameBytes)
        << quint32(width) << quint32(height) << quint32(0) << quint32(0) << quint32(0) << quint32(0);
    stream.writeRawData("LIST", 4); stream << quint32(116); stream.writeRawData("strlstrh", 8);
    stream << quint32(56); stream.writeRawData("vidsDIB ", 8);
    stream << quint32(0) << quint16(0) << quint16(0) << quint32(0) << quint32(1) << quint32(10)
        << quint32(0) << quint32(frames) << quint32(frameBytes) << quint32(0xffffffff) << quint32(0)
        << qint16(0) << qint16(0) << qint16(width) << qint16(height);
    stream.writeRawData("strf", 4); stream << quint32(40) << quint32(40) << qint32(width) << qint32(height)
        << quint16(1) << quint16(24) << quint32(0) << quint32(frameBytes)
        << qint32(0) << qint32(0) << quint32(0) << quint32(0);
    stream.writeRawData("LIST", 4); stream << quint32(4 + frames * (frameBytes + 8)); stream.writeRawData("movi", 4);
    for (int frame = 0; frame < frames; ++frame)
    {
        stream.writeRawData("00db", 4); stream << quint32(frameBytes);
        for (int y = height - 1; y >= 0; --y)
        {
            for (int x = 0; x < width; ++x)
            {
                const int pulse = (frame / 3) % 2 ? 220 : 140;
                QColor color = y < height / 2 ? (x < width / 2 ? QColor(pulse,30,30) : QColor(30,pulse,30))
                    : (x < width / 2 ? QColor(30,30,pulse) : QColor(pulse,pulse,30));
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
    stream.writeRawData("idx1", 4); stream << quint32(frames * 16);
    for (int frame = 0; frame < frames; ++frame)
    {
        stream.writeRawData("00db", 4); stream << quint32(16) << quint32(4 + frame * (frameBytes + 8)) << quint32(frameBytes);
    }
    stream.device()->seek(4); stream << quint32(bytes.size() - 8);
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}

/** Find the visible top-level window belonging exclusively to our child process */
static BOOL CALLBACK FindPlayerWindow(HWND hwnd, LPARAM parameter)
{
    QPair<DWORD, HWND>* found = reinterpret_cast<QPair<DWORD, HWND>*>(parameter);
    DWORD process = 0;
    GetWindowThreadProcessId(hwnd, &process);
    if (process == found->first && IsWindowVisible(hwnd))
    {
        found->second = hwnd;
    }
    return TRUE;
}

/** Capture actual desktop pixels in a pane, rather than an offscreen widget render */
static QImage ScreenPane(const QRect& pane)
{
    return QGuiApplication::primaryScreen()->grabWindow(0, pane.x(), pane.y(), pane.width(), pane.height()).toImage();
}

/** Check four independently generated video colors near the fitted image edges */
static bool VideoPixelsFill(const QImage& screen)
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
        const QColor color(screen.pixel(qRound(left + (quadrant % 2 ? 136 : 24) * scale),
            qRound(top + (quadrant / 2 ? 72 : 24) * scale)));
        const bool red = quadrant == 0 || quadrant == 3;
        const bool green = quadrant == 1 || quadrant == 3;
        const bool blue = quadrant == 2;
        if ((red ? color.red() < 100 : color.red() > 65) ||
            (green ? color.green() < 100 : color.green() > 65) ||
            (blue ? color.blue() < 100 : color.blue() > 65))
        {
            return false;
        }
    }
    return true;
}

/** Decode the fixture frame number from actual red-quadrant pixels
@param [in] screen Desktop image of the independently predicted fitted pane
@return Frame number, or -1 when the expected video/band is not visible
*/
static int VideoFrameNumber(const QImage& screen)
{
    if (!VideoPixelsFill(screen)) return -1;
    const double scale = (std::min)(screen.width()/160.0, screen.height()/96.0);
    const double left = (screen.width()-160*scale)/2;
    const double top = (screen.height()-96*scale)/2;
    int frame = 0;
    for (int bit = 0; bit < 9; ++bit)
    {
        const QColor color(screen.pixel(qRound(left+(13+bit*6)*scale),qRound(top+32*scale)));
        if (color.green()>65 || color.blue()>65 || color.red()<65) return -1;
        if (color.red()>160) frame |= 1 << bit;
    }
    return frame < 400 ? frame : -1;
}

/** Observe distinct fixture frame numbers within a bounded wall-clock interval
@param [in] pane Expected pane rectangle in desktop coordinates
@param [in] observationMilliseconds Deadline including the click debounce budget
@return True only when two valid video frame numbers differ
*/
static bool VideoIsAdvancing(const QRect& pane, int observationMilliseconds = 800, QTextStream* trace = nullptr)
{
    QElapsedTimer timer;
    timer.start();
    int first = VideoFrameNumber(ScreenPane(pane));
    if (trace) *trace << "frame initial=" << first << " epoch=" << QDateTime::currentMSecsSinceEpoch() << "\n";
    while (timer.elapsed() < observationMilliseconds)
    {
        QTest::qWait(100);
        const int next = VideoFrameNumber(ScreenPane(pane));
        if (trace) *trace << "frame t=" << timer.elapsed() << " value=" << next << " epoch=" << QDateTime::currentMSecsSinceEpoch() << "\n";
        if (first >= 0 && next >= 0 && first != next) return true;
        if (first < 0) first = next;
    }
    return false;
}

static LRESULT CALLBACK SplitViewerNativeFixtureProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    HWND video=static_cast<HWND>(GetPropW(hwnd,L"SplitViewerVideoSurface"));
    if (message==WM_CREATE)
    {
        video=CreateWindowExW(WS_EX_CLIENTEDGE,L"STATIC",L"video surface",
            WS_CHILD|WS_VISIBLE|SS_CENTER,0,0,1,1,hwnd,NULL,GetModuleHandleW(nullptr),NULL);
        SetPropW(hwnd,L"SplitViewerVideoSurface",video);
        return 0;
    }
    if (message==WM_SIZE)
    {
        if (video)
        {
            MoveWindow(video,0,0,(std::max)(1,static_cast<int>(LOWORD(lParam))),
                (std::max)(1,static_cast<int>(HIWORD(lParam))),TRUE);
        }
        return 0;
    }
    if (message==WM_LBUTTONDBLCLK)
    {
        // 模拟LumaPlayer自身的双击全屏：若SplitViewer没有先拦截，
        // 这个窗口会脱离容器，回归用例即可捕获该回归。
        SetParent(hwnd,NULL);
        ShowWindow(hwnd,SW_MAXIMIZE);
        return 0;
    }
    if (message==WM_NCDESTROY)
    {
        if (video)
        {
            RemovePropW(hwnd,L"SplitViewerVideoSurface");
            DestroyWindow(video);
        }
    }
    return DefWindowProcW(hwnd,message,wParam,lParam);
}

static bool SplitViewerNativeVideoFills(HWND parent)
{
    const HWND video=static_cast<HWND>(GetPropW(parent,L"SplitViewerVideoSurface"));
    if (!IsWindow(parent) || !IsWindow(video)) return false;
    RECT parentClient={0};
    RECT videoRect={0};
    if (!GetClientRect(parent,&parentClient) || !GetWindowRect(video,&videoRect)) return false;
    POINT points[2]={{videoRect.left,videoRect.top},{videoRect.right,videoRect.bottom}};
    MapWindowPoints(NULL,parent,points,2);
    return points[0].x==parentClient.left && points[0].y==parentClient.top &&
        points[1].x==parentClient.right && points[1].y==parentClient.bottom;
}

static bool NativeDrag(const QPoint& from,const QPoint& to)
{
    SetCursorPos(from.x(),from.y());
    QTest::qWait(20);
    INPUT input={0}; input.type=INPUT_MOUSE; input.mi.dwFlags=MOUSEEVENTF_LEFTDOWN;
    const bool down=SendInput(1,&input,sizeof(input))==1;
    QTest::qWait(20);
    SetCursorPos(to.x(),to.y());
    QTest::qWait(20);
    input.mi.dwFlags=MOUSEEVENTF_LEFTUP;
    const bool up=SendInput(1,&input,sizeof(input))==1;
    QTest::qWait(80);
    return down && up;
}

static bool NativeDoubleClick(const QPoint& point)
{
    SetCursorPos(point.x(),point.y());
    for (int i=0;i<2;++i)
    {
        INPUT input={0}; input.type=INPUT_MOUSE; input.mi.dwFlags=MOUSEEVENTF_LEFTDOWN;
        const bool down=SendInput(1,&input,sizeof(input))==1;
        QTest::qWait(70);
        input.mi.dwFlags=MOUSEEVENTF_LEFTUP;
        const bool up=SendInput(1,&input,sizeof(input))==1;
        if (!down || !up) return false;
        if (i==0) QTest::qWait(80);
    }
    QTest::qWait(100);
    return true;
}

/** Deliver a real single click to the expected embedded process only */
static bool NativePlayerClick(HWND player, const QPoint& point)
{
    const POINT screenPoint = {point.x(), point.y()};
    HWND target = WindowFromPoint(screenPoint);
    while (target && target != player)
    {
        target = GetParent(target);
    }
    if (target != player)
    {
        return false;
    }
    SetCursorPos(point.x(),point.y());
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    const bool down = SendInput(1,&input,sizeof(input)) == 1;
    QTest::qWait(30);
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    return SendInput(1,&input,sizeof(input)) == 1 && down;
}
#endif

int SplitViewerRunUiTests(const QString& reportDirectory, int selectedCase)
{
#ifdef Q_OS_WIN
    WNDCLASSW fixtureClass={0};
    fixtureClass.lpfnWndProc=SplitViewerNativeFixtureProc;
    fixtureClass.style=CS_DBLCLKS;
    fixtureClass.hInstance=GetModuleHandleW(nullptr);
    fixtureClass.lpszClassName=L"SplitViewerNativeTestFixture";
    fixtureClass.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
    RegisterClassW(&fixtureClass);
#endif
    if (selectedCase!=0 && (selectedCase<101 || selectedCase>177)) return 2;
    QDir().mkpath(reportDirectory);
    QFile report(reportDirectory+QStringLiteral("/ui-results.txt"));
    if (!report.open(QIODevice::WriteOnly|QIODevice::Text)) return 1;
    QTextStream out(&report);
    out.setCodec("UTF-8");
    int failures=0,total=0,nextId=100;
    auto check=[&](const char* name,bool success)
    {
        const int id=++nextId;
        if (selectedCase && selectedCase!=id) return;
        ++total;
        out << id << " " << QString::fromUtf8(name) << " " << (success ? "PASS" : "FAIL") << "\n";
        out.flush();
        failures+=!success;
    };
    const QColor red(230,30,40),blue(20,80,230),green(30,200,70),gray(128,128,128),plus(70,70,70);
    const QString redPath=reportDirectory+QStringLiteral("/红色.png");
    const QString bluePath=reportDirectory+QStringLiteral("/蓝色.png");
    const QString greenPath=reportDirectory+QStringLiteral("/绿色.png");
    QImage input(120,120,QImage::Format_RGB32);
    input.fill(red); input.save(redPath);
    input.fill(blue); input.save(bluePath);
    input.fill(green); input.save(greenPath);
    if (selectedCase==0 || (selectedCase>100 && selectedCase<=117))
    {
        nextId=100;
        SplitViewerTestWindow window;
        QWidget* canvas=window.centralWidget();
        const QRectF stage=ExpectedStage(canvas),inside=stage.adjusted(1,1,-1,-1);
        const QPoint center=inside.center().toPoint();
        check("empty colors and plus",Pixel(canvas,center,plus) && Pixel(canvas,center+QPoint(60,0),gray));
        QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,center);
        check("plus cancel keeps empty",window.browseCount==1 && Pixel(canvas,center,plus));
        check("horizontal context split",ChooseMenu(canvas,center+QPoint(70,0),QStringLiteral("水平分割")));
        const QPoint upper(center.x(),qRound(inside.top()+(inside.height()-2)/4));
        const QPoint lower(center.x(),qRound(inside.bottom()-(inside.height()-2)/4));
        check("both split plus visible",Pixel(canvas,upper,plus) && Pixel(canvas,lower,plus));
        window.nextFile=redPath;
        QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,upper);
        check("upper plus loads visible image",window.browseCount==2 && Pixel(canvas,upper,red));
        window.nextFile=bluePath;
        QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,lower);
        check("lower plus loads visible image",window.browseCount==3 && Pixel(canvas,lower,blue) && Pixel(canvas,upper,red));
        Wheel(canvas,upper,120*100);
        check("zoom clipped to own split",Pixel(canvas,lower,blue));
        Drag(canvas,upper,upper+QPoint(30,20));
        const QString profile=reportDirectory+QStringLiteral("/split.sv");
        window.nextFile=profile; Toolbar(window,QStringLiteral("保存配置"));
        SplitViewerCoreDocument state;
        const bool loaded=ReadDocument(profile,state);
        check("save profile reflects real UI",loaded && state.baseRoot()->first && state.baseRoot()->first->view.path==redPath.toStdWString() && state.baseRoot()->second->view.path==bluePath.toStdWString());
        check("image pan uses pixels",loaded && state.baseRoot()->first && std::abs(state.baseRoot()->first->view.offsetX-30)<0.01 && std::abs(state.baseRoot()->first->view.offsetY-20)<0.01);
        check("nested vertical split",ChooseMenu(canvas,lower,QStringLiteral("垂直分割")));
        const QPoint left(qRound(inside.left()+(inside.width()-2)/4),lower.y());
        const QPoint right(qRound(inside.right()-(inside.width()-2)/4),lower.y());
        check("nested content retained and plus visible",Pixel(canvas,left,blue) && Pixel(canvas,right,plus));
        check("drop uses actual target",Drop(canvas,right,greenPath) && Pixel(canvas,right,green) && Pixel(canvas,left,blue));
        check("delete base split leaf",ChooseMenu(canvas,left,QStringLiteral("删除")) && Pixel(canvas,lower,green));
        Drag(canvas,center,center+QPoint(0,-70));
        Toolbar(window,QStringLiteral("保存配置"));
        SplitViewerCoreDocument moved;
        check("splitter drag persists",ReadDocument(profile,moved) && moved.baseRoot()->ratio<0.45 && moved.baseRoot()->ratio>0.1);
        check("delete collapse preserves image",ChooseMenu(canvas,upper-QPoint(0,100),QStringLiteral("删除")) && Pixel(canvas,center,green));
        check("root delete disabled",!ChooseMenu(canvas,center,QStringLiteral("删除")) && Pixel(canvas,center,green));
        const int errors=window.errors.size();
        Drop(canvas,center,reportDirectory+QStringLiteral("/missing.png"));
        check("bad image retains document",window.errors.size()==errors+1 && Pixel(canvas,center,green));
        canvas->grab().save(reportDirectory+QStringLiteral("/base.png"));
    }
    if (selectedCase==0 || (selectedCase>117 && selectedCase<=131))
    {
        nextId=117;
        SplitViewerTestWindow window;
        QWidget* canvas=window.centralWidget();
        const QRectF stage=ExpectedStage(canvas),layer(stage.left()+stage.width()*.22,stage.top()+stage.height()*.22,stage.width()*.46,stage.height()*.46);
        const QPoint center=layer.center().toPoint(),outside(qRound(stage.left()+stage.width()*.75),center.y());
        Drop(canvas,center,redPath);
        check("new layer toolbar",Toolbar(window,QStringLiteral("新建图层")) && Pixel(canvas,center,plus));
        window.nextFile=bluePath;
        QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,center);
        Wheel(canvas,center,120*100);
        const QImage zoomShot=canvas->grab().toImage();
        const QColor zoomCenter=QColor(zoomShot.pixel(center));
        const QColor zoomOutside=QColor(zoomShot.pixel(outside));
        check("floating zoom cannot escape layer",zoomCenter==blue && zoomOutside==red);
        Drag(canvas,center,center+QPoint(100,40));
        check("floating pan cannot escape layer",Pixel(canvas,outside,red));
        canvas->grab().save(reportDirectory+QStringLiteral("/layer-clipping.png"));
        Drag(canvas,center,center+QPoint(35,-25),Qt::ControlModifier);
        const QString profile=reportDirectory+QStringLiteral("/layer.sv");
        window.nextFile=profile; Toolbar(window,QStringLiteral("保存配置"));
        SplitViewerCoreDocument state;
        bool saved=ReadDocument(profile,state);
        check("ctrl drag moves layer only",saved && state.layerCount()==1 && std::abs(state.layerAt(0)->rect.left-(.22+35/stage.width()))<.001 && std::abs(state.layerAt(0)->root->view.offsetX-100)<.01);
        const QPoint edge(qRound(layer.right()+35-3),qRound(layer.center().y()-25));
        Drag(canvas,edge,edge+QPoint(55,0));
        Toolbar(window,QStringLiteral("保存配置"));
        SplitViewerCoreDocument resized;
        check("layer right edge resize",ReadDocument(profile,resized) && resized.layerCount()==1 && resized.layerAt(0)->rect.width()>0.51);
        check("layer split",ChooseMenu(canvas,center+QPoint(35,-25),QStringLiteral("水平分割")));
        const QPoint low(center.x()+35+qRound(55.0/2),qRound(layer.top()-25+layer.height()*.75));
        check("layer split lower plus",Pixel(canvas,low,plus));
        window.nextFile=greenPath; QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,low);
        check("layer split lower image",Pixel(canvas,low,green));
        Toolbar(window,QStringLiteral("保存配置"));
        const QString exportPath=reportDirectory+QStringLiteral("/export.png");
        window.nextFile=exportPath; Toolbar(window,QStringLiteral("另存图片"));
        const QImage exported(exportPath);
        const QSize screen=QApplication::desktop()->screenGeometry(&window).size();
        const double aspect=static_cast<double>(QApplication::desktop()->screenGeometry(&window).width())/
            static_cast<double>(QApplication::desktop()->screenGeometry(&window).height());
        QSize target(4000,static_cast<int>(4000/aspect)); target.scale(screen,Qt::KeepAspectRatio);
        check("export uses screen-fit stage only",!exported.isNull() && exported.size()==target && QColor(exported.pixel(exported.width()/2,exported.height()/8))==red);
        const QImage thumb(profile);
        const int thumbnailHeight=qRound(320.0/(static_cast<double>(QApplication::desktop()->screenGeometry(&window).width())/
            static_cast<double>(QApplication::desktop()->screenGeometry(&window).height())));
        check("sv PNG thumbnail bounded and stage only",!thumb.isNull() && thumb.width()==320 && std::abs(thumb.height()-thumbnailHeight)<=1);
        SplitViewerTestWindow restored;
        check("drop sv restores layers",Drop(restored.centralWidget(),restored.centralWidget()->rect().center(),profile));
        check("loaded layer image visible",Pixel(restored.centralWidget(),low,green));
        const int errors=restored.errors.size();
        QFile invalid(reportDirectory+QStringLiteral("/invalid.sv")); invalid.open(QIODevice::WriteOnly); invalid.write("bad"); invalid.close();
        Drop(restored.centralWidget(),low,invalid.fileName());
        check("invalid profile keeps existing layout",restored.errors.size()==errors+1 && Pixel(restored.centralWidget(),low,green));
        Drag(restored.centralWidget(),low,low+QPoint(10,10));
        check("interaction after reload remains valid",restored.errors.size()==errors+1);
    }
    if (selectedCase==0 || (selectedCase>131 && selectedCase<=137))
    {
        nextId=131;
        SplitViewerTestWindow window;
        QWidget* canvas=window.centralWidget();
        const QRect before=window.geometry();
        QTest::mouseDClick(canvas,Qt::LeftButton,Qt::NoModifier,QPoint(1,1));
        check("double click margin does not fullscreen",!window.isFullScreen());
        QTest::mouseDClick(canvas,Qt::LeftButton,Qt::NoModifier,canvas->rect().center()+QPoint(70,0));
        QTest::qWait(50);
        check("double click fullscreen content only",window.isFullScreen() && !window.findChild<QToolBar*>()->isVisible() && canvas->size()==window.size());
        window.grab().save(reportDirectory+QStringLiteral("/fullscreen.png"));
        QTest::keyClick(&window,Qt::Key_Escape); QTest::qWait(30);
        check("escape restores geometry and toolbar",!window.isFullScreen() && window.findChild<QToolBar*>()->isVisible() && window.geometry()==before);
        window.showMaximized(); QTest::qWait(100);
        QTest::keyClick(&window,Qt::Key_F11); QTest::qWait(30);
        check("F11 enters fullscreen",window.isFullScreen() && !window.findChild<QToolBar*>()->isVisible());
        QTest::keyClick(&window,Qt::Key_F11); QTest::qWait(30);
        check("F11 restores maximized state",window.isMaximized() && window.findChild<QToolBar*>()->isVisible());
        window.showNormal(); QTest::qWait(30);
        const QPoint center=ExpectedStage(canvas).center().toPoint();
        Drop(canvas,center,redPath);
        ChooseMenu(canvas,center,QStringLiteral("水平分割"));
        QTest::keyClick(&window,Qt::Key_F1);
        const QString profile=reportDirectory+QStringLiteral("/border.sv");
        window.nextFile=profile; Toolbar(window,QStringLiteral("保存配置"));
        SplitViewerCoreDocument state;
        check("F1 hides borders in persisted state",ReadDocument(profile,state) && !state.borderVisible());
    }
#ifdef Q_OS_WIN
    if (selectedCase==0 || (selectedCase>137 && selectedCase<=140))
    {
        nextId=137;
        SplitViewerTestWindow window;
        HWND fixture=CreateWindowExW(WS_EX_TOOLWINDOW,L"SplitViewerNativeTestFixture",L"SplitViewer native fixture",WS_OVERLAPPEDWINDOW|WS_VISIBLE,60,80,240,180,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        RECT before={0}; GetWindowRect(fixture,&before);
        const LONG_PTR style=GetWindowLongPtrW(fixture,GWL_STYLE);
        QWidget* container=SplitViewerEmbedForeignWindow(reinterpret_cast<WId>(fixture),window.centralWidget());
        if (container) { container->setGeometry(50,50,180,130); container->show(); }
        QTest::qWait(30);
        check("native embed creates child",container && IsWindow(fixture) && (GetWindowLongPtrW(fixture,GWL_STYLE)&WS_CHILD) && GetParent(fixture)!=nullptr);
        check("native snapshot",container && !SplitViewerForeignWindowSnapshot(container).isNull());
        SplitViewerDetachForeignWindow(container);
        QTest::qWait(30);
        RECT after={0}; GetWindowRect(fixture,&after);
        check("native detach restores surviving original",IsWindow(fixture) && GetParent(fixture)==nullptr && GetWindowLongPtrW(fixture,GWL_STYLE)==style && EqualRect(&before,&after));
        DestroyWindow(fixture);
    }
#endif
    if (selectedCase==0 || (selectedCase>140 && selectedCase<=153))
    {
        nextId=140;
        // 独立手写旧版配置，不调用产品序列化器生成预期。
        const QString fixture=QStringLiteral("[SplitViewer]\r\nVersion=2\r\nStageAspect=1.3333333333\r\nBorderVisible=0\r\nLayerCount=0\r\n[Base]\r\nRoot=0\r\n[BaseNode0]\r\nKind=Leaf\r\nPath=中文 path.png\r\nHasImage=1\r\nAutoFit=0\r\nScale=1.5\r\nOffsetX=37\r\nOffsetY=-21\r\n");
        SplitViewerCoreDocument document;
        check("legacy fields decoded independently",SplitViewerCoreDeserializeProfile(Utf16(fixture),document) && document.baseRoot()->view.path==L"中文 path.png" && document.baseRoot()->view.offsetX==37 && document.baseRoot()->view.offsetY==-21 && !document.borderVisible());
        const std::wstring original=document.baseRoot()->view.path;
        QString cycle=fixture; cycle.replace(QStringLiteral("Kind=Leaf"),QStringLiteral("Kind=Split\r\nFirst=0\r\nSecond=0"));
        check("cyclic profile rejected without replacing state",!SplitViewerCoreDeserializeProfile(Utf16(cycle),document) && document.baseRoot()->view.path==original);
        QString absent=cycle; absent.replace(QStringLiteral("First=0"),QStringLiteral("First=99"));
        check("missing node rejected",!SplitViewerCoreDeserializeProfile(Utf16(absent),document) && document.baseRoot()->view.path==original);
        std::vector<uint8_t> odd=Utf16(fixture); odd.pop_back();
        check("odd UTF16 rejected",!SplitViewerCoreDeserializeProfile(odd,document));
        check("unsupported profile version rejected",!SplitViewerCoreDeserializeProfile(Utf16(QString(fixture).replace(QStringLiteral("Version=2"),QStringLiteral("Version=999"))),document));
        QImage thumb(8,6,QImage::Format_RGB32); thumb.fill(red);
        QByteArray png; QBuffer buffer(&png); buffer.open(QIODevice::WriteOnly); thumb.save(&buffer,"PNG");
        std::vector<uint8_t> package,extracted,pngVector(png.begin(),png.end()),config=Utf16(fixture);
        SplitViewerCoreBuildConfigPackage(pngVector,config,package);
        bool crcRejected=false;
        for (size_t i=4;i+4<package.size();++i)
        {
            if (package[i]=='s' && package[i+1]=='v' && package[i+2]=='C' && package[i+3]=='f')
            {
                package[i+4]^=1;
                crcRejected=!SplitViewerCoreExtractEmbeddedConfig(package,extracted);
                break;
            }
        }
        check("corrupt svCf CRC rejected",crcRejected);
        const QByteArray marker("\r\n--SPLITVIEWER_CONFIG_UTF16LE--\r\n");
        std::vector<uint8_t> legacy(pngVector); legacy.insert(legacy.end(),marker.begin(),marker.end()); legacy.insert(legacy.end(),config.begin(),config.end());
        check("old trailing marker accepted",SplitViewerCoreExtractEmbeddedConfig(legacy,extracted) && extracted==config);
        SplitViewerCoreNode node; node.makeSplit(SPLITVIEWER_CORE_SPLIT_HORIZONTAL);
        SplitViewerCoreRect a,line,b;
        SplitViewerCoreSplitNodeRects(SplitViewerCoreRect(10,20,810,620),&node,a,line,b);
        check("split geometry exact absolute bounds",a.left==10 && a.top==20 && a.bottom==319 && line.top==319 && line.bottom==321 && b.top==321 && b.bottom==620);
        SplitViewerCoreSplitNodeRects(SplitViewerCoreRect(10,20,810,620),&node,a,line,b,0);
        check("borderless geometry has no separator gap",a.bottom==320 && line.height()==0 && b.top==320);
        SplitViewerCoreLeafState view; view.hasImage=true;
        SplitViewerCoreZoom(view,2,240,false);
        check("wheel notch arithmetic",std::abs(view.scale-2*1.05*1.05)<1e-9 && !view.autoFit);
        SplitViewerCoreZoom(view,2,120,true);
        check("ctrl fine wheel arithmetic",std::abs(view.scale-2*1.05*1.05*1.01)<1e-9);
        SplitViewerCoreZoom(view,2,120000,false);
        const bool maximum=view.scale==100;
        SplitViewerCoreZoom(view,2,-120000,false);
        check("zoom min max bounds",maximum && view.scale==.1);
        view.scale=2; view.offsetX=30; view.offsetY=-10;
        SplitViewerCoreResizeView(view,100,100,200,300);
        check("manual view resize preserves pixel proportions",view.scale==5 && view.offsetX==60 && view.offsetY==-30);
    }
#ifdef Q_OS_WIN
    if (selectedCase==0 || (selectedCase>153 && selectedCase<=156))
    {
        nextId=153;
        SplitViewerTestWindow window;
        QWidget* canvas=window.centralWidget();
        ChooseMenu(canvas,canvas->rect().center(),QStringLiteral("垂直分割"));
        const QRectF stage=ExpectedStage(canvas);
        const QPoint target=canvas->mapToGlobal(QPoint(qRound(stage.left()+stage.width()*.25),qRound(stage.center().y())));
        POINT savedCursor; GetCursorPos(&savedCursor);
        HWND fixture=CreateWindowExW(WS_EX_TOOLWINDOW,L"SplitViewerNativeTestFixture",L"SplitViewer drag fixture",WS_OVERLAPPEDWINDOW|WS_VISIBLE,30,40,240,180,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        const LONG_PTR style=GetWindowLongPtrW(fixture,GWL_STYLE);
        SetWindowPos(reinterpret_cast<HWND>(window.winId()),HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
        SetWindowPos(fixture,HWND_TOPMOST,30,40,240,180,SWP_SHOWWINDOW);
        QTest::qWait(50);
        check("native hook drag in to empty leaf",NativeDrag(QPoint(80,110),target) && GetParent(fixture)!=nullptr && (GetWindowLongPtrW(fixture,GWL_STYLE)&WS_CHILD));
        const bool wasEmbedded=GetParent(fixture)!=nullptr;
        const bool draggedOut=NativeDrag(target,QPoint(180,100));
        check("native drag out restores top level",wasEmbedded && draggedOut && IsWindow(fixture) && GetParent(fixture)==nullptr && GetWindowLongPtrW(fixture,GWL_STYLE)==style);
        RECT detached={0}; GetWindowRect(fixture,&detached);
        const QPoint from((detached.left+detached.right)/2,(detached.top+detached.bottom)/2);
        NativeDrag(from,target);
        const bool embeddedAgain=GetParent(fixture)!=nullptr;
        window.close(); QTest::qWait(30);
        check("closing viewer restores dragged window",embeddedAgain && IsWindow(fixture) && GetParent(fixture)==nullptr && GetWindowLongPtrW(fixture,GWL_STYLE)==style);
        DestroyWindow(fixture);
        SetCursorPos(savedCursor.x,savedCursor.y);
    }
#endif
    if (selectedCase==0 || selectedCase==157 || selectedCase==158)
    {
        nextId=156;
        SplitViewerTestWindow window;
        QWidget* canvas=window.centralWidget();
        const QPoint center=canvas->rect().center();
        Drop(canvas,center,redPath);
        bool formats=true;
        foreach (const QString& extension,QStringList() << "png" << "jpg" << "bmp" << "tiff")
        {
            const QString file=reportDirectory+"/format."+extension;
            window.nextFile=file; Toolbar(window,QStringLiteral("另存图片"));
            const QImage decoded(file);
            if (decoded.isNull()) formats=false;
            else
            {
                const QColor color(decoded.pixel(decoded.width()/2,decoded.height()/2));
                formats=formats && std::abs(color.red()-red.red())<=3 && std::abs(color.green()-red.green())<=3 && std::abs(color.blue()-red.blue())<=3;
            }
        }
        const int errors=window.errors.size();
        window.nextFile=reportDirectory+"/unsupported.gif"; Toolbar(window,QStringLiteral("另存图片"));
        check("Qt image format exports and unsupported GIF feedback",formats && window.errors.size()==errors+1 && Pixel(canvas,center,red));
        Wheel(canvas,center,120);
        QLabel* tip=canvas->findChild<QLabel*>();
        const bool shown=tip && tip->isVisible() && tip->text()==QStringLiteral("105%");
        QTest::qWait(1100);
        check("zoom percentage popup expires",shown && !tip->isVisible());
    }
#ifdef Q_OS_WIN
    if (selectedCase==0 || selectedCase==159 || selectedCase==160)
    {
        nextId=158;
        SplitViewerTestWindow window;
        QWidget* canvas=window.centralWidget();
        ChooseMenu(canvas,canvas->rect().center(),QStringLiteral("垂直分割"));
        const QRectF stage=ExpectedStage(canvas);
        const QPoint left=canvas->mapToGlobal(QPoint(qRound(stage.left()+stage.width()*.25),qRound(stage.center().y())));
        const QPoint right=canvas->mapToGlobal(QPoint(qRound(stage.left()+stage.width()*.75),qRound(stage.center().y())));
        POINT savedCursor; GetCursorPos(&savedCursor);
        SetWindowPos(reinterpret_cast<HWND>(window.winId()),HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
        HWND first=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,L"SplitViewerNativeTestFixture",L"SplitViewer first fixture",WS_OVERLAPPEDWINDOW|WS_VISIBLE,30,40,240,180,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        QTest::qWait(20); NativeDrag(QPoint(80,110),left);
        HWND second=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,L"SplitViewerNativeTestFixture",L"SplitViewer second fixture",WS_OVERLAPPEDWINDOW|WS_VISIBLE,30,40,240,180,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        QTest::qWait(20); NativeDrag(QPoint(80,110),right);
        const bool both=GetParent(first)!=nullptr && GetParent(second)!=nullptr && IsWindow(first) && IsWindow(second);
        const bool videoFilledBefore=SplitViewerNativeVideoFills(first) && SplitViewerNativeVideoFills(second);
        RECT firstBefore={0}; RECT secondBefore={0};
        GetWindowRect(first,&firstBefore); GetWindowRect(second,&secondBefore);
        const bool nativeDoubleClick=NativeDoubleClick(QPoint((firstBefore.left+firstBefore.right)/2,(firstBefore.top+firstBefore.bottom)/2));
        QTest::qWait(120);
        RECT firstFullscreen={0}; RECT secondFullscreen={0};
        GetWindowRect(first,&firstFullscreen); GetWindowRect(second,&secondFullscreen);
        const bool embeddedPaneScaled=firstFullscreen.right-firstFullscreen.left>firstBefore.right-firstBefore.left &&
            secondFullscreen.right-secondFullscreen.left>secondBefore.right-secondBefore.left;
        const bool videoFilledFullscreen=SplitViewerNativeVideoFills(first) && SplitViewerNativeVideoFills(second);
        const bool nativePlayerStayedEmbedded=GetParent(first)!=nullptr && GetParent(second)!=nullptr;
        window.leaveFullscreen();
        QTest::qWait(40);
        const bool videoFilledAfter=SplitViewerNativeVideoFills(first) && SplitViewerNativeVideoFills(second);
        window.close(); QTest::qWait(30);
        check("two native panes restored independently on close",both && IsWindow(first) && IsWindow(second) && GetParent(first)==nullptr && GetParent(second)==nullptr);
        check("embedded panes scale with content fullscreen",nativeDoubleClick && embeddedPaneScaled && nativePlayerStayedEmbedded && videoFilledBefore && videoFilledFullscreen && videoFilledAfter);
        DestroyWindow(first); DestroyWindow(second); SetCursorPos(savedCursor.x,savedCursor.y);
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
        QString playerPath = QDir(QApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../LumaPlayer/LumaPlayer1.0.exe"));
        if (!QFile::exists(playerPath))
        {
            playerPath = QDir(QApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../common/LumaPlayer/LumaPlayer1.0.exe"));
        }
        foreach (const QString& argument, QApplication::arguments())
        {
            if (argument.startsWith(QStringLiteral("--player=")))
            {
                playerPath = argument.mid(9);
            }
        }
        const QString videoPath = QDir(reportDirectory).absoluteFilePath(QStringLiteral("quadrants.avi"));
        const bool fixtureReady = WriteVideoFixture(videoPath);
        QProcess player;
        if (QApplication::arguments().contains(QStringLiteral("--native-trace")))
        {
            QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
            environment.insert(QStringLiteral("QT_LOGGING_RULES"), QStringLiteral("qt.qpa.events=true"));
            environment.insert(QStringLiteral("QT_MESSAGE_PATTERN"), QStringLiteral("%{time yyyy-MM-dd hh:mm:ss.zzz} %{message}"));
            player.setProcessEnvironment(environment);
            player.setStandardErrorFile(reportDirectory + QStringLiteral("/player-qpa.log"));
        }
        player.setWorkingDirectory(QFileInfo(playerPath).absolutePath());
        player.start(playerPath, QStringList() << videoPath);
        const bool started = fixtureReady && player.waitForStarted(5000);
        QPair<DWORD, HWND> found(started ? player.pid()->dwProcessId : 0, nullptr);
        for (int i = 0; started && !found.second && i < 50; ++i)
        {
            QTest::qWait(100);
            EnumWindows(FindPlayerWindow, reinterpret_cast<LPARAM>(&found));
        }
        const HWND hwnd = found.second;
        out << "player=" << playerPath << " pid=" << found.first << " fixture=" << videoPath << "\n";
        QFile binary(playerPath);
        if (binary.open(QIODevice::ReadOnly))
        {
            out << "player_sha256=" << QCryptographicHash::hash(binary.readAll(), QCryptographicHash::Sha256).toHex() << "\n";
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
        QRect pane = ExpectedStage(canvas).adjusted(1,1,-1,-1).toAlignedRect();
        pane.moveTopLeft(canvas->mapToGlobal(pane.topLeft()));
        const POINT sourcePoint = {300,120};
        RECT playerRect = {0};
        GetWindowRect(hwnd, &playerRect);
        const HWND sourceAtPoint = GetAncestor(WindowFromPoint(sourcePoint),GA_ROOT);
        out << "source=" << reinterpret_cast<quintptr>(hwnd) << " hit=" << reinterpret_cast<quintptr>(sourceAtPoint)
            << " rect=" << playerRect.left << "," << playerRect.top << "," << playerRect.right << "," << playerRect.bottom << "\n";
        const bool dragged = hwnd && sourceAtPoint == hwnd && NativeDrag(QPoint(300,120), pane.center());
        SetCursorPos(20, 900);
        QTest::qWait(600);
        ScreenPane(pane).save(reportDirectory + QStringLiteral("/161-embedded.png"));
        check("real player drag shows fitted video pixels", dragged && GetParent(hwnd) && VideoPixelsFill(ScreenPane(pane)) && VideoIsAdvancing(pane));

        const bool doubled = hwnd && GetParent(hwnd) && NativeDoubleClick(pane.center());
        SetCursorPos(20, 900);
        QTest::qWait(600);
        const QRect full = QApplication::desktop()->screenGeometry(&window).adjusted(1,1,-1,-1);
        ScreenPane(full).save(reportDirectory + QStringLiteral("/162-fullscreen.png"));
        check("real player double click scales video and keeps playing", doubled && window.isFullScreen() && GetParent(hwnd) && VideoPixelsFill(ScreenPane(full)) && VideoIsAdvancing(full));

        const bool doubledBack = hwnd && GetParent(hwnd) && NativeDoubleClick(full.center());
        QTest::qWait(300);
        ScreenPane(pane).save(reportDirectory + QStringLiteral("/163-restored.png"));
        check("real player leaves fullscreen with visible video", doubledBack && hwnd && !window.isFullScreen() && GetParent(hwnd) && VideoPixelsFill(ScreenPane(pane)));

        window.setGeometry(760, 150, 1000, 650);
        QTest::qWait(300);
        pane = ExpectedStage(canvas).adjusted(1,1,-1,-1).toAlignedRect();
        pane.moveTopLeft(canvas->mapToGlobal(pane.topLeft()));
        check("real player follows host move and resize", hwnd && VideoPixelsFill(ScreenPane(pane)));

        ChooseMenu(canvas, canvas->rect().center(), QStringLiteral("水平分割"));
        const QRectF splitStage = ExpectedStage(canvas).adjusted(1,1,-1,-1);
        QRect upper(qRound(splitStage.left()), qRound(splitStage.top()), qRound(splitStage.width()), qRound((splitStage.height()-2)/2));
        upper.moveTopLeft(canvas->mapToGlobal(upper.topLeft()));
        Drop(canvas, QPoint(qRound(splitStage.center().x()),qRound(splitStage.top()+splitStage.height()*.75)), bluePath);
        SetCursorPos(20,900);
        QTest::qWait(1300);
        ScreenPane(pane).save(reportDirectory + QStringLiteral("/165-split.png"));
        const QImage splitScreen = ScreenPane(pane);
        check("real player split video and neighbor image stay visible", hwnd && VideoPixelsFill(ScreenPane(upper)) &&
            !splitScreen.isNull() && QColor(splitScreen.pixel(splitScreen.width()/2,splitScreen.height()*3/4)) == blue);

        const bool paused = hwnd && NativePlayerClick(hwnd,upper.center());
        SetCursorPos(20,900);
        QTest::qWait(1300);
        ScreenPane(upper).save(reportDirectory + QStringLiteral("/166-paused.png"));
        const bool stoppedFrames = paused && VideoPixelsFill(ScreenPane(upper)) && !VideoIsAdvancing(upper);
        const bool resumed = hwnd && NativePlayerClick(hwnd,upper.center());
        // Both the host and LumaPlayer defer a click to distinguish double clicks.
        // Observe state until that documented delay plus one full fixture cycle.
        const int resumeBudget = GetDoubleClickTime() + QApplication::doubleClickInterval() + 800;
        QElapsedTimer resumeTimer;
        resumeTimer.start();
        const bool resumedFrames = resumed && VideoIsAdvancing(upper, resumeBudget, &out);
        out << "166 resume budget=" << resumeBudget << " elapsed=" << resumeTimer.elapsed() << "ms\n";
        ScreenPane(upper).save(reportDirectory + QStringLiteral("/166-resume.png"));
        const bool repeatEnter = hwnd && GetParent(hwnd) && NativeDoubleClick(upper.center());
        QTest::qWait(400);
        window.leaveFullscreen();
        QTest::qWait(400);
        SetCursorPos(20,900);
        QTest::qWait(1300);
        ScreenPane(upper).save(reportDirectory + QStringLiteral("/166-final.png"));
        const bool finalPixels = VideoPixelsFill(ScreenPane(upper));
        const bool finalPlaying = VideoIsAdvancing(upper);
        out << "166 details pauseInput=" << paused << " stopped=" << stoppedFrames << " resumeInput=" << resumed
            << " resumed=" << resumedFrames << " doubled=" << repeatEnter << " pixels=" << finalPixels << " playing=" << finalPlaying << "\n";
        check("real player single click and repeated fullscreen retain playback", stoppedFrames && resumedFrames && repeatEnter && hwnd && finalPixels && finalPlaying);

        const bool detached = hwnd && GetParent(hwnd) && NativeDrag(upper.center(),QPoint(380,650));
        QTest::qWait(300);
        check("real player drag out restores live top level", detached && IsWindow(hwnd) && !GetParent(hwnd) && IsWindowVisible(hwnd) && !IsZoomed(hwnd));

        if (hwnd && IsWindow(hwnd))
        {
            SetWindowPos(hwnd, HWND_TOPMOST, 30,100,640,400,SWP_SHOWWINDOW);
            SetCursorPos(300,102);
            QTest::qWait(650);
            if (GetAncestor(WindowFromPoint(sourcePoint),GA_ROOT) == hwnd)
            {
                NativeDrag(QPoint(300,120),upper.center());
            }
        }
        const bool reembedded = hwnd && GetParent(hwnd);
        window.close();
        QTest::qWait(300);
        const bool playerPreserved = reembedded && player.state()==QProcess::Running && IsWindow(hwnd) && !GetParent(hwnd) && IsWindowVisible(hwnd) && !IsZoomed(hwnd);
        if (hwnd && IsWindow(hwnd))
        {
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
        }
        const bool playerClosed = player.waitForFinished(5000);
        if (!playerClosed)
        {
            player.kill();
            player.waitForFinished(2000);
            out << "player cleanup required termination\n";
        }
        check("real player viewer close preserves external process", playerPreserved && playerClosed && player.exitStatus()==QProcess::NormalExit && player.exitCode()==0);
        SetCursorPos(originalCursor.x,originalCursor.y);
    }
#endif
    if (selectedCase == 0 || selectedCase >= 169)
    {
        nextId = 168;
        SplitViewerCoreDocument document;
        document.baseRoot()->makeSplit(SPLITVIEWER_CORE_SPLIT_VERTICAL);
        document.baseRoot()->first->view.path = L"ownership-marker";
        SplitViewerCoreNode* original = document.baseRoot();
        document.setBaseRoot(original);
        check("reassign owned root preserves tree and content", document.baseRoot() == original &&
            !document.baseRoot()->isLeaf() && document.baseRoot()->first->view.path == L"ownership-marker");
        SplitViewerCoreLayer* layer = document.addLayer();
        layer->root->view.path = L"layer-marker";
        document.appendLayer(layer);
        check("append owned layer is idempotent and reset releases once", document.layerCount() == 1 &&
            document.layerAt(0)->root->view.path == L"layer-marker");
        document.reset();
        QWidget foreign;
        foreign.setGeometry(20,20,160,100);
        foreign.show();
        const WId foreignId = foreign.winId();
        const QRect originalGeometry = foreign.geometry();
        check("embed rejects missing host without touching foreign window", SplitViewerEmbedForeignWindow(foreignId, nullptr) == nullptr &&
            foreign.isVisible() && foreign.geometry() == originalGeometry && foreign.parentWidget() == nullptr);
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
    out << "total=" << total << " failures=" << failures << "\n";
    return failures;
}

QStringList SplitViewerAuditCaseNames()
{
    return QStringList()
        << QStringLiteral("101 empty colors and plus")
        << QStringLiteral("102 plus cancel keeps empty")
        << QStringLiteral("103 horizontal context split")
        << QStringLiteral("104 both split plus visible")
        << QStringLiteral("105 upper plus loads visible image")
        << QStringLiteral("106 lower plus loads visible image")
        << QStringLiteral("107 zoom clipped to own split")
        << QStringLiteral("108 save profile reflects real UI")
        << QStringLiteral("109 image pan uses pixels")
        << QStringLiteral("110 nested vertical split")
        << QStringLiteral("111 nested content retained and plus visible")
        << QStringLiteral("112 drop uses actual target")
        << QStringLiteral("113 delete base split leaf")
        << QStringLiteral("114 splitter drag persists")
        << QStringLiteral("115 delete collapse preserves image")
        << QStringLiteral("116 root delete disabled")
        << QStringLiteral("117 bad image retains document")
        << QStringLiteral("118 new layer toolbar")
        << QStringLiteral("119 floating zoom cannot escape layer")
        << QStringLiteral("120 floating pan cannot escape layer")
        << QStringLiteral("121 ctrl drag moves layer only")
        << QStringLiteral("122 layer right edge resize")
        << QStringLiteral("123 layer split")
        << QStringLiteral("124 layer split lower plus")
        << QStringLiteral("125 layer split lower image")
        << QStringLiteral("126 export uses screen-fit stage only")
        << QStringLiteral("127 sv PNG thumbnail bounded and stage only")
        << QStringLiteral("128 drop sv restores layers")
        << QStringLiteral("129 loaded layer image visible")
        << QStringLiteral("130 invalid profile keeps existing layout")
        << QStringLiteral("131 interaction after reload remains valid")
        << QStringLiteral("132 double click margin does not fullscreen")
        << QStringLiteral("133 double click fullscreen content only")
        << QStringLiteral("134 escape restores geometry and toolbar")
        << QStringLiteral("135 F11 enters fullscreen")
        << QStringLiteral("136 F11 restores maximized state")
        << QStringLiteral("137 F1 hides borders in persisted state")
        << QStringLiteral("138 native embed creates child")
        << QStringLiteral("139 native snapshot")
        << QStringLiteral("140 native detach restores surviving original")
        << QStringLiteral("141 legacy fields decoded independently")
        << QStringLiteral("142 cyclic profile rejected without replacing state")
        << QStringLiteral("143 missing node rejected")
        << QStringLiteral("144 odd UTF16 rejected")
        << QStringLiteral("145 unsupported profile version rejected")
        << QStringLiteral("146 corrupt svCf CRC rejected")
        << QStringLiteral("147 old trailing marker accepted")
        << QStringLiteral("148 split geometry exact absolute bounds")
        << QStringLiteral("149 borderless geometry has no separator gap")
        << QStringLiteral("150 wheel notch arithmetic")
        << QStringLiteral("151 ctrl fine wheel arithmetic")
        << QStringLiteral("152 zoom min max bounds")
        << QStringLiteral("153 manual view resize preserves pixel proportions")
        << QStringLiteral("154 native hook drag in to empty leaf")
        << QStringLiteral("155 native drag out restores top level")
        << QStringLiteral("156 closing viewer restores dragged window")
        << QStringLiteral("157 Qt image format exports and unsupported GIF feedback")
        << QStringLiteral("158 zoom percentage popup expires")
        << QStringLiteral("159 two native panes restored independently on close")
        << QStringLiteral("160 embedded panes scale with content fullscreen")
        << QStringLiteral("161 真实播放器拖入后视频像素可见并适配")
        << QStringLiteral("162 双击全屏后视频等比放大且继续播放")
        << QStringLiteral("163 退出全屏后视频保持可见")
        << QStringLiteral("164 移动和调整宿主窗口后视频适配")
        << QStringLiteral("165 视频分屏和相邻图片互不遮挡")
        << QStringLiteral("166 单击暂停继续与重复全屏后播放")
        << QStringLiteral("167 拖出后播放器独立存活")
        << QStringLiteral("168 再嵌入后关闭宿主不销毁播放器")
        << QStringLiteral("169 重复设置已持有根节点保留树与内容")
        << QStringLiteral("170 重复接收已持有图层不重复释放")
        << QStringLiteral("171 嵌入缺失宿主时安全拒绝")
        << QStringLiteral("172 可见控件使用QtControls并保持显示")
        << QStringLiteral("173 真实文件框取消保持空白工作区")
        << QStringLiteral("174 真实文件框确认加载中文空格路径图片")
        << QStringLiteral("175 托管覆盖确认取消重选及确认")
        << QStringLiteral("176 关于按钮使用DialogManager提示框")
        << QStringLiteral("177 托管输入框接受及取消")
        ;
}