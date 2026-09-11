#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayer.h"
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
#include <QMenu>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QToolTip>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <stdint.h>

LumaPlayer::LumaPlayer(bool debugEnabled, QWidget* parent) :
QWidget(parent),
m_audioRender(),
m_videoRender(),
m_core(&m_audioRender, &m_videoRender),
m_logic(&m_core),
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
m_baseDisplayScale(1.0),
m_panOffset(0.0, 0.0),
m_pressGlobalPos(0, 0),
m_pressWindowGeometry(),
m_pressPanOffset(0.0, 0.0),
m_dragWindow(false),
m_dragVideo(false),
m_dragProgress(false),
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
	m_uiTimer.stop();
	m_clickTimer.stop();
    LOGINFO("LumaPlayer destroyed");
}

void LumaPlayer::init(bool debugEnabled)
{
    LOGINFO("LumaPlayer init begin, debug=%d", debugEnabled ? 1 : 0);
    setWindowTitle(g_config.m_windowTitle);
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
	paintZoomTip(painter);
}

void LumaPlayer::resizeEvent(QResizeEvent* event)
{
    LumaPlayerLogicAction viewport;
    viewport.m_type = LumaActionViewport;
    viewport.m_viewportGeneration = ++m_viewportGeneration;
    m_logic.submit(viewport);
	QWidget::resizeEvent(event);
	updateLoopMarkerHover(m_lastMousePos);
}

void LumaPlayer::mouseMoveEvent(QMouseEvent* event)
{
	m_mouseInside = true;
	m_lastMouseMoveMs = m_elapsedTimer.elapsed();
	m_lastMousePos = event->pos();
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
	if (m_dragProgress)
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
		m_dragProgress = true;
		m_hasDragPosition = true;
		m_dragPosition100ns = progressPointToTime100ns(event->pos());
		m_lastPreviewRequestMs = 0;
		m_lastPreviewRequestPosition100ns = -1;
		m_progressWasPlaying = LumaPlayerHelper::isPlayingState(m_snapshot.m_state);
        m_lastPreviewRequestMs = m_elapsedTimer.elapsed();
        m_lastPreviewRequestPosition100ns = m_dragPosition100ns;
        postAction(LumaActionDragBegin, m_dragPosition100ns);
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
	if (m_dragProgress)
	{
		commitSeekByProgressPoint(event->pos(), m_progressWasPlaying);
		m_dragProgress = false;
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
	if (m_hasMedia)
	{
		togglePlayPause();
	}
}

void LumaPlayer::wheelEvent(QWheelEvent* event)
{
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
	if ((event->modifiers() & Qt::ControlModifier) != 0)
	{
		if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
		{
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
	QWidget::keyPressEvent(event);
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
    QWidget::keyReleaseEvent(event);
}

void LumaPlayer::focusOutEvent(QFocusEvent* event)
{
    m_leftPressed = false;
    QToolTip::hideText();
    postAction(LumaActionCancelMove);
    m_pendingLoopMovePoint = -1;
    m_loopMoveKey = 0;
    m_loopMoveRepeating = false;
    m_clickTimer.stop();
    m_dragWindow = false;
    m_dragVideo = false;
    m_resizeWindow = false;
    m_resizeEdge = ResizeNone;
    if (m_dragProgress)
    {
        commitSeekByProgressPoint(m_lastMousePos, false);
        m_dragProgress = false;
        m_progressWasPlaying = false;
    }
    m_pressArea = HitNone;
    QWidget::focusOutEvent(event);
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
    if (m_dragProgress)
    {
        previewByProgressPoint(m_lastMousePos);
    }
	const LumaPlayerCoreCSnapshot previous = m_snapshot;
	const uint64_t previousFrame = m_cachedFrameSerial;
	const bool previousDragPosition = m_hasDragPosition;
	const int32_t previousHover = m_hoverLoopPoint;
    postAction(LumaActionPoll);
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
	const bool zoomTip = nowMs < m_zoomTipHideMs;
	const bool changed = previous.m_state != m_snapshot.m_state || previous.m_position100ns != m_snapshot.m_position100ns ||
		previous.m_duration100ns != m_snapshot.m_duration100ns || previous.m_ratePermille != m_snapshot.m_ratePermille ||
		previous.m_hasLoopA != m_snapshot.m_hasLoopA || previous.m_hasLoopB != m_snapshot.m_hasLoopB ||
		previous.m_loopAStart100ns != m_snapshot.m_loopAStart100ns || previous.m_loopBEnd100ns != m_snapshot.m_loopBEnd100ns ||
		previousFrame != m_cachedFrameSerial || previousDragPosition != m_hasDragPosition || previousHover != m_hoverLoopPoint ||
		centerTip != m_lastCenterTipVisible || zoomTip != m_lastZoomTipVisible;
	m_lastCenterTipVisible = centerTip;
	m_lastZoomTipVisible = zoomTip;
	if (changed)
	{
		update();
	}
}

void LumaPlayer::openFileDialog()
{
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
    m_clickTimer.stop();
    m_loopMoveKey = 0;
    m_loopMoveRepeating = false;
    m_pendingLoopMovePoint = -1;
    m_dragProgress = false;
    m_dragVideo = false;
    m_hasDragPosition = false;
    LumaPlayerLogicAction action;
    action.m_type = LumaActionLoad;
    action.m_path = filePath;
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
    QSize maxSize((std::max)(g_config.m_minWindowWidth, static_cast<int32_t>(availableRect.width() * g_config.m_initialDesktopFraction)), (std::max)(g_config.m_minWindowHeight,
        static_cast<int32_t>(availableRect.height() * g_config.m_initialDesktopFraction)));
	double scale = 1.0;
	if (videoSize.width() > maxSize.width() || videoSize.height() > maxSize.height())
	{
        scale = g_config.m_largeMediaInitialScale;
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
    QSize targetSize((std::max)(g_config.m_minWindowWidth, static_cast<int32_t>(availableRect.width() * g_config.m_initialDesktopFraction)), (std::max)(g_config.m_minWindowHeight,
        static_cast<int32_t>(availableRect.height() * g_config.m_initialDesktopFraction)));
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
    QTimer::singleShot(0, this, [this, serial, type, state]() {
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
        const bool success = actual == state && (state != Qt::WindowNoState ||
            !m_normalWindowGeometry.isValid() || geometry() == m_normalWindowGeometry);
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
    if (!m_dragProgress || !m_hasMedia || m_snapshot.m_duration100ns <= 0)
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
    m_hasDragPosition = m_dragProgress;
    if (m_dragProgress)
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
	QMenu menu(this);
	menu.setFont(font());
	menu.setWindowFlags(menu.windowFlags() | Qt::WindowStaysOnTopHint);
    menu.setStyleSheet(g_config.menuStyleSheet());
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
	bool bottomActive = m_fileDialogActive || m_progressMenuActive || m_dragProgress || nearBottom || overBottom;
	if (topActive)
	{
		m_lastTopOverlayActiveMs = nowMs;
	}
	if (bottomActive)
	{
		m_lastBottomOverlayActiveMs = nowMs;
	}
	m_topTargetVisible = topActive || (m_lastTopOverlayActiveMs > 0 && nowMs - m_lastTopOverlayActiveMs <= g_config.m_mouseIdleHideMs);
    m_bottomTargetVisible = m_hasMedia && (bottomActive ||
        (m_lastBottomOverlayActiveMs > 0 && nowMs - m_lastBottomOverlayActiveMs <= g_config.m_mouseIdleHideMs));
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
    if (m_dragProgress || m_hasDragPosition)
    {
        return m_dragPosition100ns;
    }
    if (!m_cachedFrame.isNull() && m_cachedFrameEnd > m_cachedFrameStart)
    {
        if (m_dragProgress || !LumaPlayerHelper::isPlayingState(m_snapshot.m_state))
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
    return QRect(g_config.m_titleMargin, topOverlayRect().top() + g_config.m_titleButtonTop,
        g_config.m_titleButtonSize, g_config.m_titleButtonSize);
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
            timeWidth - g_config.m_trackExtension) + g_config.m_timeTrackGap;
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
    paintTitleButton(painter, loadButtonRect(), HitLoadButton);
    paintTitleButton(painter, resetButtonRect(), HitResetButton);
    paintTitleButton(painter, pinButtonRect(), HitPinButton);
    paintTitleButton(painter, helpButtonRect(), HitHelpButton);
	paintTitleButton(painter, minimizeButtonRect(), HitMinButton);
	paintTitleButton(painter, maximizeButtonRect(), HitMaxButton);
	paintTitleButton(painter, closeButtonRect(), HitCloseButton);
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
    const QPoint knob(progressX, track.center().y());
    const bool hoverKnob = m_mouseInside &&
        (m_lastMousePos - knob).manhattanLength() <= 2 * g_config.m_positionDragRadius;
    const int32_t radius = hoverKnob && !m_dragProgress ?
        g_config.m_positionDragRadius : g_config.m_positionRadius;
    painter.drawEllipse(knob, radius, radius);
    if (m_dragProgress)
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
	if (m_elapsedTimer.elapsed() > m_zoomTipHideMs)
	{
		return;
	}
    QRect tipRect(videoViewportRect().center() - QPoint(g_config.m_zoomTipWidth / 2, g_config.m_zoomTipHeight / 2), QSize(g_config.m_zoomTipWidth,
        g_config.m_zoomTipHeight));
	painter.save();
	painter.setPen(Qt::NoPen);
    painter.setBrush(g_config.m_tipBackgroundColor);
    painter.drawRoundedRect(tipRect, g_config.m_zoomTipRadius, g_config.m_zoomTipRadius);
	QFont tipFont = font();
    tipFont.setPointSize(g_config.m_zoomTipFontSize);
	painter.setFont(tipFont);
    painter.setPen(g_config.m_zoomTextColor);
    painter.drawText(tipRect, Qt::AlignCenter, QString::fromStdString(CStringManager::Format(g_config.m_zoomTextFormat.c_str(), m_zoomPercent)));
	painter.restore();
}

void LumaPlayer::paintTitleButton(QPainter& painter, const QRect& rect, HitArea hitArea)
{
    const bool enabled = hitArea != HitResetButton || m_resetEnabled;
    const bool hover = enabled && m_hoverArea == hitArea;
    painter.save();
    painter.setPen(Qt::NoPen);
    const bool pressed = enabled && m_mouseInside && m_leftPressed && m_pressArea == hitArea && rect.contains(m_lastMousePos);
    painter.setBrush(hitArea == HitCloseButton ?
        (pressed ? g_config.m_closeButtonColor.darker(g_config.m_closePressDarkening) : g_config.m_closeButtonColor) :
        (hover ? g_config.m_buttonHoverColor : g_config.m_buttonColor));
    painter.drawRoundedRect(rect, g_config.m_cornerRadius, g_config.m_cornerRadius);
    if (pressed && hitArea != HitCloseButton)
    {
        painter.setOpacity(g_config.m_buttonPressOpacity);
        painter.setBrush(g_config.m_buttonPressColor);
        painter.drawRoundedRect(rect, g_config.m_cornerRadius, g_config.m_cornerRadius);
        painter.setOpacity(1.0);
    }
    const bool blue = hitArea == HitHelpButton || (hitArea == HitPinButton && m_pinned);
    QPen pen(!enabled ? g_config.m_disabledColor : blue ? g_config.m_themeColor : g_config.m_iconColor);
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
        g_config.m_markerLabelHeight), Qt::AlignCenter, label);
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
            labelWidth, labelHeight), Qt::AlignCenter, text);
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
    if (m_closeReady)
    {
        event->accept();
        return;
    }
    event->ignore();
    postAction(LumaActionShutdown);
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
    if (result.m_type == LumaActionFullscreen)
    {
        applyFullScreen();
        return;
    }
    m_snapshot = result.m_snapshot;
    m_hasMedia = LumaPlayerHelper::isOpenedState(m_snapshot.m_state) && m_snapshot.m_hasVideo;
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
        if (result.m_operation == LumaPlayerCoreCOperationPause && result.m_error != 0 &&
            m_dragProgress && result.m_inputSerial == m_lastSeekInput)
        {
            m_dragProgress = false;
            m_hasDragPosition = false;
            m_progressWasPlaying = false;
        }
        if (result.m_operation == LumaPlayerCoreCOperationOpen)
        {
            m_pendingMediaLoad = false;
        }
        if (result.m_operation == LumaPlayerCoreCOperationSeek && !m_dragProgress && result.m_inputSerial == m_lastSeekInput)
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
        if (result.m_error == 0 && (result.m_operation == LumaPlayerCoreCOperationSetA ||
            result.m_operation == LumaPlayerCoreCOperationSetB))
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