#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayer.h"
#include "LumaPlayerMicrophoneRetireTask.h"
#include "LumaPlayerKaraokeProgressView.h"
#include "LumaPlayerMicrophoneView.h"
#include <QPainterPath>
#include <QScopedValueRollback>
#include <QCoreApplication>
#include <QFileInfo>
#include "Config.h"
#include "CStringManager/CStringManagerAPI.h"
#include "LumaPlayerHelper.h"
#include "LumaPlayerHelpView.h"
#include "DialogManager/DialogManagerAPI.h"

#include "LogManager/LogManagerAPI.h"

#include <QAction>
#include <QCloseEvent>
#include <QApplication>
#include <QCursor>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QKeyEvent>
#include "QtControls/Menu.h"
#include <QFontMetrics>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QToolTip>
#include <QWheelEvent>
#include <QWindow>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdint.h>
#include <limits>
#include <vector>

LumaPlayer::LumaPlayer(bool debugEnabled, QWidget* parent) :
Widget(parent),
m_volumePercent(100),
m_appliedVolumePercent(100),
m_restoreVolumePercent(100),
m_volumeRequest(0),
m_volumePopupVisible(false),
m_dragVolume(false),
m_volumeMutePressed(false),
m_volumeMuted(false),
m_dismissVolumeClick(false),
m_microphonePopupVisible(false),
m_microphoneDeviceMenuActive(false),
m_dismissMicrophoneClick(false),
m_dragMicrophoneGain(false),
m_microphoneMonitoring(true),
m_karaokeRecording(false),
m_karaokeRecordingReady(false),
m_karaokeStartInput(0),
m_karaokeTimelineInitialized(false),
m_karaokeSessionStarted(false),
m_karaokeFinishing(false),
m_vocalRemovalEnabled(false),
m_karaokeBacking(nullptr),
m_karaokePitchSemitones(0),
m_audioRender(),
m_videoRender(),
m_core(&m_audioRender, &m_videoRender),
m_logic(&m_core),
m_selectedMicrophone(0),
m_pinned(false),
m_resetEnabled(false),
m_closeReady(false),
m_closeRequested(false),
m_viewportGeneration(0),
m_viewRevision(0),
m_nextInputSerial(0),
m_lastSeekInput(0),
m_lastMoveInput(0),
m_mediaGeneration(0),
m_lastCenterTipVisible(false),
m_lastZoomTipVisible(false),
m_snapshot(),
m_scaledFrameSourceKey(0),
m_cachedFrameSerial(0),
m_cachedFrameStart(0),
m_cachedFrameEnd(0),
m_leftPressed(false),
m_hasMedia(false),
m_pendingMediaLoad(false),
m_pendingMediaSerial(0),
m_mouseInside(false),
m_topTargetVisible(false),
m_bottomTargetVisible(false),
m_topVisibleHeight(0),
m_bottomVisibleHeight(0),
m_lastMouseMoveMs(0),
m_lastTopOverlayActiveMs(0),
m_lastBottomOverlayActiveMs(0),
m_lastMousePos(0, 0),
m_zoomTipHideMs(0),
m_zoomPercent(100),
m_rateTipPermille(1000),
m_rateTipHideMs(0),
m_baseDisplayScale(1.0),
m_panOffset(0.0, 0.0),
m_pressGlobalPos(0, 0),
m_pressWindowGeometry(),
m_pressPanOffset(0.0, 0.0),
m_dragWindow(false),
m_dragVideo(false),
m_isDraggingProgress(false),
m_progressPressPending(false),
m_resizeWindow(false),
m_resizeEdge(ResizeNone),
m_progressWasPlaying(false),
m_dragPosition100ns(0),
m_hasDragPosition(false),
m_lastPreviewRequestMs(0),
m_lastPreviewRequestPosition100ns(-1),
m_progressMenuActive(false),
m_fileDialogActive(false),
m_maximizedOutsideFullScreen(false),
m_windowStateSerial(0),
m_hoverArea(HitNone),
m_pressArea(HitNone),
m_hoverLoopPoint(-1),
m_lastActiveLoopPoint(-1),
m_loopMoveKey(0),
m_loopMoveRepeating(false),
m_pendingLoopMovePoint(-1),
m_cancelClickToggle(false),
m_normalWindowGeometry()
{
    init(debugEnabled);
}

LumaPlayer::~LumaPlayer()
{
    qApp->removeEventFilter(this);
    m_uiTimer.stop();
    m_clickTimer.stop();
    stopKaraokeForShutdown();
    LOGINFO("LumaPlayer destroyed");
}

void LumaPlayer::init(bool debugEnabled)
{
    m_microphones.push_back(std::shared_ptr<LumaPlayerMicrophoneChannel>(new LumaPlayerMicrophoneChannel(&m_core, 0)));
    m_volumePercent = g_config.m_volumeNormalPercent;
    m_appliedVolumePercent = m_volumePercent;
    m_restoreVolumePercent = m_volumePercent;
    m_audioRender.setVolumePercent(m_volumePercent);
    qApp->installEventFilter(this);
    LOGINFO("LumaPlayer init begin, debug=%d", debugEnabled ? 1 : 0);
    setWindowTitle(g_config.m_windowTitle);
    setWindowIcon(QIcon(g_config.m_applicationIconPath));
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(g_config.m_minWindowWidth, g_config.m_minWindowHeight);
    setFont(QFont(g_config.m_fontFamily, g_config.m_fontSize));

    m_elapsedTimer.start();
    m_lastMouseMoveMs = m_elapsedTimer.elapsed();
    QObject::connect(&m_logic, &LumaPlayerLogicController::resultReady,
        this, &LumaPlayer::onLogicResult, Qt::QueuedConnection);
    if (!m_logic.init())
    {
        LOGERROR("Logic initialization submission failed");
        m_closeReady = true;
    }
    fitInitialWindowToDesktop();

    m_clickTimer.setSingleShot(true);
    connect(&m_clickTimer, SIGNAL(timeout()), this, SLOT(onVideoClick()));
    connect(&m_uiTimer, SIGNAL(timeout()), this, SLOT(onUiTimer()));
    m_uiTimer.start(g_config.m_uiRefreshMs);
    LOGINFO("LumaPlayer init end");
}

void LumaPlayer::paintEvent(QPaintEvent* event)
{
    (void)event;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // 图形抗锯齿不处理图像采样，视频缩放需单独启用平滑变换
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(rect(), g_config.m_videoBackgroundColor);
    painter.save();
    painter.setClipRect(videoViewportRect());
    paintVideo(painter);
    paintCenterPlayTip(painter);
    painter.restore();
    paintTopOverlay(painter);
    paintBottomOverlay(painter);
    paintVolume(painter, true);
    paintZoomTip(painter);
}

void LumaPlayer::resizeEvent(QResizeEvent* event)
{
    LumaPlayerLogicAction viewport;
    viewport.m_type = LumaActionViewport;
    viewport.m_viewportGeneration = ++m_viewportGeneration;
    m_logic.submit(viewport);
    Widget::resizeEvent(event);
    updateLoopMarkerHover(m_lastMousePos);
}

void LumaPlayer::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dismissMicrophoneClick)
    {
        event->accept();
        return;
    }
    if (m_dismissVolumeClick)
    {
        event->accept();
        return;
    }
    m_mouseInside = true;
    m_lastMouseMoveMs = m_elapsedTimer.elapsed();
    m_lastMousePos = event->pos();
    if (m_dragMicrophoneGain)
    {
        setMicrophoneGainFromPoint(event->pos());
        event->accept();
        return;
    }
    if (m_volumeMutePressed)
    {
        update();
        event->accept();
        return;
    }
    if (m_dragVolume)
    {
        volumeFromPoint(event->pos());
        showVolumeTooltip(event->pos());
        event->accept();
        return;
    }
    if ((m_volumePopupVisible && volumePopupRect().contains(event->pos())) ||
        (m_bottomVisibleHeight > 0 && volumeButtonRect().contains(event->pos())))
    {
        m_hoverLoopPoint = -1;
        m_hoverArea = hitTest(event->pos());
        showVolumeTooltip(event->pos());
        setCursor(Qt::ArrowCursor);
        updateOverlayTargets();
        update();
        return;
    }
    if ((m_microphonePopupVisible && microphonePopupRect().contains(event->pos())) ||
        (m_bottomVisibleHeight > 0 && microphoneButtonRect().contains(event->pos())))
    {
        m_hoverLoopPoint = -1;
        m_hoverArea = hitTest(event->pos());
        if (m_microphonePopupVisible && microphoneGainTrackRect().adjusted(-10, -8, 10, 8).contains(event->pos()))
        {
            QToolTip::showText(mapToGlobal(event->pos()),
                QStringLiteral("麦克风增益 %1%").arg(currentMicrophone().m_gain), this);
        }
        else if (m_hoverArea == HitMicrophoneButton)
        {
            QToolTip::showText(mapToGlobal(event->pos()), QStringLiteral("麦克风控制"), this);
        }
        else
        {
            QToolTip::hideText();
        }
        setCursor(Qt::ArrowCursor);
        updateOverlayTargets();
        update();
        return;
    }
    if (m_resizeWindow)
    {
        setGeometry(resizeGeometryFromMouse(event->globalPos()));
        m_cancelClickToggle = true;
        return;
    }
    if (m_dragWindow)
    {
        move(m_pressWindowGeometry.topLeft() + event->globalPos() - m_pressGlobalPos);
        m_cancelClickToggle = true;
        return;
    }
    if (m_progressPressPending && (event->buttons() & Qt::LeftButton) &&
        (event->pos() - mapFromGlobal(m_pressGlobalPos)).manhattanLength() >= QApplication::startDragDistance())
    {
        m_progressPressPending = false;
        m_isDraggingProgress = true;
        m_lastPreviewRequestMs = 0;
        m_lastPreviewRequestPosition100ns = -1;
        postAction(LumaActionDragBegin, m_dragPosition100ns);
        previewByProgressPoint(event->pos());
        m_cancelClickToggle = true;
        return;
    }
    if (m_isDraggingProgress)
    {
        previewByProgressPoint(event->pos());
        m_cancelClickToggle = true;
        return;
    }
    if (m_dragVideo)
    {
        if (m_bottomVisibleHeight > 0 && bottomOverlayRect().contains(event->pos()))
        {
            m_pressGlobalPos = event->globalPos();
            m_pressPanOffset = m_panOffset;
            m_cancelClickToggle = true;
            return;
        }
        if ((event->globalPos() - m_pressGlobalPos).manhattanLength() < QApplication::startDragDistance() && !m_cancelClickToggle)
        {
            return;
        }
        m_clickTimer.stop();
        QPoint pressLocalPos = mapFromGlobal(m_pressGlobalPos);
        QPoint delta = event->pos() - pressLocalPos;
        LumaPlayerLogicAction pan;
        pan.m_type = LumaActionPan;
        pan.m_position = m_pressPanOffset + QPointF(delta);
        pan.m_viewportGeneration = m_viewportGeneration;
        m_logic.submit(pan);
        m_cancelClickToggle = true;
        update();
        return;
    }
    m_hoverArea = hitTest(event->pos());
    updateLoopMarkerHover(event->pos());
    QString tooltip;
    if (m_hoverArea == HitLoadButton)
    {
        tooltip = g_config.m_loadText;
    }
    else if (m_hoverArea == HitResetButton)
    {
        tooltip = g_config.m_resetText;
    }
    else if (m_hoverArea == HitPinButton)
    {
        tooltip = m_pinned ? g_config.m_unpinText : g_config.m_pinText;
    }
    else if (m_hoverArea == HitHelpButton)
    {
        tooltip = g_config.m_helpTooltip;
    }
    if (tooltip.isEmpty())
    {
        QToolTip::hideText();
    }
    else
    {
        QToolTip::showText(mapToGlobal(event->pos()), tooltip, this);
    }
    updateCursorShape(event->pos());
    updateOverlayTargets();
    update();
}

void LumaPlayer::mousePressEvent(QMouseEvent* event)
{
    m_dismissVolumeClick = false;
    m_dismissMicrophoneClick = false;
    if (m_microphonePopupVisible && !microphonePopupRect().contains(event->pos()) &&
        !microphoneButtonRect().contains(event->pos()))
    {
        const HitArea target = hitTest(event->pos());
        m_microphonePopupVisible = false;
        m_dragMicrophoneGain = false;
        if (target == HitNone || target == HitTopTitle)
        {
            m_dismissMicrophoneClick = true;
            update();
            event->accept();
            return;
        }
        update();
    }
    if (m_microphonePopupVisible && microphonePopupRect().contains(event->pos()))
    {
        if (event->button() == Qt::LeftButton)
        {
            if (microphoneGainTrackRect().adjusted(-10, -8, 10, 8).contains(event->pos()))
            {
                m_dragMicrophoneGain = true;
                setMicrophoneGainFromPoint(event->pos());
            }
            else
            {
                handleMicrophonePopupClick(event->pos());
            }
        }
        event->accept();
        return;
    }
    if (m_volumePopupVisible && !volumePopupRect().contains(event->pos()) &&
        !volumeButtonRect().contains(event->pos()))
    {
        const HitArea target = hitTest(event->pos());
        hideVolumePopup();
        m_clickTimer.stop();
        if (target == HitNone || target == HitTopTitle)
        {
            m_dismissVolumeClick = true;
            event->accept();
            return;
        }
    }
    if (m_volumePopupVisible && volumePopupRect().contains(event->pos()))
    {
        m_clickTimer.stop();
        if (event->button() == Qt::LeftButton)
        {
            if (volumeMuteButtonRect().contains(event->pos()))
            {
                m_volumeMutePressed = true;
                update();
            }
            else if (event->pos().y() < volumePopupRect().bottom() - g_config.m_volumeFooterHeight + 1)
            {
                m_dragVolume = true;
                volumeFromPoint(event->pos());
            }
        }
        event->accept();
        return;
    }
    m_leftPressed = event->button() == Qt::LeftButton;
    update();
    setFocus(Qt::MouseFocusReason);
    m_mouseInside = true;
    m_lastMouseMoveMs = m_elapsedTimer.elapsed();
    m_lastMousePos = event->pos();
    m_cancelClickToggle = false;
    m_pressArea = hitTest(event->pos());
    if (m_pressArea != HitVideoArea)
    {
        m_clickTimer.stop();
    }
    m_pressGlobalPos = event->globalPos();
    m_pressWindowGeometry = geometry();
    m_pressPanOffset = m_panOffset;
    if (event->button() == Qt::RightButton && m_hasMedia && m_bottomVisibleHeight > 0 && progressTrackRect().adjusted(0, -g_config.m_trackHitExtra, 0, g_config.m_trackHitExtra).contains(event->pos()))
    {
        showProgressMenu(event->pos());
        return;
    }
    if (event->button() != Qt::LeftButton)
    {
        return;
    }
    m_resizeEdge = hitTestResizeEdge(event->pos());
    if (!isFullScreen() && !isMaximized() && m_resizeEdge != ResizeNone)
    {
        m_resizeWindow = true;
        return;
    }
    if (m_pressArea == HitTopTitle && !isFullScreen() && !isMaximized())
    {
        m_dragWindow = true;
        m_topTargetVisible = true;
        m_topVisibleHeight = topOverlayHeight();
        return;
    }
    if (m_pressArea == HitProgressTrack)
    {
        m_progressPressPending = true;
        m_isDraggingProgress = false;
        m_hasDragPosition = true;
        m_dragPosition100ns = progressPointToTime100ns(event->pos());
        m_lastPreviewRequestMs = 0;
        m_lastPreviewRequestPosition100ns = -1;
        m_progressWasPlaying = LumaPlayerHelper::isPlayingState(m_snapshot.m_state);
        update();
        return;
    }
    if (m_pressArea == HitVideoArea && m_hasMedia)
    {
        m_dragVideo = true;
        return;
    }
}

void LumaPlayer::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_dragMicrophoneGain && event->button() == Qt::LeftButton)
    {
        setMicrophoneGainFromPoint(event->pos());
        m_dragMicrophoneGain = false;
        event->accept();
        return;
    }
    if (m_dismissMicrophoneClick)
    {
        m_dismissMicrophoneClick = false;
        event->accept();
        return;
    }
    if (m_microphonePopupVisible && microphonePopupRect().contains(event->pos()))
    {
        event->accept();
        return;
    }
    if (m_volumeMutePressed && event->button() == Qt::LeftButton)
    {
        m_volumeMutePressed = false;
        if (m_volumePopupVisible && volumeMuteButtonRect().contains(event->pos()))
        {
            if (!m_volumeMuted)
            {
                m_restoreVolumePercent = m_volumePercent;
            }
            m_volumeMuted = !m_volumeMuted;
            requestVolume(m_volumeMuted ? 0 : m_restoreVolumePercent, true);
            showVolumeTooltip(event->pos());
        }
        update();
        event->accept();
        return;
    }
    if (m_dismissVolumeClick)
    {
        m_dismissVolumeClick = false;
        event->accept();
        return;
    }
    if (m_dragVolume && event->button() == Qt::LeftButton)
    {
        volumeFromPoint(event->pos());
        m_dragVolume = false;
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton)
    {
        m_leftPressed = false;
    }
    update();
    m_lastMousePos = event->pos();
    if (event->button() != Qt::LeftButton)
    {
        return;
    }
    if (m_resizeWindow)
    {
        m_resizeWindow = false;
        m_resizeEdge = ResizeNone;
        updateCursorShape(event->pos());
        return;
    }
    if (m_dragWindow)
    {
        m_dragWindow = false;
        return;
    }
    if (m_progressPressPending)
    {
        m_progressPressPending = false;
        commitSeekByProgressPoint(event->pos(), m_progressWasPlaying);
        m_progressWasPlaying = false;
        return;
    }
    if (m_isDraggingProgress)
    {
        commitSeekByProgressPoint(event->pos(), m_progressWasPlaying);
        m_isDraggingProgress = false;
        m_progressWasPlaying = false;
        return;
    }
    if (m_dragVideo)
    {
        m_dragVideo = false;
        if (m_cancelClickToggle)
        {
            return;
        }
    }
    if (event->button() != Qt::LeftButton || m_cancelClickToggle)
    {
        return;
    }
    HitArea releaseArea = hitTest(event->pos());
    if (m_pressArea == HitLoadButton && releaseArea == HitLoadButton)
    {
        openFileDialog();
    }
    else if (m_pressArea == HitResetButton && releaseArea == HitResetButton && m_resetEnabled)
    {
        postAction(LumaActionReset);
    }
    else if (m_pressArea == HitPinButton && releaseArea == HitPinButton)
    {
        postAction(LumaActionPin);
    }
    else if (m_pressArea == HitHelpButton && releaseArea == HitHelpButton)
    {
        postAction(LumaActionHelp);
    }
    else if (m_pressArea == HitMinButton && releaseArea == HitMinButton)
    {
        postAction(LumaActionMinimize);
    }
    else if (m_pressArea == HitMaxButton && releaseArea == HitMaxButton)
    {
        toggleMaximize();
    }
    else if (m_pressArea == HitCloseButton && releaseArea == HitCloseButton)
    {
        postAction(LumaActionShutdown);
    }
    else if (m_pressArea == HitPlusButton && releaseArea == HitPlusButton)
    {
        openFileDialog();
    }
    else if (m_pressArea == HitVolumeButton && releaseArea == HitVolumeButton)
    {
        m_volumePopupVisible = !m_volumePopupVisible;
        m_bottomVisibleHeight = bottomOverlayHeight();
        m_hoverLoopPoint = -1;
        updateOverlayTargets();
        update();
    }
    else if (m_pressArea == HitMicrophoneButton && releaseArea == HitMicrophoneButton)
    {
        m_volumePopupVisible = false;
        m_microphonePopupVisible = !m_microphonePopupVisible;
        m_bottomVisibleHeight = bottomOverlayHeight();
        m_hoverLoopPoint = -1;
        if (m_microphonePopupVisible && !currentMicrophone().m_capture.isRunning())
        {
            currentMicrophone().m_status = QStringLiteral("麦克风已关闭");
            const QStringList devices = LumaPlayerMicrophoneCapture::availableDeviceNames();
            currentMicrophone().m_label = devices.value(currentMicrophone().m_device,
                devices.isEmpty() ? QStringLiteral("未检测到麦克风") : QStringLiteral("选择输入设备"));
        }
        updateOverlayTargets();
        update();
    }
    else if (m_pressArea == HitPlayButton && releaseArea == HitPlayButton)
    {
        togglePlayPause();
    }
    else if (m_pressArea == HitVideoArea && releaseArea == HitVideoArea)
    {
        m_clickTimer.start(QApplication::doubleClickInterval());
    }
}

void LumaPlayer::mouseDoubleClickEvent(QMouseEvent* event)
{
    if ((m_microphonePopupVisible && microphonePopupRect().contains(event->pos())) ||
        (m_bottomVisibleHeight > 0 && microphoneButtonRect().contains(event->pos())))
    {
        mousePressEvent(event);
        return;
    }
    if ((m_volumePopupVisible && volumePopupRect().contains(event->pos())) ||
        (m_bottomVisibleHeight > 0 && volumeButtonRect().contains(event->pos())))
    {
        // Qt以双击事件替代第二次按下，控件仍应在对应释放时执行一次。
        mousePressEvent(event);
        return;
    }
    if (m_dismissVolumeClick)
    {
        event->accept();
        return;
    }
    m_clickTimer.stop();
    if (event->button() == Qt::LeftButton && hitTest(event->pos()) == HitVideoArea)
    {
        m_dragVideo = false;
        m_cancelClickToggle = true;
        toggleFullScreen();
        event->accept();
    }
}

void LumaPlayer::onVideoClick()
{
    if (m_hasMedia && !modalBlocksInput() && !m_closeRequested)
    {
        togglePlayPause();
    }
}

void LumaPlayer::wheelEvent(QWheelEvent* event)
{
    if (m_microphonePopupVisible && microphonePopupRect().contains(event->pos()))
    {
        event->accept();
        return;
    }
    if ((m_volumePopupVisible && volumePopupRect().contains(event->pos())) ||
        (m_bottomVisibleHeight > 0 && volumeButtonRect().contains(event->pos())))
    {
        event->accept();
        return;
    }
    if (!m_hasMedia)
    {
        return;
    }
    int delta = event->angleDelta().y();
    if (delta == 0)
    {
        return;
    }
    int step = (QApplication::keyboardModifiers() & Qt::ControlModifier) ? g_config.m_zoomFineStep : g_config.m_zoomStep;
    int newZoom = m_zoomPercent + (delta > 0 ? step : -step);
    setZoomPercent(newZoom, event->pos());
}

void LumaPlayer::keyPressEvent(QKeyEvent* event)
{
    m_clickTimer.stop();
    const QPoint pointer = mapFromGlobal(QCursor::pos());
    const bool overVolume = m_volumePopupVisible && volumePopupRect().contains(pointer);
    const bool overMicrophone = m_microphonePopupVisible && microphonePopupRect().contains(pointer);
    const bool overTop = m_topVisibleHeight > 0 && topOverlayRect().contains(pointer);
    const bool horizontal = event->key() == Qt::Key_Left || event->key() == Qt::Key_Right;
    const bool vertical = event->key() == Qt::Key_Up || event->key() == Qt::Key_Down;
    if ((overVolume || overMicrophone || overTop) && (horizontal || (vertical && (event->modifiers() & Qt::ControlModifier))))
    {
        cancelDeferredInput();
        event->accept();
        return;
    }
    if (overVolume && vertical)
    {
        if (event->modifiers() == Qt::NoModifier)
        {
            requestVolume(m_volumePercent + (event->key() == Qt::Key_Up ? g_config.m_volumeStep : -g_config.m_volumeStep));
            showVolumeTooltip(pointer);
        }
        event->accept();
        return;
    }
    if (overMicrophone && vertical && event->modifiers() == Qt::NoModifier)
    {
        currentMicrophone().m_gain = (std::max)(0, (std::min)(g_config.m_microphoneMaxGainPercent, currentMicrophone().m_gain + (event->key() == Qt::Key_Up ? g_config.m_microphoneGainStep : -g_config.m_microphoneGainStep)));
        setKaraokeControls();
        QToolTip::showText(mapToGlobal(pointer),
            QStringLiteral("麦克风增益 %1%").arg(currentMicrophone().m_gain), this);
        update();
        event->accept();
        return;
    }
    if (m_volumePopupVisible && event->key() == Qt::Key_Escape)
    {
        hideVolumePopup();
        event->accept();
        return;
    }
    if (m_microphonePopupVisible && event->key() == Qt::Key_Escape)
    {
        m_microphonePopupVisible = false;
        m_dragMicrophoneGain = false;
        updateOverlayTargets();
        update();
        event->accept();
        return;
    }
    updateLoopMarkerHover(pointer);
    if (event->key() == Qt::Key_Escape && isFullScreen())
    {
        if (!event->isAutoRepeat())
        {
            postAction(LumaActionExitFullscreen);
        }
        event->accept();
        return;
    }
    if ((event->modifiers() & Qt::ControlModifier) != 0)
    {
        if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
        {
            if (m_karaokeRecording)
            {
                m_karaokeNotice = QStringLiteral("录制期间倍速已锁定");
                event->accept();
                return;
            }
            postAction(LumaActionRateDelta, event->key() == Qt::Key_Up ? g_config.m_rateStep : -g_config.m_rateStep);
            event->accept();
            return;
        }
    }
    if (m_hoverLoopPoint >= 0 && (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right))
    {
        if (m_loopMoveKey != event->key() || (m_pendingLoopMovePoint >= 0 && m_pendingLoopMovePoint != m_hoverLoopPoint))
        {
            postAction(LumaActionCancelMove);
            m_pendingLoopMovePoint = -1;
            m_loopMoveKey = event->key();
            m_loopMoveRepeating = false;
        }
        if (event->isAutoRepeat())
        {
            m_loopMoveRepeating = true;
        }
        if (m_pendingLoopMovePoint < 0)
        {
            moveHoveredLoopPoint(event->key() == Qt::Key_Left ? -1 : 1);
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
    {
        postAction(LumaActionJump, event->key() == Qt::Key_Left ? -1 : 1);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Space)
    {
        if (!event->isAutoRepeat())
        {
            togglePlayPause();
        }
        event->accept();
        return;
    }
    Widget::keyPressEvent(event);
}

void LumaPlayer::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == m_loopMoveKey)
    {
        if (!event->isAutoRepeat())
        {
            if (m_loopMoveRepeating)
            {
                postAction(LumaActionCancelMove);
                m_pendingLoopMovePoint = -1;
            }
            m_loopMoveKey = 0;
            m_loopMoveRepeating = false;
        }
        event->accept();
        return;
    }
    Widget::keyReleaseEvent(event);
}

void LumaPlayer::focusOutEvent(QFocusEvent* event)
{
    hideVolumePopup();
    m_dismissVolumeClick = false;
    QToolTip::hideText();
    cancelDeferredInput();
    Widget::focusOutEvent(event);
}

void LumaPlayer::enterEvent(QEvent* event)
{
    (void)event;
    m_mouseInside = true;
    m_lastMouseMoveMs = m_elapsedTimer.elapsed();
    updateOverlayTargets();
}

void LumaPlayer::leaveEvent(QEvent* event)
{
    QToolTip::hideText();
    (void)event;
    m_mouseInside = false;
    m_lastMouseMoveMs = m_elapsedTimer.elapsed();
}

void LumaPlayer::onUiTimer()
{
    syncMicrophonePanel();
    if (m_isDraggingProgress)
    {
        previewByProgressPoint(m_lastMousePos);
    }
    const LumaPlayerCoreCSnapshot previous = m_snapshot;
    const uint64_t previousFrame = m_cachedFrameSerial;
    const bool previousDragPosition = m_hasDragPosition;
    const int32_t previousHover = m_hoverLoopPoint;
    const QString previousMicrophoneStatus = currentMicrophone().m_status;
    postAction(LumaActionPoll);
    if (m_hasMedia && !m_karaokeTimelineInitialized)
    {
        for (size_t i = 0; i < m_microphones.size(); ++i)
        {
            if (m_microphones[i]->m_capture.isRunning())
            {
                m_karaokeTimelineInitialized = m_core.initKaraokeRecording(48000, 1, 0) == 0;
                break;
            }
        }
    }
    if (m_karaokeRecording && m_snapshot.m_state == LumaPlayerCoreCStateEnded)
    {
        m_karaokeRecording = false;
        setKaraokeControls();
        finishKaraokeSession();
    }
    for (size_t i = 0; i < m_microphones.size(); ++i)
    {
        LumaPlayerMicrophoneChannel& mic = *m_microphones[i];
        if (mic.m_initialized && !mic.m_capture.isRunning() && mic.m_task.lastError() != 0 && !mic.m_muted)
        {
            mic.m_muted = true;
            mic.m_status = QStringLiteral("麦%1设备不可用，请更换输入设备").arg(i + 1);
            setKaraokeControls();
        }
        if (m_karaokeRecording && (mic.m_capture.hasOverflowed() || mic.m_task.lastError() != 0))
        {
            m_karaokeRecording = false;
            m_karaokeNotice = QStringLiteral("麦%1录音异常，已保留已有录音").arg(i + 1);
            setKaraokeControls();
            finishKaraokeSession();
            break;
        }
    }
    if (m_microphonePopupVisible && (currentMicrophone().m_capture.isRunning() || !currentMicrophone().m_capture.lastError().isEmpty()))
    {
        const int32_t pipelineError = currentMicrophone().m_task.lastError();
        const QString captureError = currentMicrophone().m_capture.lastError();
        if (!captureError.isEmpty())
        {
            currentMicrophone().m_status = captureError;
        }
        else if (pipelineError == -2)
        {
            currentMicrophone().m_status = QStringLiteral("默认扬声器不支持监听格式");
        }
        else if (pipelineError == -3)
        {
            currentMicrophone().m_status = QStringLiteral("无法创建扬声器监听设备");
        }
        else if (pipelineError == -4)
        {
            currentMicrophone().m_status = QStringLiteral("扬声器监听启动失败");
        }
        else if (pipelineError == -5)
        {
            currentMicrophone().m_status = QStringLiteral("麦克风 PCM 格式异常");
        }
        else if (pipelineError == -6)
        {
            currentMicrophone().m_status = QStringLiteral("扬声器监听写入失败或延迟过高");
        }
        else if (pipelineError == -7)
        {
            currentMicrophone().m_status = QStringLiteral("麦克风采集队列溢出");
        }
        else if (pipelineError == -8)
        {
            currentMicrophone().m_status = QStringLiteral("未收到麦克风数据，请检查系统隐私权限和输入电平");
        }
        else if (pipelineError == -9)
        {
            currentMicrophone().m_status = QStringLiteral("麦克风与扬声器格式转换失败");
        }
        else if (pipelineError != 0)
        {
            currentMicrophone().m_status = QStringLiteral("麦克风音频处理失败 (%1)").arg(pipelineError);
        }
        else if (!m_karaokeRecording)
        {
            if (currentMicrophone().m_muted)
            {
                currentMicrophone().m_status = QStringLiteral("麦克风已关闭");
            }
            else if (!currentMicrophone().m_task.hasReceivedPcm())
            {
                currentMicrophone().m_status = QStringLiteral("等待麦克风输入");
            }
            else
            {
                const int32_t level = (std::min)(100,
                    currentMicrophone().m_task.lastInputPeak() * 100 / 32767);
                currentMicrophone().m_status = QStringLiteral("输入电平 %1% · 监听%2")
                                                   .arg(level)
                                                   .arg(m_microphoneMonitoring ? QStringLiteral("开启") : QStringLiteral("关闭"));
            }
        }
    }
    uint64_t frameSerial = m_videoRender.frameSerial();
    if (frameSerial != m_cachedFrameSerial)
    {
        QImage frame;
        int64_t frameStart = 0;
        int64_t frameEnd = 0;
        m_videoRender.copyFrame(&frame, &frameStart, &frameEnd, &frameSerial);
        m_cachedFrameStart = frameStart;
        m_cachedFrameEnd = frameEnd;
        m_cachedFrame = frame;
        m_cachedFrameSerial = frameSerial;
    }
    updateOverlayTargets();
    updateOverlayAnimation();
    updateLoopMarkerHover(m_lastMousePos);
    const int64_t nowMs = m_elapsedTimer.elapsed();
    const bool centerTip = m_hasMedia && !LumaPlayerHelper::isPlayingState(m_snapshot.m_state) &&
        nowMs - m_lastMouseMoveMs <= g_config.m_mouseIdleHideMs;
    const bool zoomTip = nowMs < m_zoomTipHideMs || nowMs < m_rateTipHideMs;
    const bool changed = previous.m_state != m_snapshot.m_state || previous.m_position100ns != m_snapshot.m_position100ns ||
        previous.m_duration100ns != m_snapshot.m_duration100ns || previous.m_ratePermille != m_snapshot.m_ratePermille ||
        previous.m_hasLoopA != m_snapshot.m_hasLoopA || previous.m_hasLoopB != m_snapshot.m_hasLoopB ||
        previous.m_loopAStart100ns != m_snapshot.m_loopAStart100ns || previous.m_loopBEnd100ns != m_snapshot.m_loopBEnd100ns ||
        previousFrame != m_cachedFrameSerial || previousDragPosition != m_hasDragPosition || previousHover != m_hoverLoopPoint ||
        centerTip != m_lastCenterTipVisible || zoomTip != m_lastZoomTipVisible ||
        previousMicrophoneStatus != currentMicrophone().m_status;
    m_lastCenterTipVisible = centerTip;
    m_lastZoomTipVisible = zoomTip;
    if (changed)
    {
        update();
    }
}

void LumaPlayer::openFileDialog()
{
    hideVolumePopup();
    cancelDeferredInput();
    if (m_fileDialogActive)
    {
        return;
    }
    m_fileDialogActive = true;
    updateOverlayTargets();
    LOGINFO("Open file dialog begin");
    QString filePath = QFileDialog::getOpenFileName(this, g_config.m_loadText, QString(), g_config.m_videoFileFilter);
    m_fileDialogActive = false;
    updateOverlayTargets();
    if (filePath.isEmpty())
    {
        LOGINFO("Open file dialog canceled");
        return;
    }
    loadMedia(filePath);
}

void LumaPlayer::loadMedia(const QString& filePath)
{
    if (filePath.isEmpty() || m_closeRequested)
    {
        return;
    }
    if ((m_karaokeRecording || m_karaokeSessionStarted) && !finishKaraokeSession())
    {
        return;
    }
    hideVolumePopup();
    m_microphonePopupVisible = false;
    cancelDeferredInput(false);
    m_hasDragPosition = false;
    if (m_karaokeTimelineInitialized)
    {
        m_karaokeRecording = false;
        m_karaokeSessionStarted = false;
        m_core.clearKaraokeRecording();
        m_karaokeTimelineInitialized = false;
        setKaraokeControls();
    }
    LumaPlayerLogicAction action;
    action.m_type = LumaActionLoad;
    action.m_path = filePath;
    lumaPlayerCoreBackingDestroy(m_karaokeBacking);
    m_karaokeBacking = nullptr;
    m_vocalRemovalEnabled = false;
    m_karaokeSource = filePath;
    m_logic.submit(action);
}

void LumaPlayer::fitWindowToMedia()
{
    QSize videoSize = m_videoRender.videoSize();
    if (!videoSize.isValid())
    {
        videoSize = QSize(m_snapshot.m_videoWidth, m_snapshot.m_videoHeight);
    }
    if (!videoSize.isValid())
    {
        fitInitialWindowToDesktop();
        return;
    }
    QDesktopWidget* desktop = QApplication::desktop();
    QRect availableRect = desktop != nullptr ? desktop->screenGeometry(this) : QRect(QPoint(0, 0), g_config.m_fallbackDesktopSize);
    const int32_t maxVideoWidth = (std::max)(1, static_cast<int32_t>(availableRect.width() * g_config.m_initialDesktopFraction));
    const int32_t maxVideoHeight = (std::max)(1, static_cast<int32_t>(availableRect.height() * g_config.m_initialDesktopFraction));
    double scale = 1.0;
    if (videoSize.width() > maxVideoWidth || videoSize.height() > maxVideoHeight)
    {
        const double widthScale = static_cast<double>(maxVideoWidth) / videoSize.width();
        const double heightScale = static_cast<double>(maxVideoHeight) / videoSize.height();
        scale = (std::min)(widthScale, heightScale);
    }
    if (scale <= 0.0)
    {
        scale = 1.0;
    }
    m_baseDisplayScale = scale;
    QSize targetSize((std::max)(g_config.m_minWindowWidth, static_cast<int>(videoSize.width() * scale + 0.5)), (std::max)(g_config.m_minWindowHeight, static_cast<int>(videoSize.height() * scale + 0.5)));
    QPoint topLeft(availableRect.left() + (availableRect.width() - targetSize.width()) / 2, availableRect.top() + (availableRect.height() - targetSize.height()) / 2);
    setGeometry(QRect(topLeft, targetSize));
    LOGINFO("Window fitted to media, window=%dx%d, scale=%.4f", targetSize.width(), targetSize.height(), m_baseDisplayScale);
}

void LumaPlayer::fitInitialWindowToDesktop()
{
    QDesktopWidget* desktop = QApplication::desktop();
    QRect availableRect = desktop != nullptr ? desktop->screenGeometry(this) : QRect(QPoint(0, 0), g_config.m_fallbackDesktopSize);
    QSize targetSize((std::max)(g_config.m_minWindowWidth, static_cast<int32_t>(availableRect.width() * g_config.m_initialDesktopFraction)), (std::max)(g_config.m_minWindowHeight, static_cast<int32_t>(availableRect.height() * g_config.m_initialDesktopFraction)));
    QPoint topLeft(availableRect.left() + (availableRect.width() - targetSize.width()) / 2, availableRect.top() + (availableRect.height() - targetSize.height()) / 2);
    setGeometry(QRect(topLeft, targetSize));
    LOGINFO("Initial window fitted, window=%dx%d", targetSize.width(), targetSize.height());
}

void LumaPlayer::togglePlayPause()
{
    postAction(LumaActionTogglePlayback);
}

void LumaPlayer::applyFullScreen()
{
    ++m_windowStateSerial;
    if (isFullScreen())
    {
        if (m_maximizedOutsideFullScreen)
        {
            showMaximized();
            completeWindowState(LumaActionFullscreen, Qt::WindowMaximized);
        }
        else
        {
            showNormal();
            completeWindowState(LumaActionFullscreen, Qt::WindowNoState);
        }
        LOGINFO("Exit fullscreen, maximized=%d normal=%d,%d %dx%d",
            m_maximizedOutsideFullScreen ? 1 : 0, m_normalWindowGeometry.x(), m_normalWindowGeometry.y(),
            m_normalWindowGeometry.width(), m_normalWindowGeometry.height());
    }
    else
    {
        m_maximizedOutsideFullScreen = isMaximized();
        if (!m_maximizedOutsideFullScreen)
        {
            m_normalWindowGeometry = geometry();
        }
        else if (!m_normalWindowGeometry.isValid())
        {
            m_normalWindowGeometry = normalGeometry();
        }
        showFullScreen();
        completeWindowState(LumaActionFullscreen, Qt::WindowFullScreen);
        LOGINFO("Enter fullscreen, restoreMaximized=%d", m_maximizedOutsideFullScreen ? 1 : 0);
    }
}

void LumaPlayer::applyMaximize()
{
    ++m_windowStateSerial;
    if (isFullScreen())
    {
        m_maximizedOutsideFullScreen = !m_maximizedOutsideFullScreen;
        update();
        acknowledgeWindow(LumaActionMaximize, isFullScreen());
        LOGINFO("Fullscreen retained, restoreMaximized=%d", m_maximizedOutsideFullScreen ? 1 : 0);
    }
    else if (isMaximized())
    {
        m_maximizedOutsideFullScreen = false;
        showNormal();
        completeWindowState(LumaActionMaximize, Qt::WindowNoState);
    }
    else
    {
        // 只在离开普通状态前捕获，最大化/全屏产生的尺寸不能覆盖用户矩形。
        m_normalWindowGeometry = geometry();
        showMaximized();
        m_maximizedOutsideFullScreen = isMaximized();
        completeWindowState(LumaActionMaximize, Qt::WindowMaximized);
        LOGINFO("Window maximized, normal=%d,%d %dx%d", m_normalWindowGeometry.x(),
            m_normalWindowGeometry.y(), m_normalWindowGeometry.width(), m_normalWindowGeometry.height());
    }
}

void LumaPlayer::completeWindowState(LumaPlayerActionType type, Qt::WindowState state)
{
    const uint64_t serial = m_windowStateSerial;
    // Qt5原生切换通知先完成，再核对最终状态；回调无参数、无返回值。
    QTimer::singleShot(0, this, [this, serial, type, state]()
        {
            if (m_closeRequested || m_closeReady || serial != m_windowStateSerial)
            {
                return;
            }
            setWindowState(state);
            if (state == Qt::WindowNoState && m_normalWindowGeometry.isValid())
            {
                setGeometry(m_normalWindowGeometry);
            }
            const Qt::WindowStates actual = windowState() &
                (Qt::WindowFullScreen | Qt::WindowMaximized | Qt::WindowMinimized);
            const bool success = actual == state && (state != Qt::WindowNoState || !m_normalWindowGeometry.isValid() || geometry() == m_normalWindowGeometry);
            LOGINFO("Window state applied, serial=%llu target=%d success=%d geometry=%d,%d %dx%d",
                static_cast<unsigned long long>(serial), static_cast<int>(state), success ? 1 : 0,
                x(), y(), width(), height());
            acknowledgeWindow(type, success);
        });
}

bool LumaPlayer::isMaximizedOutsideFullScreen() const
{
    return isFullScreen() ? m_maximizedOutsideFullScreen : isMaximized();
}

void LumaPlayer::previewByProgressPoint(const QPoint& point)
{
    if (!m_isDraggingProgress || !m_hasMedia || m_snapshot.m_duration100ns <= 0)
    {
        return;
    }
    const int64_t target = progressPointToTime100ns(point);
    if (m_dragPosition100ns != target)
    {
        m_dragPosition100ns = target;
        // 进度交互不等待后台解码，画面预览独立更新。
        update(bottomOverlayRect());
    }
    const int64_t nowMs = m_elapsedTimer.elapsed();
    if (m_dragPosition100ns == m_lastPreviewRequestPosition100ns ||
        nowMs - m_lastPreviewRequestMs < g_config.m_previewDragIntervalMs)
    {
        return;
    }
    m_lastPreviewRequestMs = nowMs;
    m_lastPreviewRequestPosition100ns = m_dragPosition100ns;
    postAction(LumaActionDragPreview, m_dragPosition100ns);
}

void LumaPlayer::commitSeekByProgressPoint(const QPoint& point, bool resumeAfterSeek)
{
    if (m_closeRequested || m_closeReady)
    {
        m_hasDragPosition = false;
        return;
    }
    if (!m_hasMedia || m_snapshot.m_duration100ns <= 0)
    {
        return;
    }
    m_dragPosition100ns = progressPointToTime100ns(point);
    // 单击和拖动都先显示用户目标，实际定位完成后由回执清除临时位置。
    m_hasDragPosition = true;
    if (m_isDraggingProgress)
    {
        LumaPlayerLogicAction action;
        action.m_type = LumaActionDragCommit;
        action.m_operation = LumaPlayerCoreCOperationSeek;
        action.m_value = m_dragPosition100ns;
        action.m_flag = resumeAfterSeek;
        action.m_revision = ++m_nextInputSerial;
        m_lastSeekInput = action.m_revision;
        if (!m_logic.submit(action))
        {
            m_hasDragPosition = false;
            LOGERROR("Drag commit rejected input=%llu", static_cast<unsigned long long>(action.m_revision));
        }
    }
    else
    {
        postCore(LumaPlayerCoreCOperationSeek, m_dragPosition100ns, resumeAfterSeek);
    }
    LOGINFO("Progress seek committed, position=%lld, resume=%d", static_cast<long long>(m_dragPosition100ns), resumeAfterSeek ? 1 : 0);
    update();
}

void LumaPlayer::showProgressMenu(const QPoint& point)
{
    if (!m_hasMedia)
    {
        return;
    }
    m_progressMenuActive = true;
    m_bottomTargetVisible = true;
    m_bottomVisibleHeight = bottomOverlayHeight();
    int64_t position = progressPointToTime100ns(point);
    if (!LumaPlayerHelper::isPlayingState(m_snapshot.m_state))
    {
        commitSeekByProgressPoint(point, false);
    }
    else
    {
        // 利用用户选择菜单的时间后台解析真实边界，不移动画面或预画AB竖线。
        postCore(LumaPlayerCoreCOperationPrepareLoopPoint, position);
    }
    Menu menu(this);
    menu.setFont(font());
    menu.setWindowFlags(menu.windowFlags() | Qt::WindowStaysOnTopHint);
    menu.setBackgroundColor(g_config.m_menuBackgroundColor);
    menu.setTextColor(QColor(g_config.m_textColor.name()));
    menu.setBorderWidth(g_config.m_menuBorderWidth);
    menu.setBorderColor(g_config.m_menuBorderColor);
    menu.setItemPadding(g_config.m_menuPaddingLeft, g_config.m_menuPaddingVertical,
        g_config.m_menuPaddingRight, g_config.m_menuPaddingVertical);
    menu.setItemBackgroundColor(Qt::transparent, g_config.m_menuSelectionColor, Qt::transparent);
    QAction* setAAction = menu.addAction(g_config.m_setAText);
    QAction* setBAction = menu.addAction(g_config.m_setBText);
    QAction* clearAction = menu.addAction(g_config.m_clearABText);
    QAction* selectedAction = menu.exec(mapToGlobal(point));
    m_progressMenuActive = false;
    if (selectedAction == setAAction)
    {
        postCore(LumaPlayerCoreCOperationSetA, position);
    }
    else if (selectedAction == setBAction)
    {
        postCore(LumaPlayerCoreCOperationSetB, position, false, 1);
    }
    else if (selectedAction == clearAction)
    {
        postCore(LumaPlayerCoreCOperationClearLoop);
    }
    update();
}

void LumaPlayer::setZoomPercent(int32_t newZoomPercent, const QPoint& anchorPoint)
{
    LumaPlayerLogicAction action;
    action.m_type = LumaActionZoom;
    action.m_value = newZoomPercent - m_zoomPercent;
    action.m_position = anchorPoint;
    action.m_viewport = videoViewportRect();
    action.m_viewportGeneration = m_viewportGeneration;
    m_logic.submit(action);
}

void LumaPlayer::moveHoveredLoopPoint(int32_t frameOffset)
{
    if (m_hoverLoopPoint < 0 || frameOffset == 0)
    {
        return;
    }
    m_pendingLoopMovePoint = m_hoverLoopPoint;
    postCore(LumaPlayerCoreCOperationMoveLoop, frameOffset, false, m_hoverLoopPoint);
}

void LumaPlayer::updateOverlayTargets()
{
    if (m_pinned)
    {
        m_topTargetVisible = true;
        m_bottomTargetVisible = m_hasMedia;
        m_topVisibleHeight = topOverlayHeight();
        m_bottomVisibleHeight = bottomOverlayHeight();
        return;
    }
    if (!m_hasMedia)
    {
        m_bottomVisibleHeight = 0;
        m_lastBottomOverlayActiveMs = 0;
    }
    int64_t nowMs = m_elapsedTimer.elapsed();
    bool nearTop = m_mouseInside && m_lastMousePos.y() <= g_config.m_topTriggerMargin;
    bool nearBottom = m_mouseInside && m_lastMousePos.y() >= height() - g_config.m_progressTriggerMargin;
    bool overTop = m_mouseInside && m_topVisibleHeight > 0 && topOverlayRect().contains(m_lastMousePos);
    bool overBottom = m_mouseInside && m_bottomVisibleHeight > 0 && bottomOverlayRect().contains(m_lastMousePos);
    bool topActive = m_fileDialogActive || m_dragWindow || nearTop || overTop;
    bool bottomActive = m_volumePopupVisible || m_dragVolume || m_microphonePopupVisible ||
        m_dragMicrophoneGain || m_fileDialogActive || m_progressMenuActive ||
        m_isDraggingProgress || nearBottom || overBottom;
    if (topActive)
    {
        m_lastTopOverlayActiveMs = nowMs;
    }
    if (bottomActive)
    {
        m_lastBottomOverlayActiveMs = nowMs;
    }
    m_topTargetVisible = topActive || (m_lastTopOverlayActiveMs > 0 && nowMs - m_lastTopOverlayActiveMs <= g_config.m_mouseIdleHideMs);
    m_bottomTargetVisible = m_hasMedia && (bottomActive || (m_lastBottomOverlayActiveMs > 0 && nowMs - m_lastBottomOverlayActiveMs <= g_config.m_mouseIdleHideMs));
}

void LumaPlayer::updateOverlayAnimation()
{
    int targetTop = m_topTargetVisible ? topOverlayHeight() : 0;
    int targetBottom = m_bottomTargetVisible ? bottomOverlayHeight() : 0;
    int oldTop = m_topVisibleHeight;
    int oldBottom = m_bottomVisibleHeight;
    if (m_topVisibleHeight < targetTop)
    {
        m_topVisibleHeight = (std::min)(targetTop, m_topVisibleHeight + g_config.m_overlayStep);
    }
    else if (m_topVisibleHeight > targetTop)
    {
        m_topVisibleHeight = (std::max)(targetTop, m_topVisibleHeight - g_config.m_overlayStep);
    }
    if (m_bottomVisibleHeight < targetBottom)
    {
        m_bottomVisibleHeight = (std::min)(targetBottom, m_bottomVisibleHeight + g_config.m_overlayStep);
    }
    else if (m_bottomVisibleHeight > targetBottom)
    {
        m_bottomVisibleHeight = (std::max)(targetBottom, m_bottomVisibleHeight - g_config.m_overlayStep);
    }
    if (oldTop != m_topVisibleHeight || oldBottom != m_bottomVisibleHeight)
    {
        update();
    }
}

HitArea LumaPlayer::hitTest(const QPoint& point) const
{
    if (m_microphonePopupVisible && microphonePopupRect().contains(point))
    {
        return HitMicrophonePopup;
    }
    if (m_volumePopupVisible && volumePopupRect().contains(point))
    {
        return HitVolumePopup;
    }
    if (m_bottomVisibleHeight > 0 && volumeButtonRect().contains(point))
    {
        return HitVolumeButton;
    }
    if (m_bottomVisibleHeight > 0 && microphoneButtonRect().contains(point))
    {
        return HitMicrophoneButton;
    }
    if (m_topVisibleHeight > 0)
    {
        if (resetButtonRect().contains(point))
        {
            return m_resetEnabled ? HitResetButton : HitNone;
        }
        if (pinButtonRect().contains(point))
        {
            return HitPinButton;
        }
        if (helpButtonRect().contains(point))
        {
            return HitHelpButton;
        }
    }
    if (m_topVisibleHeight > 0 && loadButtonRect().contains(point))
    {
        return HitLoadButton;
    }
    if (m_topVisibleHeight > 0 && minimizeButtonRect().contains(point))
    {
        return HitMinButton;
    }
    if (m_topVisibleHeight > 0 && maximizeButtonRect().contains(point))
    {
        return HitMaxButton;
    }
    if (m_topVisibleHeight > 0 && closeButtonRect().contains(point))
    {
        return HitCloseButton;
    }
    if (!m_hasMedia && plusButtonRect().contains(point))
    {
        return HitPlusButton;
    }
    if (m_bottomVisibleHeight > 0 && playButtonRect().contains(point))
    {
        return HitPlayButton;
    }
    if (m_bottomVisibleHeight > 0 && progressTrackRect().adjusted(0, -g_config.m_trackHitExtra, 0, g_config.m_trackHitExtra).contains(point))
    {
        return HitProgressTrack;
    }
    if (m_bottomVisibleHeight > 0 && bottomOverlayRect().contains(point))
    {
        return HitNone;
    }
    if (m_topVisibleHeight > 0 && topOverlayRect().contains(point))
    {
        return HitTopTitle;
    }
    if (rect().contains(point))
    {
        return HitVideoArea;
    }
    return HitNone;
}

int LumaPlayer::hitTestResizeEdge(const QPoint& point) const
{
    if (isFullScreen() || isMaximized())
    {
        return ResizeNone;
    }
    int edge = ResizeNone;
    if (point.x() <= g_config.m_resizeMargin)
    {
        edge |= ResizeLeft;
    }
    if (point.x() >= width() - g_config.m_resizeMargin)
    {
        edge |= ResizeRight;
    }
    if (point.y() <= g_config.m_resizeMargin)
    {
        edge |= ResizeTop;
    }
    if (point.y() >= height() - g_config.m_resizeMargin)
    {
        edge |= ResizeBottom;
    }
    return edge;
}

void LumaPlayer::updateCursorShape(const QPoint& point)
{
    int edge = hitTestResizeEdge(point);
    if ((edge & ResizeLeft) != 0 && (edge & ResizeTop) != 0)
    {
        setCursor(Qt::SizeFDiagCursor);
    }
    else if ((edge & ResizeRight) != 0 && (edge & ResizeBottom) != 0)
    {
        setCursor(Qt::SizeFDiagCursor);
    }
    else if ((edge & ResizeRight) != 0 && (edge & ResizeTop) != 0)
    {
        setCursor(Qt::SizeBDiagCursor);
    }
    else if ((edge & ResizeLeft) != 0 && (edge & ResizeBottom) != 0)
    {
        setCursor(Qt::SizeBDiagCursor);
    }
    else if ((edge & ResizeLeft) != 0 || (edge & ResizeRight) != 0)
    {
        setCursor(Qt::SizeHorCursor);
    }
    else if ((edge & ResizeTop) != 0 || (edge & ResizeBottom) != 0)
    {
        setCursor(Qt::SizeVerCursor);
    }
    else
    {
        unsetCursor();
    }
}

QRect LumaPlayer::resizeGeometryFromMouse(const QPoint& globalPos) const
{
    QRect newGeometry = m_pressWindowGeometry;
    QPoint delta = globalPos - m_pressGlobalPos;
    if ((m_resizeEdge & ResizeLeft) != 0)
    {
        int newLeft = m_pressWindowGeometry.left() + delta.x();
        newLeft = (std::min)(newLeft, m_pressWindowGeometry.right() - g_config.m_minWindowWidth + 1);
        newGeometry.setLeft(newLeft);
    }
    if ((m_resizeEdge & ResizeRight) != 0)
    {
        int newRight = m_pressWindowGeometry.right() + delta.x();
        newRight = (std::max)(newRight, newGeometry.left() + g_config.m_minWindowWidth - 1);
        newGeometry.setRight(newRight);
    }
    if ((m_resizeEdge & ResizeTop) != 0)
    {
        int newTop = m_pressWindowGeometry.top() + delta.y();
        newTop = (std::min)(newTop, m_pressWindowGeometry.bottom() - g_config.m_minWindowHeight + 1);
        newGeometry.setTop(newTop);
    }
    if ((m_resizeEdge & ResizeBottom) != 0)
    {
        int newBottom = m_pressWindowGeometry.bottom() + delta.y();
        newBottom = (std::max)(newBottom, newGeometry.top() + g_config.m_minWindowHeight - 1);
        newGeometry.setBottom(newBottom);
    }
    return newGeometry;
}

int64_t LumaPlayer::progressPointToTime100ns(const QPoint& point) const
{
    int64_t duration = m_snapshot.m_duration100ns;
    if (duration <= 0)
    {
        return 0;
    }
    QRect track = progressTrackRect();
    if (track.width() <= 0)
    {
        return 0;
    }
    int x = LumaPlayerHelper::clampInt(point.x(), track.left(), track.right());
    int span = (std::max)(1, track.width() - 1);
    double ratio = static_cast<double>(x - track.left()) / static_cast<double>(span);
    int64_t position = static_cast<int64_t>(duration * ratio + 0.5);
    return LumaPlayerHelper::clampTime100ns(position, 0, duration);
}

int LumaPlayer::progressTimeToX(int64_t time100ns) const
{
    QRect track = progressTrackRect();
    int64_t duration = m_snapshot.m_duration100ns;
    if (duration <= 0 || track.width() <= 0)
    {
        return track.left();
    }
    int64_t position = LumaPlayerHelper::clampTime100ns(time100ns, 0, duration);
    int span = (std::max)(1, track.width() - 1);
    double ratio = static_cast<double>(position) / static_cast<double>(duration);
    return track.left() + static_cast<int>(ratio * span + 0.5);
}

int64_t LumaPlayer::displayPosition100ns() const
{
    if (m_isDraggingProgress || m_hasDragPosition)
    {
        return m_dragPosition100ns;
    }
    if (!m_cachedFrame.isNull() && m_cachedFrameEnd > m_cachedFrameStart)
    {
        if (m_isDraggingProgress || !LumaPlayerHelper::isPlayingState(m_snapshot.m_state))
        {
            return m_cachedFrameStart;
        }
        return (std::max)(m_cachedFrameStart,
            (std::min)(m_snapshot.m_position100ns, m_cachedFrameEnd - 1));
    }
    return m_snapshot.m_position100ns;
}

double LumaPlayer::currentVideoBaseScale(const QSize& videoSize) const
{
    if (!videoSize.isValid())
    {
        return 1.0;
    }
    if (m_pinned || isFullScreen() || isMaximized())
    {
        double widthScale = static_cast<double>(videoViewportRect().width()) / static_cast<double>(videoSize.width());
        double heightScale = static_cast<double>(videoViewportRect().height()) / static_cast<double>(videoSize.height());
        double scale = (std::min)(widthScale, heightScale);
        return scale > 0.0 ? scale : 1.0;
    }
    return m_baseDisplayScale > 0.0 ? m_baseDisplayScale : 1.0;
}

int LumaPlayer::topOverlayHeight() const
{
    return g_config.m_topOverlayHeight;
}

int LumaPlayer::bottomOverlayHeight() const
{
    return m_hasMedia ? g_config.m_bottomOverlayHeight : 0;
}

QRect LumaPlayer::topOverlayRect() const
{
    return QRect(0, m_topVisibleHeight - topOverlayHeight(), width(), topOverlayHeight());
}

QRect LumaPlayer::bottomOverlayRect() const
{
    return QRect(0, height() - m_bottomVisibleHeight, width(), bottomOverlayHeight());
}

QRect LumaPlayer::loadButtonRect() const
{
    return QRect(g_config.m_titleMargin + g_config.m_titleIconSize + g_config.m_titleButtonGap,
        topOverlayRect().top() + g_config.m_titleButtonTop,
        g_config.m_titleButtonSize, g_config.m_titleButtonSize);
}

QRect LumaPlayer::titleIconRect() const
{
    return QRect(g_config.m_titleMargin,
        topOverlayRect().top() + (topOverlayHeight() - g_config.m_titleIconSize) / 2,
        g_config.m_titleIconSize, g_config.m_titleIconSize);
}

QRect LumaPlayer::minimizeButtonRect() const
{
    return maximizeButtonRect().translated(-g_config.m_titleButtonSize - g_config.m_titleButtonGap, 0);
}

QRect LumaPlayer::maximizeButtonRect() const
{
    return closeButtonRect().translated(-g_config.m_titleButtonSize - g_config.m_titleButtonGap, 0);
}

QRect LumaPlayer::closeButtonRect() const
{
    return QRect(width() - g_config.m_titleMargin - g_config.m_titleButtonSize,
        topOverlayRect().top() + g_config.m_titleButtonTop,
        g_config.m_titleButtonSize, g_config.m_titleButtonSize);
}

QRect LumaPlayer::plusButtonRect() const
{
    int size = (std::min)(videoViewportRect().width(), videoViewportRect().height()) / g_config.m_emptySizeDivisor;
    size = LumaPlayerHelper::clampInt(size, g_config.m_emptyMinSize, g_config.m_emptyMaxSize);
    return QRect(videoViewportRect().center() - QPoint(size / 2, size / 2), QSize(size, size));
}

QRect LumaPlayer::playButtonRect() const
{
    QRect overlay = bottomOverlayRect();
    int size = g_config.m_playButtonSize;
    return QRect((width() - size) / 2, overlay.bottom() - size - g_config.m_playButtonBottom, size, size);
}

QRect LumaPlayer::progressTrackRect() const
{
    QFont timeFont = font();
    timeFont.setPointSize(g_config.m_timeFontSize);
    const QFontMetrics metrics(timeFont);
    const int32_t timeWidth = (std::max)(g_config.m_timeWidth, metrics.width(LumaPlayerHelper::formatTime(m_snapshot.m_duration100ns, false)));
    const int32_t margin = g_config.m_timeMargin +
        (std::max)(metrics.width(LumaPlayerHelper::formatTime(m_snapshot.m_duration100ns, false)),
            timeWidth - g_config.m_trackExtension) +
        g_config.m_timeTrackGap;
    return QRect(margin, bottomOverlayRect().top() + g_config.m_progressTrackTop,
        (std::max)(1, width() - 2 * margin), g_config.m_trackHeight);
}

QRect LumaPlayer::videoDrawRect() const
{
    QSize videoSize = m_videoRender.videoSize();
    if (!videoSize.isValid())
    {
        videoSize = QSize(m_snapshot.m_videoWidth, m_snapshot.m_videoHeight);
    }
    if (!videoSize.isValid())
    {
        return QRect();
    }
    double scale = currentVideoBaseScale(videoSize) * (static_cast<double>(m_zoomPercent) / 100.0);
    int drawWidth = (std::max)(1, static_cast<int>(videoSize.width() * scale + 0.5));
    int drawHeight = (std::max)(1, static_cast<int>(videoSize.height() * scale + 0.5));
    int left = static_cast<int>(videoViewportRect().left() + (videoViewportRect().width() - drawWidth) / 2.0 + m_panOffset.x() + 0.5);
    int top = static_cast<int>(videoViewportRect().top() + (videoViewportRect().height() - drawHeight) / 2.0 + m_panOffset.y() + 0.5);
    return QRect(left, top, drawWidth, drawHeight);
}

void LumaPlayer::updateLoopMarkerHover(const QPoint& point)
{
    m_hoverLoopPoint = -1;
    if (modalBlocksInput() || (m_volumePopupVisible && volumePopupRect().contains(point)) ||
        (m_topVisibleHeight > 0 && topOverlayRect().contains(point)))
    {
        return;
    }
    if (!m_hasMedia || m_snapshot.m_duration100ns <= 0 || m_bottomVisibleHeight <= 0)
    {
        return;
    }
    QRect track = progressTrackRect();
    QRect hitBand = track.adjusted(-g_config.m_markerHitWidth, -g_config.m_markerHitTop, g_config.m_markerHitWidth, g_config.m_markerHitBottom);
    if (!hitBand.contains(point))
    {
        return;
    }
    bool hitA = m_snapshot.m_hasLoopA != 0 && std::abs(point.x() - progressTimeToX(m_snapshot.m_loopAStart100ns)) <= g_config.m_markerHitWidth;
    bool hitB = m_snapshot.m_hasLoopB != 0 && std::abs(point.x() - progressTimeToX(m_snapshot.m_loopBEnd100ns)) <= g_config.m_markerHitWidth;
    if (hitA && hitB)
    {
        m_hoverLoopPoint = m_lastActiveLoopPoint == 0 ? 0 : 1;
        return;
    }
    if (hitA)
    {
        m_hoverLoopPoint = 0;
        return;
    }
    if (hitB)
    {
        m_hoverLoopPoint = 1;
    }
}

void LumaPlayer::paintVideo(QPainter& painter)
{
    QRect drawRect = videoDrawRect();
    if (!m_cachedFrame.isNull() && drawRect.isValid())
    {
        if (drawRect.width() < m_cachedFrame.width() && drawRect.height() < m_cachedFrame.height())
        {
            // 缩小采用面积采样，避免仅用双线性绘制时细节混叠；平移不改变缓存
            if (m_scaledFrameSourceKey != m_cachedFrame.cacheKey() || m_scaledFrame.size() != drawRect.size())
            {
                m_scaledFrame = m_cachedFrame.scaled(drawRect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                m_scaledFrameSourceKey = m_cachedFrame.cacheKey();
            }
            if (!m_scaledFrame.isNull())
            {
                painter.drawImage(drawRect.topLeft(), m_scaledFrame);
                return;
            }
        }
        else
        {
            m_scaledFrame = QImage();
            m_scaledFrameSourceKey = 0;
        }
        painter.drawImage(drawRect, m_cachedFrame);
        return;
    }
    if (!m_hasMedia)
    {
        QRect plusRect = plusButtonRect();
        bool hover = m_hoverArea == HitPlusButton;
        QColor circleColor = hover ? g_config.m_emptyHoverColor : g_config.m_emptyButtonColor;
        QPen pen(g_config.m_emptyIconColor);
        pen.setWidthF(g_config.m_emptyStroke);
        painter.setPen(pen);
        painter.setBrush(circleColor);
        painter.drawEllipse(plusRect.adjusted(g_config.m_emptyBorderInset, g_config.m_emptyBorderInset, -g_config.m_emptyBorderInset, -g_config.m_emptyBorderInset));
        if (m_mouseInside && m_leftPressed && m_pressArea == HitPlusButton && plusRect.contains(m_lastMousePos))
        {
            painter.save();
            painter.setPen(Qt::NoPen);
            painter.setOpacity(g_config.m_buttonPressOpacity);
            painter.setBrush(g_config.m_buttonPressColor);
            painter.drawEllipse(plusRect);
            painter.restore();
        }
        int cx = plusRect.center().x();
        int cy = plusRect.center().y();
        int len = plusRect.width() / g_config.m_emptyArmDivisor;
        painter.drawLine(QPoint(cx - len, cy), QPoint(cx + len, cy));
        painter.drawLine(QPoint(cx, cy - len), QPoint(cx, cy + len));
    }
}

void LumaPlayer::paintTopOverlay(QPainter& painter)
{
    if (m_topVisibleHeight <= 0)
    {
        return;
    }
    painter.save();
    painter.setClipRect(QRect(0, 0, width(), m_topVisibleHeight));
    QRect overlay = topOverlayRect();
    painter.setPen(Qt::NoPen);
    painter.setBrush(g_config.m_overlayColor);
    painter.drawRect(overlay);
    painter.setPen(g_config.m_overlayBorderColor);
    painter.drawLine(overlay.bottomLeft(), overlay.bottomRight());
    windowIcon().paint(&painter, titleIconRect());
    paintTitleButton(painter, loadButtonRect(), HitLoadButton);
    paintTitleButton(painter, resetButtonRect(), HitResetButton);
    paintTitleButton(painter, pinButtonRect(), HitPinButton);
    paintTitleButton(painter, helpButtonRect(), HitHelpButton);
    paintTitleButton(painter, minimizeButtonRect(), HitMinButton);
    paintTitleButton(painter, maximizeButtonRect(), HitMaxButton);
    paintTitleButton(painter, closeButtonRect(), HitCloseButton);
    painter.restore();
}

bool LumaPlayer::modalBlocksInput() const
{
    QWidget* modal = QApplication::activeModalWidget();
    return m_fileDialogActive || (modal != nullptr && modal != this && (modal->windowModality() == Qt::ApplicationModal || isAncestorOf(modal) || (modal->windowHandle() != nullptr && modal->windowHandle()->transientParent() == windowHandle())));
}

bool LumaPlayer::eventFilter(QObject* watched, QEvent* event)
{
    QWidget* widget = qobject_cast<QWidget*>(watched);
    if (event->type() == QEvent::Show && widget != nullptr && widget != this &&
        widget->isWindow() && widget->isModal() &&
        (widget->windowModality() == Qt::ApplicationModal || isAncestorOf(widget) ||
            (widget->windowHandle() != nullptr && widget->windowHandle()->transientParent() == windowHandle())))
    {
        hideVolumePopup();
        m_microphonePopupVisible = false;
        cancelDeferredInput();
    }
    if (watched != this)
    {
        if ((m_volumePopupVisible || m_microphonePopupVisible) && event->type() == QEvent::MouseButtonPress &&
            widget != nullptr && !isAncestorOf(widget) &&
            !(m_microphoneView && widget->window() == m_microphoneView->window()) &&
            !(m_microphoneDeviceMenuActive && widget->window()->objectName() == QStringLiteral("microphoneDeviceMenu")))
        {
            hideVolumePopup();
            m_microphonePopupVisible = false;
        }
        return Widget::eventFilter(watched, event);
    }
    if (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::Hide)
    {
        hideVolumePopup();
        if ((!m_microphoneDeviceMenuActive && !m_microphoneView) || event->type() == QEvent::Hide)
        {
            m_microphonePopupVisible = false;
        }
        cancelDeferredInput();
    }
    const bool input = event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease ||
        event->type() == QEvent::Shortcut || event->type() == QEvent::ShortcutOverride ||
        event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease ||
        event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::MouseMove ||
        event->type() == QEvent::Wheel || event->type() == QEvent::ContextMenu;
    if (input && (modalBlocksInput() || m_closeRequested))
    {
        event->accept();
        return true;
    }
    return Widget::eventFilter(watched, event);
}

void LumaPlayer::cancelDeferredInput(bool finishProgress)
{
    m_clickTimer.stop();
    if (m_isDraggingProgress && finishProgress)
    {
        commitSeekByProgressPoint(m_lastMousePos, false);
    }
    else if (m_isDraggingProgress || m_progressPressPending)
    {
        m_hasDragPosition = false;
    }
    m_isDraggingProgress = false;
    m_progressPressPending = false;
    m_progressWasPlaying = false;
    m_lastPreviewRequestMs = 0;
    m_lastPreviewRequestPosition100ns = -1;
    if (m_loopMoveKey != 0 || m_pendingLoopMovePoint >= 0)
    {
        postAction(LumaActionCancelMove);
        m_lastMoveInput = ++m_nextInputSerial;
    }
    m_loopMoveKey = 0;
    m_loopMoveRepeating = false;
    m_pendingLoopMovePoint = -1;
    m_hoverLoopPoint = -1;
    m_dragVideo = false;
    m_dragWindow = false;
    m_resizeWindow = false;
    m_resizeEdge = ResizeNone;
    m_pressArea = HitNone;
    m_leftPressed = false;
    m_cancelClickToggle = true;
}

void LumaPlayer::hideVolumePopup()
{
    m_volumeMutePressed = false;
    if (m_volumePopupVisible || m_dragVolume)
    {
        m_volumePopupVisible = false;
        m_dragVolume = false;
        QToolTip::hideText();
        updateOverlayTargets();
        update();
    }
}

QRect LumaPlayer::volumeButtonRect() const
{
    const QRect play = playButtonRect();
    return QRect(width() - g_config.m_timeMargin - play.width(), play.top(), play.width(), play.height()).translated(g_config.m_volumeIconOffset);
}

QRect LumaPlayer::volumePopupRect() const
{
    const int32_t top = topOverlayHeight() + g_config.m_volumeGap;
    const int32_t bottom = bottomOverlayRect().top() - g_config.m_volumeGap;
    const int32_t popupHeight = (std::min)(g_config.m_volumePopupSize.height(), (std::max)(1, bottom - top));
    const int32_t popupWidth = g_config.m_volumePopupSize.width();
    const int32_t left = (std::max)(g_config.m_volumeGap,
        (std::min)(width() - popupWidth - g_config.m_volumeGap, volumeButtonRect().center().x() - popupWidth / 2));
    return QRect(left, bottom - popupHeight, popupWidth, popupHeight);
}

QRect LumaPlayer::volumeTrackRect() const
{
    const QRect popup = volumePopupRect();
    const int32_t sliderHeight = popup.height() - g_config.m_volumeFooterHeight;
    const int32_t padding = (std::min)(g_config.m_volumePadding, (std::max)(1, sliderHeight / 4));
    return QRect(popup.center().x() - g_config.m_trackHeight / 2, popup.top() + padding,
        g_config.m_trackHeight, (std::max)(2, sliderHeight - padding * 2));
}

QRect LumaPlayer::volumeMuteButtonRect() const
{
    const QRect popup = volumePopupRect();
    const int32_t size = g_config.m_titleButtonSize;
    return QRect(popup.center().x() - size / 2,
        popup.bottom() - g_config.m_volumeFooterHeight + 1 + (g_config.m_volumeFooterHeight - size) / 2, size, size)
        .translated(g_config.m_volumeMuteButtonOffset);
}

void LumaPlayer::showVolumeTooltip(const QPoint& point)
{
    if (m_volumePopupVisible && volumeMuteButtonRect().contains(point))
    {
        QToolTip::showText(mapToGlobal(point), m_volumeMuted ? g_config.m_volumeRestoreText : g_config.m_volumeMuteText, this);
        return;
    }
    const QString text = QString::fromStdWString(CStringManager::Format(
        g_config.m_volumeTooltipFormat.toStdWString().c_str(), m_volumePercent));
    QToolTip::showText(mapToGlobal(point), text, this);
}

void LumaPlayer::volumeFromPoint(const QPoint& point)
{
    const QRect track = volumeTrackRect();
    const int32_t span = (std::max)(1, track.height() - 1);
    const int32_t distance = (std::max)(0, (std::min)(span, track.bottom() - point.y()));
    requestVolume((distance * g_config.m_volumeNormalPercent + span / 2) / span);
}

void LumaPlayer::requestVolume(int32_t percent, bool fromMuteButton)
{
    if (m_closeRequested)
    {
        return;
    }
    m_volumePercent = (std::max)(0, (std::min)(g_config.m_maxZoomPercent, percent));
    if (!fromMuteButton)
    {
        m_volumeMuted = false;
    }
    submitVolume();
    update();
}

void LumaPlayer::submitVolume()
{
    if (m_volumeRequest != 0 || m_volumePercent == m_appliedVolumePercent || m_closeRequested)
    {
        return;
    }
    LumaPlayerLogicAction action;
    action.m_type = LumaActionVolume;
    action.m_value = m_volumePercent;
    action.m_revision = ++m_nextInputSerial;
    if (m_logic.submit(action))
    {
        m_volumeRequest = action.m_revision;
    }
    else
    {
        LOGWARNING("Volume submission rejected percent=%d", m_volumePercent);
        m_volumePercent = m_appliedVolumePercent;
    }
}

void LumaPlayer::paintVolume(QPainter& painter, bool popup)
{
    if (!m_hasMedia || (popup ? !m_volumePopupVisible : m_bottomVisibleHeight <= 0))
    {
        return;
    }
    painter.save();
    if (popup)
    {
        const QRect panel = volumePopupRect();
        painter.setPen(g_config.m_overlayBorderColor);
        painter.setBrush(g_config.m_overlayColor);
        painter.drawRoundedRect(panel, g_config.m_volumeRadius, g_config.m_volumeRadius);
        const QRect track = volumeTrackRect();
        const int32_t value = (std::min)(g_config.m_volumeNormalPercent, m_volumePercent);
        const int32_t knobY = track.bottom() - (track.height() - 1) * value / g_config.m_volumeNormalPercent;
        painter.setPen(Qt::NoPen);
        painter.setBrush(g_config.m_trackColor);
        painter.drawRoundedRect(track, g_config.m_trackHeight / 2.0, g_config.m_trackHeight / 2.0);
        painter.setBrush(g_config.m_themeColor);
        painter.drawRoundedRect(QRect(track.left(), knobY, track.width(), track.bottom() - knobY + 1),
            g_config.m_trackHeight / 2.0, g_config.m_trackHeight / 2.0);
        painter.setBrush(g_config.m_playbackPositionColor);
        const QPointF knob = QPointF(track.center().x(), knobY) + g_config.m_volumeKnobOffset;
        const bool hover = panel.contains(mapFromGlobal(QCursor::pos()));
        const int32_t radius = hover && !m_dragVolume ? g_config.m_positionDragRadius : g_config.m_positionRadius;
        painter.drawEllipse(knob, radius, radius);
        if (m_dragVolume)
        {
            painter.setOpacity(g_config.m_buttonPressOpacity);
            painter.setBrush(g_config.m_buttonPressColor);
            painter.drawEllipse(knob, radius, radius);
        }
        painter.setOpacity(1.0);
        const int32_t dividerY = panel.bottom() - g_config.m_volumeFooterHeight + 1;
        // 填满像素行，避免抗锯齿描边将白线分摊到相邻两行而显灰。
        painter.fillRect(QRect(panel.left() + g_config.m_volumeGap, dividerY,
                             panel.width() - 2 * g_config.m_volumeGap, g_config.m_volumeSeparatorHeight),
            g_config.m_volumeSeparatorColor);
        paintVolumeButton(painter, volumeMuteButtonRect(), m_volumeMutePressed && volumeMuteButtonRect().contains(mapFromGlobal(QCursor::pos())));
    }
    else
    {
        paintVolumeButton(painter, volumeButtonRect(), m_leftPressed && m_pressArea == HitVolumeButton);
    }
    painter.restore();
}

void LumaPlayer::paintMicrophone(QPainter& painter, bool popup)
{
    if (!popup)
    {
        if (m_hasMedia && m_bottomVisibleHeight > 0)
        {
            bool active = false;
            for (size_t i = 0; i < m_microphones.size(); ++i)
            {
                active = active || (!m_microphones[i]->m_muted && m_microphones[i]->m_capture.isRunning());
            }
            if (active)
            {
                painter.save();
                painter.setPen(Qt::NoPen);
                painter.setBrush(g_config.m_themeColor);
                painter.drawRoundedRect(microphoneButtonRect(), g_config.m_cornerRadius, g_config.m_cornerRadius);
                painter.restore();
            }
            paintMicrophoneIcon(painter, microphoneButtonRect(),
                m_leftPressed && m_pressArea == HitMicrophoneButton, false);
        }
        return;
    }
    if (!m_microphonePopupVisible || !m_hasMedia)
    {
        return;
    }
    const QRect viewport = microphonePopupRect();
    const QRect panel = microphoneContentRect();
    painter.save();
    painter.setPen(g_config.m_overlayBorderColor);
    painter.setBrush(g_config.m_overlayColor);
    painter.drawRoundedRect(viewport, g_config.m_volumeRadius, g_config.m_volumeRadius);
    painter.setClipRect(viewport.adjusted(1, 1, -1, -1));
    painter.setPen(g_config.m_textColor);
    QFont titleFont = font();
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(QRect(panel.left() + 16, panel.top() + 8, panel.width() - 32, 28),
        Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("麦克风控制"));
    painter.setFont(font());
    for (int i = 0; i < static_cast<int>(m_microphones.size()); ++i)
    {
        const QRect device = microphoneSlotRect(i);
        const LumaPlayerMicrophoneChannel& mic = *m_microphones[i];
        painter.setPen(i == m_selectedMicrophone ? g_config.m_themeColor : g_config.m_overlayBorderColor);
        painter.setBrush(i == m_selectedMicrophone ? g_config.m_buttonHoverColor : g_config.m_buttonColor);
        painter.drawRect(device.adjusted(0, 0, -1, -1));
        painter.setPen(g_config.m_textColor);
        painter.drawText(device.adjusted(8, 2, i > 0 ? -20 : -8, -20), Qt::AlignLeft | Qt::AlignVCenter,
            QStringLiteral("麦%1%2").arg(i + 1).arg(mic.m_muted ? QStringLiteral(" · 关闭") : QStringLiteral(" · 开启")));
        if (i > 0)
        {
            const QRect cross = microphoneRemoveRect(i).adjusted(5, 5, -5, -5);
            painter.setPen(QPen(m_karaokeSessionStarted ? QColor(100, 106, 116) : g_config.m_textColor, 1));
            painter.drawLine(cross.topLeft(), cross.bottomRight());
            painter.drawLine(cross.bottomLeft(), cross.topRight());
        }
        QString deviceName = mic.m_capture.deviceName();
        if (deviceName.isEmpty())
        {
            deviceName = mic.m_label;
        }
        if (deviceName.isEmpty())
        {
            deviceName = QStringLiteral("选择输入设备");
        }
        const QRect choice = microphoneDeviceChoiceRect(i);
        const QRect label = choice.adjusted(8, 0, -18, 0);
        painter.setPen(QColor(174, 184, 197));
        painter.drawText(label, Qt::AlignLeft | Qt::AlignVCenter, QFontMetrics(font()).elidedText(deviceName, Qt::ElideRight, label.width()));
        const QPoint arrow(choice.right() - 8, choice.center().y());
        QPolygon chevron;
        chevron << arrow + QPoint(-3, -1) << arrow + QPoint(0, 2) << arrow + QPoint(3, -1);
        painter.drawPolyline(chevron);
    }
    if (m_microphones.size() < static_cast<size_t>(g_config.m_microphoneMaxCount))
    {
        painter.setPen(g_config.m_textColor);
        painter.drawText(microphoneAddRect(), Qt::AlignCenter, QStringLiteral("+"));
    }
    painter.setPen(QPen(g_config.m_overlayBorderColor, 1));
    painter.drawLine(panel.left() + 16, panel.top() + 96, panel.right() - 16, panel.top() + 96);
    painter.setPen(QColor(174, 184, 197));
    painter.drawText(panel.left() + 16, panel.top() + 112, QStringLiteral("伴奏处理"));
    painter.setPen(g_config.m_textColor);
    painter.drawText(panel.left() + 16, panel.top() + 139,
        m_karaokeBacking != nullptr ? QStringLiteral("播放音轨") : QStringLiteral("伴奏"));
    if (m_karaokeBacking == nullptr)
    {
        const QRect generate = microphoneRemovalSwitchRect();
        painter.setPen(g_config.m_overlayBorderColor);
        painter.setBrush(m_hasMedia ? g_config.m_buttonHoverColor : g_config.m_buttonColor);
        painter.drawRoundedRect(generate, 4, 4);
        painter.setPen(g_config.m_textColor);
        painter.drawText(generate, Qt::AlignCenter, QStringLiteral("生成伴奏"));
    }
    else
    {
        const QRect original = microphoneOriginalTrackRect();
        const QRect backing = microphoneBackingTrackRect();
        QPainterPath outline;
        outline.addRoundedRect(QRectF(original.united(backing)), 4, 4);
        painter.save();
        painter.setClipPath(outline, Qt::IntersectClip);
        painter.fillRect(original, m_vocalRemovalEnabled ? g_config.m_buttonColor : g_config.m_buttonHoverColor);
        painter.fillRect(backing, m_vocalRemovalEnabled ? g_config.m_buttonHoverColor : g_config.m_buttonColor);
        painter.restore();
        painter.setPen(g_config.m_overlayBorderColor);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(outline);
        painter.drawLine(backing.topLeft(), backing.bottomLeft());
        painter.setPen(g_config.m_textColor);
        painter.drawText(original, Qt::AlignCenter, QStringLiteral("原唱"));
        painter.drawText(backing, Qt::AlignCenter, QStringLiteral("伴奏"));
    }
    painter.setPen(g_config.m_textColor);
    painter.drawText(panel.left() + 16, panel.top() + 181, QStringLiteral("升降调"));
    QRect down = microphonePitchDownRect();
    QRect up = microphonePitchUpRect();
    QRect pitch(panel.right() - 108, panel.top() + 160, 56, 28);
    painter.setPen(Qt::NoPen);
    painter.setBrush(g_config.m_buttonHoverColor);
    painter.drawRoundedRect(down, 4, 4);
    painter.drawRoundedRect(up, 4, 4);
    painter.setPen(g_config.m_textColor);
    painter.drawText(down, Qt::AlignCenter, QStringLiteral("−"));
    painter.drawText(up, Qt::AlignCenter, QStringLiteral("+"));
    painter.setPen(g_config.m_overlayBorderColor);
    painter.setBrush(QColor(32, 42, 55));
    painter.drawRoundedRect(pitch, 4, 4);
    painter.setPen(g_config.m_textColor);
    painter.drawText(pitch, Qt::AlignCenter, m_karaokePitchSemitones == 0 ? QStringLiteral("0 半音") : QStringLiteral("%1%2 半音").arg(m_karaokePitchSemitones > 0 ? QStringLiteral("+") : QString()).arg(m_karaokePitchSemitones));
    painter.setPen(QPen(g_config.m_overlayBorderColor, 1));
    painter.drawLine(panel.left() + 16, panel.top() + 205, panel.right() - 16, panel.top() + 205);
    painter.setPen(g_config.m_textColor);
    painter.drawText(QRect(panel.left() + 16, panel.top() + 212, panel.width() - 130, 28),
        Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("实时监听"));
    paintKaraokeSwitch(painter, microphoneMonitorSwitchRect(), m_microphoneMonitoring);
    painter.drawText(QRect(panel.left() + 16, panel.top() + 254, panel.width() - 130, 28),
        Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("录制"));
    paintKaraokeSwitch(painter, microphoneRecordSwitchRect(), m_karaokeRecording);
    painter.setPen(QColor(142, 153, 168));
    painter.drawText(QRect(panel.left() + 16, panel.top() + 292, panel.width() - 130, 24),
        Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("人声美化（预留）"));
    painter.setPen(QColor(174, 184, 197));
    painter.drawText(QRect(panel.right() - 78, panel.top() + 216, 64, 17),
        Qt::AlignCenter, QStringLiteral("麦%1增益").arg(m_selectedMicrophone + 1));
    painter.drawText(QRect(panel.right() - 78, panel.top() + 232, 64, 14),
        Qt::AlignCenter, QStringLiteral("%1%").arg(currentMicrophone().m_gain));
    const QRect track = microphoneGainTrackRect();
    painter.setPen(Qt::NoPen);
    painter.setBrush(g_config.m_trackColor);
    painter.drawRoundedRect(track, 2, 2);
    const int32_t displayGain = (std::min)(100, currentMicrophone().m_gain);
    const int32_t knobY = track.bottom() - (track.height() - 1) * displayGain / 100;
    painter.setBrush(g_config.m_themeColor);
    painter.drawRoundedRect(QRect(track.left(), knobY, track.width(), track.bottom() - knobY + 1), 2, 2);
    painter.setBrush(g_config.m_playbackPositionColor);
    painter.drawEllipse(QPointF(track.center().x(), knobY), 5, 5);
    const QRect mute = microphoneMuteButtonRect();
    painter.setPen(Qt::NoPen);
    painter.setBrush(g_config.m_buttonHoverColor);
    painter.drawRoundedRect(mute, 5, 5);
    paintMicrophoneIcon(painter, mute, false, currentMicrophone().m_muted);
    const QString statusMessage = m_karaokeNotice.isEmpty() ? currentMicrophone().m_status : m_karaokeNotice;
    if (!statusMessage.isEmpty())
    {
        const QRect status(panel.left() + 16, panel.bottom() - 44, panel.width() - 98, 34);
        painter.setPen(QColor(174, 184, 197));
        const QString statusText = QFontMetrics(font()).elidedText(statusMessage,
            Qt::ElideRight, status.width());
        painter.drawText(status, Qt::AlignVCenter | Qt::AlignLeft, statusText);
    }
    painter.restore();
}

void LumaPlayer::paintMicrophoneIcon(QPainter& painter, const QRect& button,
    bool pressed, bool muted)
{
    painter.save();
    if (pressed)
    {
        painter.setPen(Qt::NoPen);
        painter.setOpacity(g_config.m_buttonPressOpacity);
        painter.setBrush(g_config.m_buttonPressColor);
        painter.drawRoundedRect(button.translated(g_config.m_volumePressOffset),
            g_config.m_cornerRadius, g_config.m_cornerRadius);
        painter.setOpacity(1.0);
    }
    QPainterPath icon;
    icon.addRoundedRect(QRectF(8, 2, 8, 13), 4, 4);
    icon.moveTo(5, 11);
    icon.cubicTo(5, 21, 19, 21, 19, 11);
    icon.moveTo(12, 19);
    icon.lineTo(12, 22);
    icon.moveTo(8, 22);
    icon.lineTo(16, 22);
    painter.translate(button.center().x() - g_config.m_iconCanvasSize / 2,
        button.center().y() - g_config.m_iconCanvasSize / 2);
    painter.scale(g_config.m_iconCanvasSize / 24.0, g_config.m_iconCanvasSize / 24.0);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(g_config.m_iconColor, g_config.m_iconStroke,
        Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(icon);
    if (muted)
    {
        painter.setPen(QPen(g_config.m_volumeMuteColor, g_config.m_iconStroke + 0.2,
            Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(4, 20), QPointF(20, 4));
    }
    painter.restore();
}

void LumaPlayer::paintKaraokeSwitch(QPainter& painter, const QRect& rect,
    bool enabled, bool available)
{
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(!available ? QColor(80, 88, 99, 180) : (enabled ? g_config.m_themeColor : QColor(89, 100, 116)));
    painter.drawRoundedRect(rect, rect.height() / 2.0, rect.height() / 2.0);
    const int32_t diameter = (std::max)(6, rect.height() - 5);
    const int32_t x = enabled && available ? rect.right() - diameter - 2 : rect.left() + 2;
    painter.setBrush(available ? QColor(245, 247, 250) : QColor(175, 182, 191));
    painter.drawEllipse(QRect(x, rect.center().y() - diameter / 2, diameter, diameter));
    painter.restore();
}

void LumaPlayer::paintVolumeButton(QPainter& painter, const QRect& button, bool pressed)
{
    painter.save();
    painter.setPen(Qt::NoPen);
    if (pressed)
    {
        painter.setOpacity(g_config.m_buttonPressOpacity);
        painter.setBrush(g_config.m_buttonPressColor);
        painter.drawRoundedRect(button.translated(g_config.m_volumePressOffset), g_config.m_cornerRadius, g_config.m_cornerRadius);
        painter.setOpacity(1.0);
    }
    painter.translate(button.center().x() - g_config.m_iconCanvasSize / 2,
        button.center().y() - g_config.m_iconCanvasSize / 2);
    painter.setPen(Qt::NoPen);
    painter.setBrush(g_config.m_iconColor);
    painter.drawPath(g_config.m_speakerIcon);
    if (m_volumeMuted)
    {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(g_config.m_volumeMuteColor, g_config.m_iconStroke, Qt::SolidLine, Qt::RoundCap));
        painter.drawPath(g_config.m_volumeMuteIcon);
        painter.restore();
        return;
    }
    const int32_t arcs = m_volumePercent == 0 ? 0 : m_volumePercent <= g_config.m_volumeLowPercent ? 1
        : m_volumePercent <= g_config.m_volumeMediumPercent                                        ? 2
                                                                                                   : 3;
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(g_config.m_iconColor, g_config.m_iconStroke, Qt::SolidLine, Qt::RoundCap));
    for (int32_t index = 0; index < arcs; ++index)
    {
        painter.drawPath(g_config.m_volumeArcs[index]);
    }
    painter.restore();
}

void LumaPlayer::paintBottomOverlay(QPainter& painter)
{
    if (m_bottomVisibleHeight <= 0)
    {
        return;
    }
    painter.save();
    painter.setClipRect(QRect(0, height() - m_bottomVisibleHeight, width(), m_bottomVisibleHeight));
    QRect overlay = bottomOverlayRect();
    painter.setPen(Qt::NoPen);
    painter.setBrush(g_config.m_overlayColor);
    painter.drawRect(overlay);
    painter.setPen(g_config.m_overlayBorderColor);
    painter.drawLine(overlay.topLeft(), overlay.topRight());

    QFont textFont = font();
    textFont.setPointSize(g_config.m_timeFontSize);
    painter.setFont(textFont);
    painter.setPen(g_config.m_timeTextColor);
    int64_t displayPosition = displayPosition100ns();
    const QRect track = progressTrackRect();
    const int32_t textHeight = QFontMetrics(textFont).height();
    const int32_t textTop = track.center().y() - textHeight / 2 + g_config.m_timeTextOffset;
    painter.drawText(QRect(g_config.m_timeMargin, textTop,
                         track.left() - g_config.m_timeTrackGap - g_config.m_timeMargin, textHeight),
        Qt::AlignVCenter | Qt::AlignLeft, LumaPlayerHelper::formatTime(displayPosition, false));
    painter.drawText(QRect(track.right() + g_config.m_timeTrackGap, textTop,
                         width() - track.right() - g_config.m_timeTrackGap - g_config.m_timeMargin, textHeight),
        Qt::AlignVCenter | Qt::AlignRight, LumaPlayerHelper::formatTime(m_snapshot.m_duration100ns, false));

    int progressX = progressTimeToX(displayPosition);
    painter.setPen(Qt::NoPen);
    painter.setBrush(g_config.m_trackColor);
    painter.drawRoundedRect(track, g_config.m_trackHeight / 2.0, g_config.m_trackHeight / 2.0);
    QRect played(track.left(), track.top(), (std::max)(0, progressX - track.left()), track.height());
    painter.setBrush(g_config.m_themeColor);
    painter.drawRoundedRect(played, g_config.m_trackHeight / 2.0, g_config.m_trackHeight / 2.0);
    painter.setBrush(g_config.m_playbackPositionColor);
    const QPointF knob = QPointF(progressX, track.center().y()) + g_config.m_progressKnobOffset;
    const bool hoverKnob = m_mouseInside &&
        (m_lastMousePos - knob).manhattanLength() <= 2 * g_config.m_positionDragRadius;
    const bool knobPressed = m_isDraggingProgress || m_progressPressPending;
    const int32_t radius = hoverKnob && !knobPressed ? g_config.m_positionDragRadius : g_config.m_positionRadius;
    painter.drawEllipse(knob, radius, radius);
    if (knobPressed)
    {
        painter.save();
        painter.setOpacity(g_config.m_buttonPressOpacity);
        painter.setBrush(g_config.m_buttonPressColor);
        painter.drawEllipse(knob, radius, radius);
        painter.restore();
    }

    bool drawA = m_snapshot.m_hasLoopA != 0;
    bool drawB = m_snapshot.m_hasLoopB != 0;
    bool drawAOnTop = false;
    if (drawA && drawB)
    {
        int ax = progressTimeToX(m_snapshot.m_loopAStart100ns);
        int bx = progressTimeToX(m_snapshot.m_loopBEnd100ns);
        drawAOnTop = std::abs(ax - bx) <= g_config.m_markerOverlapTolerance && m_lastActiveLoopPoint == 0;
    }
    if (drawAOnTop)
    {
        paintLoopMarker(painter, m_snapshot.m_loopBEnd100ns, g_config.m_bLabel, m_hoverLoopPoint == 1);
        paintLoopMarker(painter, m_snapshot.m_loopAStart100ns, g_config.m_aLabel, m_hoverLoopPoint == 0);
    }
    else
    {
        if (drawA)
        {
            paintLoopMarker(painter, m_snapshot.m_loopAStart100ns, g_config.m_aLabel, m_hoverLoopPoint == 0);
        }
        if (drawB)
        {
            paintLoopMarker(painter, m_snapshot.m_loopBEnd100ns, g_config.m_bLabel, m_hoverLoopPoint == 1);
        }
    }
    paintPlayIcon(painter, playButtonRect());
    paintVolume(painter, false);
    paintMicrophone(painter, false);
    painter.restore();
}

void LumaPlayer::paintCenterPlayTip(QPainter& painter)
{
    if (!m_hasMedia || LumaPlayerHelper::isPlayingState(m_snapshot.m_state))
    {
        return;
    }
    int64_t nowMs = m_elapsedTimer.elapsed();
    if ((nowMs - m_lastMouseMoveMs) > g_config.m_mouseIdleHideMs)
    {
        return;
    }
    int size = (std::min)(videoViewportRect().width(), videoViewportRect().height()) / g_config.m_centerTipSizeDivisor;
    size = LumaPlayerHelper::clampInt(size, g_config.m_centerTipMinSize, g_config.m_centerTipMaxSize);
    QRect circleRect(videoViewportRect().center() - QPoint(size / 2, size / 2), QSize(size, size));
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(g_config.m_centerTipBackgroundColor);
    painter.drawEllipse(circleRect);
    painter.setBrush(g_config.m_centerTipIconColor);
    painter.translate(circleRect.topLeft());
    painter.scale(size / g_config.m_iconCanvasSize, size / g_config.m_iconCanvasSize);
    painter.drawPath(g_config.m_playIcon);
    painter.restore();
}

void LumaPlayer::paintZoomTip(QPainter& painter)
{
    const int64_t nowMs = m_elapsedTimer.elapsed();
    const bool showRateTip = nowMs < m_rateTipHideMs;
    const bool showZoomTip = !showRateTip && nowMs < m_zoomTipHideMs;
    if (!showRateTip && !showZoomTip)
    {
        return;
    }
    QRect tipRect(videoViewportRect().center() - QPoint(g_config.m_zoomTipWidth / 2, g_config.m_zoomTipHeight / 2), QSize(g_config.m_zoomTipWidth, g_config.m_zoomTipHeight));
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(g_config.m_tipBackgroundColor);
    painter.drawRoundedRect(tipRect, g_config.m_zoomTipRadius, g_config.m_zoomTipRadius);
    QFont tipFont = font();
    tipFont.setPointSize(g_config.m_zoomTipFontSize);
    painter.setFont(tipFont);
    painter.setPen(g_config.m_zoomTextColor);
    const QString text = showRateTip ? QString::number(static_cast<double>(m_rateTipPermille) / 1000.0, 'f', 1) + QStringLiteral("x") : QString::fromStdString(CStringManager::Format(g_config.m_zoomTextFormat.c_str(), m_zoomPercent));
    painter.drawText(tipRect, Qt::AlignCenter, text);
    painter.restore();
}

void LumaPlayer::paintTitleButton(QPainter& painter, const QRect& rect, HitArea hitArea)
{
    const bool enabled = hitArea != HitResetButton || m_resetEnabled;
    const bool hover = enabled && m_hoverArea == hitArea;
    painter.save();
    painter.setPen(Qt::NoPen);
    const bool pressed = enabled && m_mouseInside && m_leftPressed && m_pressArea == hitArea && rect.contains(m_lastMousePos);
    painter.setBrush(hitArea == HitCloseButton ? (pressed ? g_config.m_closeButtonColor.darker(g_config.m_closePressDarkening) : g_config.m_closeButtonColor) : (hover ? g_config.m_buttonHoverColor : g_config.m_buttonColor));
    painter.drawRoundedRect(rect, g_config.m_cornerRadius, g_config.m_cornerRadius);
    if (pressed && hitArea != HitCloseButton)
    {
        painter.setOpacity(g_config.m_buttonPressOpacity);
        painter.setBrush(g_config.m_buttonPressColor);
        painter.drawRoundedRect(rect, g_config.m_cornerRadius, g_config.m_cornerRadius);
        painter.setOpacity(1.0);
    }
    const bool blue = hitArea == HitHelpButton || (hitArea == HitPinButton && m_pinned);
    QPen pen(!enabled ? g_config.m_disabledColor : blue ? g_config.m_themeColor
                                                        : g_config.m_iconColor);
    pen.setWidthF(g_config.m_iconStroke);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.translate(rect.topLeft());
    painter.scale(rect.width() / g_config.m_iconCanvasSize, rect.height() / g_config.m_iconCanvasSize);
    const QPainterPath* path = &g_config.m_loadIcon;
    if (hitArea == HitResetButton)
    {
        path = &g_config.m_resetIcon;
    }
    else if (hitArea == HitHelpButton)
    {
        path = &g_config.m_questionIcon;
    }
    else if (hitArea == HitPinButton)
    {
        path = &g_config.m_pinIcon;
        if (!m_pinned)
        {
            painter.translate(g_config.m_iconCanvasSize / 2, g_config.m_iconCanvasSize / 2);
            painter.rotate(g_config.m_pinAngle);
            painter.translate(-g_config.m_iconCanvasSize / 2, -g_config.m_iconCanvasSize / 2);
        }
    }
    else if (hitArea == HitMinButton)
    {
        path = &g_config.m_minimizeIcon;
    }
    else if (hitArea == HitMaxButton)
    {
        path = isMaximizedOutsideFullScreen() ? &g_config.m_restoreIcon : &g_config.m_maximizeIcon;
    }
    else if (hitArea == HitCloseButton)
    {
        path = &g_config.m_closeIcon;
    }
    painter.drawPath(*path);
    painter.restore();
}

void LumaPlayer::paintPlayIcon(QPainter& painter, const QRect& rect)
{
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_hoverArea == HitPlayButton ? g_config.m_playHoverColor : g_config.m_playButtonColor);
    painter.drawEllipse(rect);
    if (m_mouseInside && m_leftPressed && m_pressArea == HitPlayButton && rect.contains(m_lastMousePos))
    {
        painter.setOpacity(g_config.m_buttonPressOpacity);
        painter.setBrush(g_config.m_buttonPressColor);
        painter.drawEllipse(rect);
        painter.setOpacity(1.0);
    }
    painter.setBrush(g_config.m_textColor);
    painter.translate(rect.topLeft());
    painter.scale(rect.width() / g_config.m_iconCanvasSize, rect.height() / g_config.m_iconCanvasSize);
    painter.drawPath(LumaPlayerHelper::isPlayingState(m_snapshot.m_state) ? g_config.m_pauseIcon : g_config.m_playIcon);
    painter.restore();
}

void LumaPlayer::paintLoopMarker(QPainter& painter, int64_t markerTime100ns, const QString& label, bool hovered)
{
    const bool isB = label == g_config.m_bLabel;
    const int32_t x = progressTimeToX(markerTime100ns);
    const QRect track = progressTrackRect();
    const QColor color = isB ? g_config.m_loopBColor : g_config.m_loopAColor;
    painter.save();
    QPen pen(color);
    pen.setWidth(hovered ? g_config.m_markerHoverWidth : g_config.m_markerWidth);
    painter.setPen(pen);
    painter.drawLine(x, track.center().y() - g_config.m_markerHeight / 2,
        x, track.center().y() + g_config.m_markerHeight / 2);
    QFont markerFont = font();
    markerFont.setPointSize(g_config.m_timeFontSize);
    painter.setFont(markerFont);
    painter.drawText(QRect(x - g_config.m_markerLabelWidth / 2,
                         track.bottom() + g_config.m_markerLabelGap, g_config.m_markerLabelWidth,
                         g_config.m_markerLabelHeight),
        Qt::AlignCenter, label);
    if (hovered)
    {
        const int64_t time = isB ? (std::max<int64_t>)(0, markerTime100ns - g_config.m_millisecond100ns) : markerTime100ns;
        const QString text = LumaPlayerHelper::formatTime(time, true);
        const QFontMetrics metrics(markerFont);
        const int32_t labelWidth = metrics.width(text) + 2 * g_config.m_timestampPadding;
        const int32_t labelHeight = metrics.height();
        const int32_t left = LumaPlayerHelper::clampInt(x - labelWidth / 2,
            g_config.m_timeMargin, (std::max)(g_config.m_timeMargin, width() - labelWidth - g_config.m_timeMargin));
        painter.drawText(QRect(left, track.top() - labelHeight - g_config.m_timestampGap,
                             labelWidth, labelHeight),
            Qt::AlignCenter, text);
    }
    painter.restore();
}

void LumaPlayer::postAction(LumaPlayerActionType type, int64_t value, int32_t point)
{
    if (m_closeRequested)
    {
        return;
    }
    LumaPlayerLogicAction action;
    action.m_type = type;
    action.m_value = value;
    action.m_point = point;
    action.m_revision = ++m_nextInputSerial;
    if (type == LumaActionJump || type == LumaActionDragBegin)
    {
        m_lastSeekInput = action.m_revision;
        if (type == LumaActionJump)
        {
            m_hasDragPosition = false;
        }
    }
    if (type == LumaActionCancelMove)
    {
        m_lastMoveInput = action.m_revision;
    }
    if (type == LumaActionShutdown)
    {
        if ((m_karaokeRecording || m_karaokeSessionStarted) && !finishKaraokeSession())
        {
            return;
        }
        stopKaraokeForShutdown();
        m_closeRequested = m_logic.submit(action);
        return;
    }
    m_logic.submit(action);
}

void LumaPlayer::postCore(int32_t operation, int64_t value, bool flag, int32_t point)
{
    LumaPlayerLogicAction action;
    action.m_type = LumaActionCore;
    action.m_operation = operation;
    action.m_value = value;
    action.m_flag = flag;
    action.m_point = point;
    action.m_revision = ++m_nextInputSerial;
    if (operation == LumaPlayerCoreCOperationKaraokeRecording && value == 1)
    {
        m_karaokeStartInput = action.m_revision;
    }
    if (operation == LumaPlayerCoreCOperationSeek)
    {
        m_lastSeekInput = action.m_revision;
    }
    if (operation == LumaPlayerCoreCOperationMoveLoop)
    {
        m_lastMoveInput = action.m_revision;
    }
    if (!m_closeRequested)
    {
        if (!m_logic.submit(action))
        {
            if (operation == LumaPlayerCoreCOperationSeek)
            {
                m_hasDragPosition = false;
            }
            if (operation == LumaPlayerCoreCOperationMoveLoop)
            {
                m_pendingLoopMovePoint = -1;
            }
            LOGERROR("GUI submission rejected operation=%d input=%llu", operation,
                static_cast<unsigned long long>(action.m_revision));
        }
    }
}

void LumaPlayer::toggleFullScreen()
{
    postAction(LumaActionFullscreen);
}

void LumaPlayer::toggleMaximize()
{
    postAction(LumaActionMaximize);
}

void LumaPlayer::closeEvent(QCloseEvent* event)
{
    hideVolumePopup();
    if (m_closeReady)
    {
        event->accept();
        return;
    }
    event->ignore();
    postAction(LumaActionShutdown);
}

QRect LumaPlayer::microphoneButtonRect() const
{
    const QRect speaker = volumeButtonRect();
    return speaker.translated(-speaker.width() - 3, 0);
}

QRect LumaPlayer::microphonePopupRect() const
{
    const QRect screen = QApplication::desktop()->availableGeometry(this);
    QPoint anchor = mapToGlobal(QPoint(volumePopupRect().right(), bottomOverlayRect().top() - g_config.m_volumeGap));
    // Keep the right edge aligned even when a narrow player leaves insufficient room on its left.
    anchor.setX((std::min)(anchor.x(), screen.right()) - 359);
    anchor.setY((std::max)(screen.top(), (std::min)(anchor.y() - 430, screen.bottom() - 429)));
    return QRect(mapFromGlobal(anchor), QSize(360, 430));
}

QRect LumaPlayer::microphoneContentRect() const
{
    return microphonePopupRect();
}

QRect LumaPlayer::microphoneDeviceRect() const
{
    const QRect panel = microphoneContentRect();
    return QRect(panel.left() + 16, panel.top() + 42, panel.width() - 32, 40);
}

QRect LumaPlayer::microphoneDeviceChoiceRect(int32_t index) const
{
    const bool lastWithAdd = index + 1 == static_cast<int32_t>(m_microphones.size()) &&
        m_microphones.size() < static_cast<size_t>(g_config.m_microphoneMaxCount);
    return microphoneSlotRect(index).adjusted(0, 19, lastWithAdd ? -24 : -2, -2);
}

QRect LumaPlayer::microphoneRemovalSwitchRect() const
{
    const QRect panel = microphoneContentRect();
    return QRect(panel.right() - 104, panel.top() + 116, 88, 24);
}

QRect LumaPlayer::microphoneOriginalTrackRect() const
{
    const QRect panel = microphoneContentRect();
    return QRect(panel.right() - 146, panel.top() + 116, 65, 24);
}

QRect LumaPlayer::microphoneBackingTrackRect() const
{
    const QRect panel = microphoneContentRect();
    return QRect(panel.right() - 81, panel.top() + 116, 65, 24);
}

QRect LumaPlayer::microphoneSlotRect(int32_t index) const
{
    const QRect area = microphoneDeviceRect();
    const int count = static_cast<int>(m_microphones.size());
    const int left = area.left() + area.width() * index / count;
    const int right = area.left() + area.width() * (index + 1) / count;
    return QRect(left, area.top(), right - left, area.height());
}

QRect LumaPlayer::microphoneAddRect() const
{
    const QRect area = microphoneDeviceRect();
    return QRect(area.right() - 21, area.bottom() - 19, 20, 18);
}

void LumaPlayer::attachMicrophonePanel(QWidget* view)
{
    m_microphoneView = view;
}
void LumaPlayer::dismissMicrophonePanel()
{
    m_microphonePopupVisible = false;
    m_dragMicrophoneGain = false;
}
bool LumaPlayer::microphonePanelVisible() const
{
    return m_microphonePopupVisible && isVisible() && !isMinimized() && !m_closeRequested && !modalBlocksInput();
}
QRect LumaPlayer::microphonePanelGeometry() const
{
    return QRect(mapToGlobal(microphonePopupRect().topLeft()), microphonePopupRect().size());
}
void LumaPlayer::syncMicrophonePanel()
{
    if (microphonePanelVisible() && !m_microphoneView)
    {
        LumaPlayerMicrophoneParam param(this);
        param.m_parent = windowHandle();
        param.m_title = QStringLiteral("麦克风控制");
        DialogManager::instance().makeDialog(param);
        if (!m_microphoneView)
        {
            m_microphonePopupVisible = false;
        }
    }
}
void LumaPlayer::paintMicrophonePanel(QPainter& painter)
{
    painter.translate(-microphonePopupRect().topLeft());
    painter.setFont(font());
    paintMicrophone(painter, true);
}
void LumaPlayer::microphonePanelMouse(QMouseEvent* event)
{
    if (modalBlocksInput() || m_closeRequested)
    {
        return;
    }
    const QPoint point = mapFromGlobal(event->globalPos());
    if (event->type() == QEvent::MouseMove)
    {
        if (m_dragMicrophoneGain)
        {
            setMicrophoneGainFromPoint(point);
        }
    }
    else if (event->type() == QEvent::MouseButtonRelease)
    {
        m_dragMicrophoneGain = false;
    }
    else if (event->button() == Qt::LeftButton)
    {
        if (microphoneGainTrackRect().adjusted(-10, -8, 10, 8).contains(point))
        {
            m_dragMicrophoneGain = true;
            setMicrophoneGainFromPoint(point);
        }
        else if ((microphoneRecordSwitchRect().contains(point) && (m_karaokeRecording || m_karaokeSessionStarted)) ||
            (microphoneRemovalSwitchRect().contains(point) && !m_karaokeBacking))
        {
            // A modal action can retire the tool window. Finish its input dispatch first.
            QTimer::singleShot(0, this, [this, point]()
                {
                    if (!modalBlocksInput() && !m_closeRequested)
                    {
                        handleMicrophonePopupClick(point);
                    }
                });
        }
        else
        {
            handleMicrophonePopupClick(point);
        }
    }
    if (m_microphoneView)
    {
        m_microphoneView->update();
    }
}
void LumaPlayer::microphonePanelKey(QKeyEvent* event)
{
    if (modalBlocksInput() || m_closeRequested)
    {
        return;
    }
    keyPressEvent(event);
    if (m_microphoneView)
    {
        m_microphoneView->update();
    }
}

QRect LumaPlayer::microphonePitchDownRect() const
{
    const QRect panel = microphoneContentRect();
    return QRect(panel.right() - 144, panel.top() + 158, 30, 32);
}

QRect LumaPlayer::microphonePitchUpRect() const
{
    const QRect panel = microphoneContentRect();
    return QRect(panel.right() - 42, panel.top() + 158, 30, 32);
}

QRect LumaPlayer::microphoneMonitorSwitchRect() const
{
    const QRect panel = microphoneContentRect();
    return QRect(panel.left() + panel.width() - 132, panel.top() + 222, 38, 22);
}

QRect LumaPlayer::microphoneRecordSwitchRect() const
{
    const QRect panel = microphoneContentRect();
    return QRect(panel.left() + panel.width() - 132, panel.top() + 264, 38, 22);
}

QRect LumaPlayer::microphoneGainTrackRect() const
{
    const QRect panel = microphoneContentRect();
    return QRect(panel.right() - 48, panel.top() + 256, 5, (std::max)(20, panel.height() - 318));
}

QRect LumaPlayer::microphoneMuteButtonRect() const
{
    const QRect panel = microphoneContentRect();
    const int32_t size = 44;
    return QRect(panel.right() - 68, panel.bottom() - 56, size, 32);
}

bool LumaPlayer::startMicrophone()
{
    const bool captureAlreadyRunning = currentMicrophone().m_capture.isRunning();
    QStringList devices;
    if (!captureAlreadyRunning)
    {
        devices = LumaPlayerMicrophoneCapture::availableDeviceNames();
        if (devices.isEmpty())
        {
            currentMicrophone().m_status = QStringLiteral("未检测到可用麦克风");
            return false;
        }
        if (currentMicrophone().m_device < 0 || currentMicrophone().m_device >= devices.size())
        {
            currentMicrophone().m_device = -1;
            for (int32_t i = 0; i < devices.size(); ++i)
            {
                if (!microphoneDeviceUsed(i, m_selectedMicrophone))
                {
                    currentMicrophone().m_device = i;
                    break;
                }
            }
        }
        if (currentMicrophone().m_device < 0 || microphoneDeviceUsed(currentMicrophone().m_device, m_selectedMicrophone))
        {
            currentMicrophone().m_status = QStringLiteral("没有未占用的麦克风，请选择其他输入设备");
            return false;
        }
    }
    if (!captureAlreadyRunning && currentMicrophone().m_initialized)
    {
        currentMicrophone().m_task.uninit();
        currentMicrophone().m_initialized = false;
    }
    if (!captureAlreadyRunning && !currentMicrophone().m_capture.prepare(currentMicrophone().m_device, 2000, g_config.m_microphoneDeviceBufferMs))
    {
        currentMicrophone().m_status = currentMicrophone().m_capture.lastError();
        return false;
    }
    if (!m_karaokeTimelineInitialized)
    {
        const int32_t result = m_core.initKaraokeRecording(
            currentMicrophone().m_capture.sampleRate(), currentMicrophone().m_capture.channels(),
            0);
        if (result != LumaPlayerCoreCResultSuccess)
        {
            currentMicrophone().m_status = QStringLiteral("录音时间线初始化失败 (%1)").arg(result);
            if (currentMicrophone().m_initialized)
            {
                currentMicrophone().m_task.uninit();
                currentMicrophone().m_initialized = false;
            }
            currentMicrophone().m_capture.stop();
            return false;
        }
        m_karaokeTimelineInitialized = true;
    }
    currentMicrophone().m_status = captureAlreadyRunning ? currentMicrophone().m_capture.deviceName() : devices[currentMicrophone().m_device];
    setKaraokeControls();
    if (!currentMicrophone().m_initialized)
    {
        currentMicrophone().m_initialized = currentMicrophone().m_task.init();
        if (!currentMicrophone().m_initialized)
        {
            currentMicrophone().m_capture.stop();
            currentMicrophone().m_status = QStringLiteral("麦克风处理线程启动失败");
            return false;
        }
    }
    return true;
}

LumaPlayerMicrophoneChannel& LumaPlayer::currentMicrophone()
{
    return *m_microphones[m_selectedMicrophone];
}

const LumaPlayerMicrophoneChannel& LumaPlayer::currentMicrophone() const
{
    return *m_microphones[m_selectedMicrophone];
}

bool LumaPlayer::microphoneDeviceUsed(int32_t device, int32_t except) const
{
    for (size_t i = 0; i < m_microphones.size(); ++i)
    {
        if (static_cast<int32_t>(i) != except && m_microphones[i]->m_device == device)
        {
            return true;
        }
    }
    for (size_t i = 0; i < m_retiredMicrophones.size(); ++i)
    {
        if (!m_retiredMicrophones[i]->m_retired.load() && m_retiredMicrophones[i]->m_device == device)
        {
            return true;
        }
    }
    return false;
}

QRect LumaPlayer::microphoneRemoveRect(int32_t index) const
{
    if (index <= 0 || index >= static_cast<int32_t>(m_microphones.size()))
    {
        return QRect();
    }
    const QRect slot = microphoneSlotRect(index);
    return QRect(slot.right() - 18, slot.top() + 1, 18, 18);
}

void LumaPlayer::removeMicrophone(int32_t index)
{
    if (index <= 0 || index >= static_cast<int32_t>(m_microphones.size()))
    {
        return;
    }
    if (m_karaokeSessionStarted)
    {
        m_karaokeNotice = QStringLiteral("当前录音尚未保存，不能移除麦克风");
        return;
    }
    const std::shared_ptr<LumaPlayerMicrophoneChannel> channel = m_microphones[index];
    if (!m_logic.submitCleanup(std::make_shared<LumaPlayerMicrophoneRetireTask>(channel)))
    {
        m_karaokeNotice = QStringLiteral("暂时无法移除麦克风，请稍后重试");
        return;
    }
    m_retiredMicrophones.push_back(channel);
    m_microphones.erase(m_microphones.begin() + index);
    if (m_selectedMicrophone >= index)
    {
        m_selectedMicrophone = (std::max)(0, m_selectedMicrophone - 1);
    }
    LOGINFO("Microphone removal submitted stableSlot=%d", channel->m_slot);
}

void LumaPlayer::setKaraokeControls()
{
    for (size_t i = 0; i < m_microphones.size(); ++i)
    {
        LumaPlayerMicrophoneChannel& mic = *m_microphones[i];
        mic.m_task.setControls(mic.m_gain * g_config.m_microphoneGainScale, mic.m_muted, m_microphoneMonitoring,
            m_karaokeRecording && m_karaokeRecordingReady, m_karaokeRecording && m_karaokeRecordingReady);
    }
}

void LumaPlayer::setMicrophoneGainFromPoint(const QPoint& point)
{
    const QRect track = microphoneGainTrackRect();
    const int32_t span = (std::max)(1, track.height() - 1);
    const int32_t distance = (std::max)(0,
        (std::min)(span, track.bottom() - point.y()));
    currentMicrophone().m_gain = (distance * 100 + span / 2) / span;
    setKaraokeControls();
    QToolTip::showText(mapToGlobal(point),
        QStringLiteral("麦克风增益 %1%").arg(currentMicrophone().m_gain), this);
    update();
}

void LumaPlayer::handleMicrophonePopupClick(const QPoint& point)
{
    if (m_microphoneDeviceMenuActive)
    {
        return;
    }
    m_karaokeNotice.clear();
    for (size_t i = 0; i < m_retiredMicrophones.size();)
    {
        if (m_retiredMicrophones[i]->m_retired.load())
        {
            m_retiredMicrophones.erase(m_retiredMicrophones.begin() + i);
        }
        else
        {
            ++i;
        }
    }
    for (int i = 1; i < static_cast<int>(m_microphones.size()); ++i)
    {
        if (microphoneRemoveRect(i).contains(point))
        {
            removeMicrophone(i);
            return;
        }
    }
    if (microphoneAddRect().contains(point) && m_microphones.size() < static_cast<size_t>(g_config.m_microphoneMaxCount))
    {
        int slot = 0;
        for (; slot < g_config.m_microphoneMaxCount; ++slot)
        {
            bool used = false;
            for (size_t i = 0; i < m_microphones.size(); ++i)
            {
                used = used || m_microphones[i]->m_slot == slot;
            }
            for (size_t i = 0; i < m_retiredMicrophones.size(); ++i)
            {
                used = used || m_retiredMicrophones[i]->m_slot == slot;
            }
            if (!used)
            {
                break;
            }
        }
        if (slot >= g_config.m_microphoneMaxCount)
        {
            m_karaokeNotice = QStringLiteral("设备正在关闭，请稍后添加");
            return;
        }
        m_microphones.push_back(std::shared_ptr<LumaPlayerMicrophoneChannel>(new LumaPlayerMicrophoneChannel(&m_core, slot)));
        m_selectedMicrophone = static_cast<int>(m_microphones.size()) - 1;
        return;
    }
    if (microphoneDeviceRect().contains(point))
    {
        for (int i = 0; i < static_cast<int>(m_microphones.size()); ++i)
        {
            if (microphoneSlotRect(i).contains(point))
            {
                m_selectedMicrophone = i;
                if (!microphoneDeviceChoiceRect(i).contains(point))
                {
                    return;
                }
                break;
            }
        }
        if (m_karaokeSessionStarted)
        {
            currentMicrophone().m_status = QStringLiteral("当前录音会话尚未保存，不能切换麦克风");
            LOGINFO("Microphone device menu blocked by unsaved recording slot=%d", m_selectedMicrophone);
            return;
        }
        const QStringList devices = LumaPlayerMicrophoneCapture::availableDeviceNames();
        if (devices.isEmpty())
        {
            currentMicrophone().m_status = QStringLiteral("未检测到可用麦克风");
            return;
        }
        Menu menu(this);
        menu.setFont(font());
        menu.setWindowFlags(menu.windowFlags() | Qt::WindowStaysOnTopHint);
        menu.setBackgroundColor(g_config.m_menuBackgroundColor);
        menu.setTextColor(QColor(g_config.m_textColor.name()));
        menu.setBorderWidth(g_config.m_menuBorderWidth);
        menu.setBorderColor(g_config.m_menuBorderColor);
        menu.setItemPadding(g_config.m_menuPaddingLeft, g_config.m_menuPaddingVertical,
            g_config.m_menuPaddingRight, g_config.m_menuPaddingVertical);
        menu.setItemBackgroundColor(Qt::transparent, g_config.m_menuSelectionColor, Qt::transparent);
        for (int32_t index = 0; index < devices.size(); ++index)
        {
            const QString label = index == currentMicrophone().m_device
                ? QStringLiteral("%1  (当前)").arg(devices[index])
                : devices[index];
            QAction* action = menu.addAction(label);
            action->setData(index);
            action->setEnabled(!microphoneDeviceUsed(index, m_selectedMicrophone));
        }
        menu.setObjectName(QStringLiteral("microphoneDeviceMenu"));
        m_microphoneDeviceMenuActive = true;
        QAction* selected = menu.exec(mapToGlobal(microphoneSlotRect(m_selectedMicrophone).bottomLeft()));
        m_microphoneDeviceMenuActive = false;
        LOGINFO("Microphone device menu closed selected=%d panel=%d active=%d",
            selected != nullptr ? selected->data().toInt() : -1, m_microphonePopupVisible ? 1 : 0, isActiveWindow() ? 1 : 0);
        if (selected == NULL)
        {
            return;
        }
        const int32_t selectedIndex = selected->data().toInt();
        if (selectedIndex < 0 || selectedIndex >= devices.size() ||
            selectedIndex == currentMicrophone().m_device || microphoneDeviceUsed(selectedIndex, m_selectedMicrophone))
        {
            return;
        }
        if (currentMicrophone().m_initialized)
        {
            currentMicrophone().m_task.uninit();
            currentMicrophone().m_initialized = false;
        }
        currentMicrophone().m_capture.stop();
        currentMicrophone().m_device = selectedIndex;
        currentMicrophone().m_label = devices[selectedIndex];
        if (!currentMicrophone().m_muted && !startMicrophone())
        {
            currentMicrophone().m_muted = true;
        }
    }
    else if (m_karaokeBacking != nullptr && microphoneOriginalTrackRect().contains(point))
    {
        selectKaraokeTrack(false);
    }
    else if (m_karaokeBacking != nullptr && microphoneBackingTrackRect().contains(point))
    {
        selectKaraokeTrack(true);
    }
    else if (m_karaokeBacking == nullptr && microphoneRemovalSwitchRect().contains(point))
    {
        toggleVocalRemoval();
    }
    else if (microphonePitchDownRect().contains(point))
    {
        m_karaokePitchSemitones = (std::max)(-7, m_karaokePitchSemitones - 1);
        postCore(LumaPlayerCoreCOperationKaraokePitch, m_karaokePitchSemitones);
    }
    else if (microphonePitchUpRect().contains(point))
    {
        m_karaokePitchSemitones = (std::min)(7, m_karaokePitchSemitones + 1);
        postCore(LumaPlayerCoreCOperationKaraokePitch, m_karaokePitchSemitones);
    }
    else if (microphoneMonitorSwitchRect().contains(point))
    {
        m_microphoneMonitoring = !m_microphoneMonitoring;
        setKaraokeControls();
    }
    else if (microphoneRecordSwitchRect().contains(point))
    {
        if (!m_hasMedia)
        {
            currentMicrophone().m_status = QStringLiteral("请先打开视频，再开始录音");
            update();
            return;
        }
        if (!m_karaokeRecording)
        {
            if (m_karaokeSessionStarted)
            {
                finishKaraokeSession();
                update();
                return;
            }
            if (!currentMicrophone().m_capture.isRunning() && !startMicrophone())
            {
                return;
            }
            if (!m_karaokeTimelineInitialized && m_core.initKaraokeRecording(currentMicrophone().m_capture.sampleRate(), currentMicrophone().m_capture.channels(), 0) != LumaPlayerCoreCResultSuccess)
            {
                currentMicrophone().m_status = QStringLiteral("录音时间线初始化失败");
                return;
            }
            m_karaokeTimelineInitialized = true;
            m_karaokeSessionStarted = true;
            m_karaokeRecording = true;
            m_karaokeRecordingReady = false;
            postCore(LumaPlayerCoreCOperationKaraokeRecording, 1);
            currentMicrophone().m_status = QStringLiteral("正在录制；暂停期间不会写入麦克风音频");
        }
        else
        {
            m_karaokeRecording = false;
            setKaraokeControls();
            finishKaraokeSession();
            update();
            return;
        }
        setKaraokeControls();
    }
    else if (microphoneMuteButtonRect().contains(point))
    {
        currentMicrophone().m_muted = !currentMicrophone().m_muted;
        if (!currentMicrophone().m_muted && !startMicrophone())
        {
            currentMicrophone().m_muted = true;
        }
        setKaraokeControls();
    }
    update();
}

void LumaPlayer::stopKaraokeForShutdown()
{
    lumaPlayerCoreBackingDestroy(m_karaokeBacking);
    m_karaokeBacking = nullptr;
    m_karaokeRecording = false;
    for (size_t i = 0; i < m_microphones.size(); ++i)
    {
        LumaPlayerMicrophoneChannel& mic = *m_microphones[i];
        mic.m_task.setControls(mic.m_gain * g_config.m_microphoneGainScale, true, false, false, false);
        mic.m_task.uninit();
        mic.m_initialized = false;
        mic.m_capture.stop();
    }
}

void LumaPlayer::toggleVocalRemoval()
{
    if (!m_hasMedia || m_pendingMediaLoad || m_karaokeSource.isEmpty())
    {
        return;
    }
    if (m_karaokeRecording)
    {
        currentMicrophone().m_status = QStringLiteral("请先结束当前录音，再切换伴奏");
        return;
    }
    const QString source = m_karaokeSource;
    if (m_karaokeBacking == nullptr)
    {
        QString runtime = QString::fromLocal8Bit(qgetenv("LUMAPLAYER_ORT_RUNTIME"));
        QString model = QString::fromLocal8Bit(qgetenv("LUMAPLAYER_VOCAL_MODEL"));
        if (runtime.isEmpty())
        {
#ifdef Q_OS_WIN
            runtime = QCoreApplication::applicationDirPath() + QStringLiteral("/karaoke/onnxruntime.dll");
#elif defined(Q_OS_MAC)
            runtime = QCoreApplication::applicationDirPath() + QStringLiteral("/karaoke/libonnxruntime.dylib");
#else
            runtime = QCoreApplication::applicationDirPath() + QStringLiteral("/karaoke/libonnxruntime.so");
#endif
        }
        if (model.isEmpty())
        {
            model = QCoreApplication::applicationDirPath() + QStringLiteral("/karaoke/UVR-MDX-NET-Inst_HQ_5.onnx");
        }
        if (!QFileInfo(runtime).isFile() || !QFileInfo(model).isFile())
        {
            m_karaokeNotice = QStringLiteral("缺少人声分离模型或运行库");
            return;
        }
        LumaPlayerKaraokeJob task(source.toUtf8().constData(), runtime.toUtf8().constData(), model.toUtf8().constData());
        LOGINFO("Vocal separation source=%s runtime=%s model=%s", source.toUtf8().constData(),
            runtime.toUtf8().constData(), model.toUtf8().constData());
        if (!task.start())
        {
            m_karaokeNotice = QStringLiteral("人声分离任务启动失败");
            return;
        }
        LumaPlayerKaraokeProgressParam param(&task);
        param.m_parent = windowHandle();
        param.m_title = QStringLiteral("消除人声");
        DialogManager::instance().makeDialog(param);
        m_microphonePopupVisible = true;
        if (!task.done())
        {
            task.StopTask();
        }
        m_karaokeBacking = task.takeResult();
        if (m_karaokeBacking == nullptr)
        {
            m_karaokeNotice = task.canceled() ? QStringLiteral("人声分离已取消") : QStringLiteral("人声分离失败：%1").arg(task.error());
            LOGERROR("Vocal separation failed: %s", task.error().toUtf8().constData());
            if (!task.canceled())
            {
                TipDialogParam failure;
                failure.m_parent = windowHandle();
                failure.m_title = QStringLiteral("人声分离失败");
                failure.m_tip = m_karaokeNotice;
                DialogManager::instance().makeDialog(failure);
            }
            return;
        }
    }
    if (source == m_karaokeSource && m_core.applyKaraokeBacking(source.toUtf8().constData(), m_karaokeBacking) == LumaPlayerCoreCResultSuccess)
    {
        m_vocalRemovalEnabled = true;
        currentMicrophone().m_status = QStringLiteral("正在使用分离后的伴奏");
    }
}

void LumaPlayer::selectKaraokeTrack(bool backing)
{
    if (!m_hasMedia || m_pendingMediaLoad || m_karaokeSource.isEmpty() ||
        (backing && m_karaokeBacking == nullptr))
    {
        return;
    }
    if (m_karaokeRecording)
    {
        currentMicrophone().m_status = QStringLiteral("请先结束当前录音，再切换音轨");
        return;
    }
    const QString source = m_karaokeSource;
    const LumaPlayerCoreCBacking selected = backing ? m_karaokeBacking : nullptr;
    if (source == m_karaokeSource &&
        m_core.applyKaraokeBacking(source.toUtf8().constData(), selected) == LumaPlayerCoreCResultSuccess)
    {
        m_vocalRemovalEnabled = backing;
        currentMicrophone().m_status = backing ? QStringLiteral("正在播放伴奏") : QStringLiteral("正在播放原唱");
        update();
    }
}

bool LumaPlayer::finishKaraokeSession()
{
    if (m_karaokeFinishing)
    {
        return false;
    }
    if (!m_karaokeSessionStarted)
    {
        return true;
    }
    QScopedValueRollback<bool> finishing(m_karaokeFinishing, true);
    const int64_t end100ns = m_core.snapshot().m_recordingPosition100ns;
    LOGINFO("Karaoke finish begin position=%lld microphones=%u backing=%d", static_cast<long long>(end100ns),
        static_cast<unsigned>(m_microphones.size()), m_vocalRemovalEnabled ? 1 : 0);
    m_karaokeRecording = false;
    postCore(LumaPlayerCoreCOperationKaraokeRecording, 0);
    setKaraokeControls();
    std::vector<LumaPlayerKaraokeTask*> captures;
    for (size_t i = 0; i < m_microphones.size(); ++i)
    {
        captures.push_back(&m_microphones[i]->m_task);
    }
    LumaPlayerKaraokeJob drain(captures, &m_core, (std::max<int64_t>)(0, end100ns));
    if (!drain.start())
    {
        return false;
    }
    LumaPlayerKaraokeProgressParam drainProgress(&drain);
    drainProgress.m_parent = windowHandle();
    drainProgress.m_title = QStringLiteral("结束录音");
    DialogManager::instance().makeDialog(drainProgress);
    LOGINFO("Karaoke finish drain returned dialog=%llu done=%d success=%d error=%s",
        static_cast<unsigned long long>(drainProgress.m_dialogId), drain.done() ? 1 : 0,
        drain.succeeded() ? 1 : 0, drain.error().toUtf8().constData());
    if (!drain.succeeded())
    {
        m_karaokeNotice = QStringLiteral("录音未能结束，已保留，请重试");
        return false;
    }
    CustomDialogParam prompt(static_cast<DialogType>(CUSTOM_DIALOG_TYPE_BEGIN + 2));
    prompt.m_title = QStringLiteral("保存录音");
    prompt.m_parent = windowHandle();
    DialogManager::instance().makeDialog(prompt);
    LOGINFO("Karaoke save choice returned dialog=%llu result=%d selection=%d",
        static_cast<unsigned long long>(prompt.m_dialogId), static_cast<int>(prompt.m_result), prompt.m_userResult);
    if (prompt.m_result == ACCEPT_BUTTON)
    {
        const bool video = prompt.m_userResult >= 2;
        const bool mix = prompt.m_userResult % 2 != 0;
        const QString extension = video ? QStringLiteral(".mp4") : QStringLiteral(".mp3");
        LOGINFO("Karaoke save path chooser begin video=%d mix=%d", video ? 1 : 0, mix ? 1 : 0);
        QString path = QFileDialog::getSaveFileName(this, QStringLiteral("保存录音"),
            QStringLiteral("LumaPlayer-录音") + extension,
            video ? QStringLiteral("MP4 视频 (*.mp4)") : QStringLiteral("MP3 音频 (*.mp3)"), nullptr, QFileDialog::DontConfirmOverwrite);
        LOGINFO("Karaoke save path chooser returned selected=%d", path.isEmpty() ? 0 : 1);
        if (path.isEmpty())
        {
            return false;
        }
        if (!path.endsWith(extension, Qt::CaseInsensitive))
        {
            path += extension;
        }
        if (QFileInfo(path).canonicalFilePath() == QFileInfo(m_karaokeSource).canonicalFilePath())
        {
            m_karaokeNotice = QStringLiteral("不能覆盖当前播放的原视频");
            update();
            return false;
        }
        if (QFileInfo::exists(path))
        {
            AskDialogParam overwrite;
            overwrite.m_parent = windowHandle();
            overwrite.m_title = QStringLiteral("覆盖文件");
            overwrite.m_tip = QStringLiteral("文件已存在，是否覆盖？\n%1").arg(QDir::toNativeSeparators(path));
            overwrite.m_acceptText = QStringLiteral("覆盖");
            DialogManager::instance().makeDialog(overwrite);
            if (overwrite.m_result != ACCEPT_BUTTON)
            {
                return false;
            }
        }
        LumaPlayerKaraokeJob task(&m_core, m_karaokeSource, path, mix, video,
            m_karaokePitchSemitones, m_volumeMuted ? 0 : m_volumePercent,
            m_vocalRemovalEnabled ? m_karaokeBacking : nullptr);
        if (!task.start())
        {
            m_karaokeNotice = QStringLiteral("录音保存任务启动失败");
            return false;
        }
        LumaPlayerKaraokeProgressParam progress(&task);
        progress.m_parent = windowHandle();
        progress.m_title = QStringLiteral("保存录音");
        DialogManager::instance().makeDialog(progress);
        LOGINFO("Karaoke export dialog returned done=%d success=%d error=%s", task.done() ? 1 : 0,
            task.succeeded() ? 1 : 0, task.error().toUtf8().constData());
        if (!task.done())
        {
            task.StopTask();
        }
        if (!task.succeeded())
        {
            m_karaokeNotice = task.canceled() ? QStringLiteral("保存已取消；录音仍保留") : QStringLiteral("保存失败：%1；录音仍保留").arg(task.error());
            LOGERROR("Karaoke export failed: %s", task.error().toUtf8().constData());
            update();
            return false;
        }
        m_karaokeNotice = QStringLiteral("录音已保存");
    }
    else if (prompt.m_result == IGNORE_BUTTON)
    {
        m_karaokeNotice = QStringLiteral("录音已放弃");
    }
    else
    {
        m_karaokeNotice = QStringLiteral("录音仍保留");
        update();
        return false;
    }
    m_core.clearKaraokeRecording();
    m_karaokeSessionStarted = false;
    m_karaokeTimelineInitialized = currentMicrophone().m_capture.isRunning();
    setKaraokeControls();
    update();
    return true;
}

void LumaPlayer::onLogicResult(const LumaPlayerLogicResult& result)
{
    if (result.m_type == LumaActionHelpClose || result.m_type == LumaActionWindowApplied)
    {
        return;
    }
    if (result.m_type == LumaActionShutdown)
    {
        if (result.m_error != 0)
        {
            m_closeRequested = false;
            LOGERROR("Shutdown submission failed result=%d", result.m_error);
            return;
        }
        m_closeReady = true;
        m_uiTimer.stop();
        m_clickTimer.stop();
        close();
        return;
    }
    if (m_closeRequested)
    {
        return;
    }
    if (result.m_type == LumaActionVolume)
    {
        if (result.m_requestId == m_volumeRequest && m_volumeRequest != 0)
        {
            m_volumeRequest = 0;
            if (result.m_error == 0 && m_audioRender.setVolumePercent(result.m_point))
            {
                m_appliedVolumePercent = result.m_point;
                submitVolume();
            }
            else
            {
                m_volumePercent = m_appliedVolumePercent;
                LOGERROR("Volume application failed percent=%d", result.m_point);
            }
            update();
        }
        return;
    }
    if (result.m_mediaGeneration < m_mediaGeneration)
    {
        return;
    }
    m_mediaGeneration = result.m_mediaGeneration;
    if (result.m_type == LumaActionHelp)
    {
        showHelpDialog();
        return;
    }
    if (result.m_type == LumaActionMinimize)
    {
        ++m_windowStateSerial;
        showMinimized();
        acknowledgeWindow(LumaActionMinimize, isMinimized());
        return;
    }
    if (result.m_type == LumaActionMaximize)
    {
        applyMaximize();
        return;
    }
    if (result.m_type == LumaActionFullscreen || result.m_type == LumaActionExitFullscreen)
    {
        if (result.m_type == LumaActionFullscreen || isFullScreen())
        {
            applyFullScreen();
        }
        return;
    }
    m_snapshot = result.m_snapshot;
    m_hasMedia = LumaPlayerHelper::isOpenedState(m_snapshot.m_state) && m_snapshot.m_hasVideo;
    if (result.m_type == LumaActionCompleted && result.m_operation == LumaPlayerCoreCOperationRate && result.m_error == 0)
    {
        m_rateTipPermille = m_snapshot.m_ratePermille;
        m_rateTipHideMs = m_elapsedTimer.elapsed() + g_config.m_mouseIdleHideMs;
        m_zoomTipHideMs = 0;
    }
    m_resetEnabled = result.m_dirty;
    if (result.m_revision >= m_viewRevision &&
        result.m_viewportGeneration == m_viewportGeneration)
    {
        m_zoomPercent = result.m_zoom;
        m_panOffset = result.m_pan;
        m_pinned = result.m_pinned;
        m_viewRevision = result.m_revision;
    }
    if (result.m_type == LumaActionReset || result.m_type == LumaActionPan ||
        result.m_type == LumaActionZoom || result.m_type == LumaActionPin)
    {
        LumaPlayerLogicAction applied;
        applied.m_type = LumaActionViewApplied;
        applied.m_revision = result.m_revision;
        applied.m_flag = m_viewRevision == result.m_revision &&
            m_zoomPercent == result.m_zoom && m_panOffset == result.m_pan;
        m_logic.submit(applied);
        if (result.m_type == LumaActionZoom)
        {
            m_zoomTipHideMs = m_elapsedTimer.elapsed() + g_config.m_mouseIdleHideMs;
            m_rateTipHideMs = 0;
        }
    }
    if (result.m_type == LumaActionLoad)
    {
        m_pendingMediaLoad = result.m_requestId == 0;
        if (!m_pendingMediaLoad && result.m_error == 0)
        {
            m_cachedFrame = QImage();
            m_scaledFrame = QImage();
            m_scaledFrameSourceKey = 0;
            m_cachedFrameSerial = 0;
            m_lastActiveLoopPoint = -1;
            fitWindowToMedia();
        }
    }
    if (result.m_type == LumaActionCompleted)
    {
        if (result.m_operation == LumaPlayerCoreCOperationKaraokeRecording && m_karaokeRecording &&
            result.m_inputSerial == m_karaokeStartInput)
        {
            m_karaokeRecordingReady = result.m_error == 0;
            if (!m_karaokeRecordingReady)
            {
                m_karaokeRecording = false;
                postCore(LumaPlayerCoreCOperationKaraokeRecording, 0);
                m_karaokeNotice = QStringLiteral("录制启动失败，未开始写入");
            }
            setKaraokeControls();
        }
        if (result.m_operation == LumaPlayerCoreCOperationPause && result.m_error != 0 &&
            m_isDraggingProgress && result.m_inputSerial == m_lastSeekInput)
        {
            m_isDraggingProgress = false;
            m_hasDragPosition = false;
            m_progressWasPlaying = false;
        }
        if (result.m_operation == LumaPlayerCoreCOperationOpen)
        {
            m_pendingMediaLoad = false;
        }
        if (result.m_operation == LumaPlayerCoreCOperationSeek && !m_isDraggingProgress && result.m_inputSerial == m_lastSeekInput)
        {
            // 完成回报到达时同步取已发布帧，避免先退回旧缓存位置再跳到最终帧。
            m_videoRender.copyFrame(&m_cachedFrame, &m_cachedFrameStart,
                &m_cachedFrameEnd, &m_cachedFrameSerial);
            m_hasDragPosition = false;
        }
        if (result.m_operation == LumaPlayerCoreCOperationMoveLoop && result.m_inputSerial == m_lastMoveInput)
        {
            m_pendingLoopMovePoint = -1;
            if (result.m_error == 0)
            {
                m_lastActiveLoopPoint = result.m_point;
                const int64_t time = result.m_point == 0 ? m_snapshot.m_loopAStart100ns : m_snapshot.m_loopBEnd100ns;
                QCursor::setPos(mapToGlobal(QPoint(progressTimeToX(time), progressTrackRect().center().y())));
            }
        }
        if (result.m_error == 0 && (result.m_operation == LumaPlayerCoreCOperationSetA || result.m_operation == LumaPlayerCoreCOperationSetB))
        {
            m_lastActiveLoopPoint = result.m_point;
        }
        if (result.m_error == 0 && result.m_operation == LumaPlayerCoreCOperationClearLoop)
        {
            m_lastActiveLoopPoint = -1;
        }
    }
    if (result.m_error != 0)
    {
        LOGERROR("GUI operation failed, operation=%d request=%llu result=%d, %s",
            result.m_operation, static_cast<unsigned long long>(result.m_requestId),
            result.m_error, lumaPlayerCoreResultDescription(result.m_error));
    }
    updateOverlayTargets();
    update();
}

QRect LumaPlayer::videoViewportRect() const
{
    return m_pinned ? rect().adjusted(0, topOverlayHeight(), 0, -bottomOverlayHeight()) : rect();
}

QRect LumaPlayer::resetButtonRect() const
{
    return loadButtonRect().translated(g_config.m_titleButtonSize + g_config.m_titleButtonGap, 0);
}

QRect LumaPlayer::helpButtonRect() const
{
    return minimizeButtonRect().translated(-g_config.m_titleButtonSize - g_config.m_titleButtonGap - g_config.m_windowButtonGroupGap, 0);
}

QRect LumaPlayer::pinButtonRect() const
{
    return helpButtonRect().translated(-g_config.m_titleButtonSize - g_config.m_titleButtonGap, 0);
}

void LumaPlayer::showHelpDialog()
{
    hideVolumePopup();
    cancelDeferredInput();
    LumaPlayerHelpParam param(&m_logic);
    param.m_title = g_config.m_helpTitle;
    param.m_parent = windowHandle();
    param.m_businessId = CUSTOM_DIALOG_TYPE_BEGIN;
    param.m_userId = static_cast<quint64>(reinterpret_cast<uintptr_t>(this));
    DialogManager::instance().makeDialog(param);
    acknowledgeWindow(LumaActionHelp, param.m_dialogId != 0);
}

void LumaPlayer::acknowledgeWindow(LumaPlayerActionType type, bool success)
{
    LumaPlayerLogicAction action;
    action.m_type = LumaActionWindowApplied;
    action.m_point = static_cast<int32_t>(type);
    action.m_flag = success;
    m_logic.submit(action);
}