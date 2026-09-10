#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayer.h"
#include "Config.h"
#include "LumaPlayerHelper.h"

#include "LumaPlayerLogger.h"

#include <QAction>
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

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

LumaPlayer::LumaPlayer(bool debugEnabled, QWidget* parent) :
QWidget(parent),
m_audioRender(),
m_videoRender(),
m_core(&m_audioRender, &m_videoRender),
m_lastCenterTipVisible(false),
m_lastZoomTipVisible(false),
m_snapshot(),
m_scaledFrameSourceKey(0),
m_cachedFrameSerial(0),
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
m_wasMaximizedBeforeFullScreen(false),
m_hoverArea(HitNone),
m_pressArea(HitNone),
m_hoverLoopPoint(-1),
m_lastActiveLoopPoint(-1),
m_loopMoveKey(0),
m_loopMoveRepeating(false),
m_pendingLoopMovePoint(-1),
m_pendingLoopMoveReference100ns(0),
m_pendingLoopMoveDeadlineMs(0),
m_cancelClickToggle(false),
m_normalGeometryBeforeFullScreen()
{
	init(debugEnabled);
}

LumaPlayer::~LumaPlayer()
{
	m_uiTimer.stop();
	m_clickTimer.stop();
	m_core.uninit();
	m_audioRender.uninit();
	LumaPlayerLogger::log("LumaPlayer destroyed");
}

void LumaPlayer::init(bool debugEnabled)
{
	LumaPlayerLogger::init(debugEnabled);
	LumaPlayerLogger::log("LumaPlayer init begin, debug=%d", debugEnabled ? 1 : 0);
	setWindowTitle(LumaPlayerHelper::qcn(L"流光播放器"));
	setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);
	setMinimumSize(g_config.m_minWindowWidth, g_config.m_minWindowHeight);
	setFont(QFont(LumaPlayerHelper::qcn(L"楷体"), 10));

	m_elapsedTimer.start();
	m_lastMouseMoveMs = m_elapsedTimer.elapsed();
	int32_t initResult = m_core.init(debugEnabled);
	if (initResult != LumaPlayerCoreCResultSuccess)
	{
		LumaPlayerLogger::log("Core初始化失败，result=%d，%s", initResult, lumaPlayerCoreResultDescription(initResult));
	}
	m_snapshot = m_core.snapshot();
	fitInitialWindowToDesktop();

	m_clickTimer.setSingleShot(true);
	connect(&m_clickTimer, SIGNAL(timeout()), this, SLOT(onVideoClick()));
	connect(&m_uiTimer, SIGNAL(timeout()), this, SLOT(onUiTimer()));
	m_uiTimer.start(16);
	LumaPlayerLogger::log("LumaPlayer init end");
}

void LumaPlayer::paintEvent(QPaintEvent* event)
{
	(void)event;
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
	// 图形抗锯齿不处理图像采样，视频缩放需单独启用平滑变换
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	painter.fillRect(rect(), QColor(0, 0, 0));
	paintVideo(painter);
	paintCenterPlayTip(painter);
	paintTopOverlay(painter);
	paintBottomOverlay(painter);
	paintZoomTip(painter);
}

void LumaPlayer::resizeEvent(QResizeEvent* event)
{
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
		m_panOffset = m_pressPanOffset + QPointF(delta);
		m_cancelClickToggle = true;
		update();
		return;
	}
	m_hoverArea = hitTest(event->pos());
	updateLoopMarkerHover(event->pos());
	if (m_hoverArea == HitLoadButton)
	{
		QToolTip::showText(mapToGlobal(loadButtonRect().bottomLeft()), LumaPlayerHelper::qcn(L"加载视频"), this);
	}
	else
	{
		QToolTip::hideText();
	}
	updateCursorShape(event->pos());
	updateOverlayTargets();
	update();
}

void LumaPlayer::mousePressEvent(QMouseEvent* event)
{
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
	if (event->button() == Qt::RightButton && progressTrackRect().adjusted(0, -1, 0, 1).contains(event->pos()))
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
		if (m_progressWasPlaying)
		{
			m_core.pauseAsync();
		m_snapshot.m_state = LumaPlayerCoreCStatePaused;
		}
		previewByProgressPoint(event->pos());
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
	else if (m_pressArea == HitMinButton && releaseArea == HitMinButton)
	{
		showMinimized();
	}
	else if (m_pressArea == HitMaxButton && releaseArea == HitMaxButton)
	{
		if (isFullScreen())
		{
			toggleFullScreen();
		}
		else
		{
			toggleMaximize();
		}
	}
	else if (m_pressArea == HitCloseButton && releaseArea == HitCloseButton)
	{
		close();
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
	int step = (QApplication::keyboardModifiers() & Qt::ControlModifier) ? 1 : 5;
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
			int64_t rate = m_snapshot.m_ratePermille;
			if (event->key() == Qt::Key_Up)
			{
				rate += 100;
			}
			else
			{
				rate -= 100;
			}
			rate = std::max<int64_t>(g_config.m_coreConfig.m_minRatePermille,
				std::min<int64_t>(rate, g_config.m_coreConfig.m_maxRatePermille));
			m_core.setPlaybackRatePermilleAsync(static_cast<int32_t>(rate));
			m_snapshot.m_ratePermille = static_cast<int32_t>(rate);
			LumaPlayerLogger::log("Playback rate changed by key, ratePermille=%d", static_cast<int32_t>(rate));
			event->accept();
			return;
		}
	}
	if (m_hoverLoopPoint >= 0 && (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right))
	{
        if (m_loopMoveKey != event->key() || (m_pendingLoopMovePoint >= 0 && m_pendingLoopMovePoint != m_hoverLoopPoint))
        {
            m_core.cancelLoopPointMove();
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
                m_core.cancelLoopPointMove();
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
    m_core.cancelLoopPointMove();
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
	(void)event;
	m_mouseInside = false;
	m_lastMouseMoveMs = m_elapsedTimer.elapsed();
}

void LumaPlayer::onUiTimer()
{
	const LumaPlayerCoreCSnapshot previous = m_snapshot;
	const uint64_t previousFrame = m_cachedFrameSerial;
	const bool previousDragPosition = m_hasDragPosition;
	const int32_t previousHover = m_hoverLoopPoint;
	int32_t previousResult = m_snapshot.m_result;
	m_snapshot = m_core.snapshot();
	if (m_snapshot.m_result != previousResult && m_snapshot.m_result != LumaPlayerCoreCResultSuccess)
	{
		LumaPlayerLogger::log("后台播放操作失败，result=%d，%s", m_snapshot.m_result, lumaPlayerCoreResultDescription(m_snapshot.m_result));
	}
	if (m_pendingMediaLoad)
	{
		bool isRequestedMedia = m_snapshot.m_mediaSerial != m_pendingMediaSerial;
		if (m_snapshot.m_state == LumaPlayerCoreCStatePaused && isRequestedMedia)
		{
			m_pendingMediaLoad = false;
			m_hasMedia = m_snapshot.m_hasVideo;
			if (m_hasMedia)
			{
				fitWindowToMedia();
			}
			int32_t playResult = m_core.playAsync();
			if (playResult == LumaPlayerCoreCResultSuccess)
			{
				m_snapshot.m_state = LumaPlayerCoreCStatePlaying;
			}
			LumaPlayerLogger::log("Load media completed, playPostResult=%d, duration=%lld, size=%dx%d",
				static_cast<int>(playResult),
				static_cast<long long>(m_snapshot.m_duration100ns),
				m_snapshot.m_videoWidth,
				m_snapshot.m_videoHeight);
		}
		else if (m_snapshot.m_state == LumaPlayerCoreCStateError)
		{
			m_pendingMediaLoad = false;
			LumaPlayerLogger::log("异步打开媒体失败，result=%d，%s", m_snapshot.m_result, lumaPlayerCoreResultDescription(m_snapshot.m_result));
		}
	}
	m_hasMedia = LumaPlayerHelper::isOpenedState(m_snapshot.m_state) && m_snapshot.m_hasVideo;
	updatePendingLoopMove();
	if (m_hasDragPosition && !m_dragProgress)
	{
		int64_t threshold = std::max<int64_t>(m_snapshot.m_defaultVideoFrameDuration100ns, 100000);
		int64_t diff = m_snapshot.m_position100ns - m_dragPosition100ns;
		if (diff < 0)
		{
			diff = -diff;
		}
		if (diff <= threshold || m_snapshot.m_state == LumaPlayerCoreCStatePlaying ||
			m_snapshot.m_state == LumaPlayerCoreCStateError || m_snapshot.m_state == LumaPlayerCoreCStateClosed)
		{
			m_hasDragPosition = false;
		}
	}
	uint64_t frameSerial = m_videoRender.frameSerial();
	if (frameSerial != m_cachedFrameSerial)
	{
		QImage frame;
		int64_t frameStart = 0;
		int64_t frameEnd = 0;
		m_videoRender.copyFrame(&frame, &frameStart, &frameEnd, &frameSerial);
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
	LumaPlayerLogger::log("Open file dialog begin");
	QString filePath = QFileDialog::getOpenFileName(this, LumaPlayerHelper::qcn(L"加载视频"), QString(), LumaPlayerHelper::qcn(L"视频文件 (*.mp4 *.avi);;所有文件 (*.*)"));
	m_fileDialogActive = false;
	updateOverlayTargets();
	if (filePath.isEmpty())
	{
		LumaPlayerLogger::log("Open file dialog canceled");
		return;
	}
	loadMedia(filePath);
}

void LumaPlayer::loadMedia(const QString& filePath)
{
	QByteArray utf8Path = filePath.toUtf8();
	if (utf8Path.isEmpty())
	{
		return;
	}
	m_clickTimer.stop();
	m_core.cancelLoopPointMove();
	m_loopMoveKey = 0;
	m_loopMoveRepeating = false;
	m_pendingLoopMovePoint = -1;
	m_hoverLoopPoint = -1;
	m_lastActiveLoopPoint = -1;
	m_dragProgress = false;
	m_dragVideo = false;
	m_dragWindow = false;
	m_resizeWindow = false;
	m_pressArea = HitNone;
	LumaPlayerLogger::log("Load media command begin: %s", utf8Path.constData());
	m_core.pauseAsync();
	m_cachedFrame = QImage();
	m_scaledFrame = QImage();
	m_scaledFrameSourceKey = 0;
	m_cachedFrameSerial = 0;
	m_panOffset = QPointF(0.0, 0.0);
	m_zoomPercent = 100;
	m_baseDisplayScale = 1.0;
	m_hasMedia = false;
	m_hasDragPosition = false;
	m_lastPreviewRequestMs = 0;
	m_lastPreviewRequestPosition100ns = -1;
	m_pendingMediaLoad = true;
	m_pendingMediaSerial = m_snapshot.m_mediaSerial;
	std::string mediaPathUtf8(utf8Path.constData(), static_cast<size_t>(utf8Path.size()));
	int32_t result = m_core.openMediaAsync(mediaPathUtf8);
	if (result != LumaPlayerCoreCResultSuccess)
	{
		m_pendingMediaLoad = false;
		std::string errorText = m_core.lastError();
		LumaPlayerLogger::log("打开媒体命令提交失败，result=%d，%s", result, lumaPlayerCoreResultDescription(result));
		return;
	}
	m_snapshot.m_state = LumaPlayerCoreCStateOpening;
	LumaPlayerLogger::log("Load media command posted: %s", utf8Path.constData());
	update();
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
	QRect availableRect = desktop != nullptr ? desktop->screenGeometry(this) : QRect(0, 0, 1280, 720);
	QSize maxSize(std::max(g_config.m_minWindowWidth, availableRect.width() / 2), std::max(g_config.m_minWindowHeight, availableRect.height() / 2));
	double scale = 1.0;
	if (videoSize.width() > maxSize.width() || videoSize.height() > maxSize.height())
	{
		scale = 0.5;
	}
	if (scale <= 0.0)
	{
		scale = 1.0;
	}
	m_baseDisplayScale = scale;
	QSize targetSize(std::max(g_config.m_minWindowWidth, static_cast<int>(videoSize.width() * scale + 0.5)), std::max(g_config.m_minWindowHeight, static_cast<int>(videoSize.height() * scale + 0.5)));
	QPoint topLeft(availableRect.left() + (availableRect.width() - targetSize.width()) / 2, availableRect.top() + (availableRect.height() - targetSize.height()) / 2);
	setGeometry(QRect(topLeft, targetSize));
	m_panOffset = QPointF(0.0, 0.0);
	m_zoomPercent = 100;
	LumaPlayerLogger::log("Window fitted to media, window=%dx%d, scale=%.4f", targetSize.width(), targetSize.height(), m_baseDisplayScale);
}

void LumaPlayer::fitInitialWindowToDesktop()
{
	QDesktopWidget* desktop = QApplication::desktop();
	QRect availableRect = desktop != nullptr ? desktop->screenGeometry(this) : QRect(0, 0, 1280, 720);
	QSize targetSize(std::max(g_config.m_minWindowWidth, availableRect.width() / 2), std::max(g_config.m_minWindowHeight, availableRect.height() / 2));
	QPoint topLeft(availableRect.left() + (availableRect.width() - targetSize.width()) / 2, availableRect.top() + (availableRect.height() - targetSize.height()) / 2);
	setGeometry(QRect(topLeft, targetSize));
	LumaPlayerLogger::log("Initial window fitted, window=%dx%d", targetSize.width(), targetSize.height());
}

void LumaPlayer::togglePlayPause()
{
	if (!m_hasMedia)
	{
		return;
	}
	m_snapshot = m_core.snapshot();
	if (LumaPlayerHelper::isPlayingState(m_snapshot.m_state))
	{
		m_core.pauseAsync();
		m_snapshot.m_state = LumaPlayerCoreCStatePaused;
		LumaPlayerLogger::log("Pause by UI");
	}
	else
	{
		m_core.playAsync();
		m_snapshot.m_state = LumaPlayerCoreCStatePlaying;
		LumaPlayerLogger::log("Play by UI");
	}
}

void LumaPlayer::toggleFullScreen()
{
	if (isFullScreen())
	{
		showNormal();
		if (m_wasMaximizedBeforeFullScreen)
		{
			showMaximized();
		}
		else if (m_normalGeometryBeforeFullScreen.isValid())
		{
			setGeometry(m_normalGeometryBeforeFullScreen);
		}
		LumaPlayerLogger::log("Exit fullscreen");
	}
	else
	{
		m_wasMaximizedBeforeFullScreen = isMaximized();
		m_normalGeometryBeforeFullScreen = isMaximized() ? normalGeometry() : geometry();
		showFullScreen();
		LumaPlayerLogger::log("Enter fullscreen");
	}
}

void LumaPlayer::toggleMaximize()
{
	if (isMaximized())
	{
		showNormal();
		LumaPlayerLogger::log("Window restore from maximized");
	}
	else
	{
		showMaximized();
		LumaPlayerLogger::log("Window maximized");
	}
}

void LumaPlayer::previewByProgressPoint(const QPoint& point)
{
	if (!m_hasMedia || m_snapshot.m_duration100ns <= 0)
	{
		return;
	}
	m_dragPosition100ns = progressPointToTime100ns(point);
	m_hasDragPosition = true;
	int64_t nowMs = m_elapsedTimer.elapsed();
	int64_t frameDuration = m_snapshot.m_defaultVideoFrameDuration100ns;
	if (frameDuration <= 0)
	{
		frameDuration = 100000;
	}
	bool shouldRequestPreview = m_lastPreviewRequestPosition100ns < 0 || nowMs - m_lastPreviewRequestMs >= g_config.m_previewDragIntervalMs;
	int64_t diff = m_dragPosition100ns - m_lastPreviewRequestPosition100ns;
	if (diff < 0)
	{
		diff = -diff;
	}
	if (diff >= frameDuration / 2)
	{
		shouldRequestPreview = true;
	}
	if (shouldRequestPreview)
	{
		m_lastPreviewRequestMs = nowMs;
		m_lastPreviewRequestPosition100ns = m_dragPosition100ns;
		m_core.previewFrameAsync(m_dragPosition100ns);
	}
	update();
}

void LumaPlayer::commitSeekByProgressPoint(const QPoint& point, bool resumeAfterSeek)
{
	if (!m_hasMedia || m_snapshot.m_duration100ns <= 0)
	{
		return;
	}
	m_dragPosition100ns = progressPointToTime100ns(point);
	m_hasDragPosition = true;
	m_core.seekToAsync(m_dragPosition100ns, resumeAfterSeek);
	LumaPlayerLogger::log("Progress seek committed, position=%lld, resume=%d", static_cast<long long>(m_dragPosition100ns), resumeAfterSeek ? 1 : 0);
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
	m_core.previewFrameAsync(position);
	QMenu menu(this);
	menu.setFont(font());
	menu.setWindowFlags(menu.windowFlags() | Qt::WindowStaysOnTopHint);
	menu.setStyleSheet("QMenu{background:rgba(20,20,20,235);color:white;border:1px solid rgba(255,255,255,80);font-family:'KaiTi','楷体';} QMenu::item{padding:5px 26px 5px 20px;} QMenu::item:selected{background:rgba(255,255,255,35);}");
	QAction* setAAction = menu.addAction(LumaPlayerHelper::qcn(L"设置A点"));
	QAction* setBAction = menu.addAction(LumaPlayerHelper::qcn(L"设置B点"));
	QAction* clearAction = menu.addAction(LumaPlayerHelper::qcn(L"清除AB点"));
	QAction* selectedAction = menu.exec(mapToGlobal(point));
	m_progressMenuActive = false;
	if (selectedAction == setAAction)
	{
		int32_t result = m_core.setLoopAAtPositionAsync(position);
		if (result == LumaPlayerCoreCResultSuccess)
		{
			m_lastActiveLoopPoint = 0;
		}
		LumaPlayerLogger::log("Set loop A by progress menu, result=%d, position=%lld", static_cast<int>(result), static_cast<long long>(position));
	}
	else if (selectedAction == setBAction)
	{
		int32_t result = m_core.setLoopBAtPositionAsync(position);
		if (result == LumaPlayerCoreCResultSuccess)
		{
			m_lastActiveLoopPoint = 1;
		}
		LumaPlayerLogger::log("Set loop B by progress menu, result=%d, position=%lld", static_cast<int>(result), static_cast<long long>(position));
	}
	else if (selectedAction == clearAction)
	{
		m_core.clearLoopAsync();
		m_lastActiveLoopPoint = -1;
		LumaPlayerLogger::log("Clear loop by progress menu");
	}
	m_snapshot = m_core.snapshot();
	update();
}

void LumaPlayer::setZoomPercent(int32_t newZoomPercent, const QPoint& anchorPoint)
{
	newZoomPercent = LumaPlayerHelper::clampInt(newZoomPercent, g_config.m_minZoomPercent, g_config.m_maxZoomPercent);
	if (newZoomPercent == m_zoomPercent)
	{
		return;
	}
	QRect oldRect = videoDrawRect();
	QPointF contentPoint(0.5, 0.5);
	if (oldRect.width() > 0 && oldRect.height() > 0)
	{
		contentPoint.setX(static_cast<double>(anchorPoint.x() - oldRect.left()) / static_cast<double>(oldRect.width()));
		contentPoint.setY(static_cast<double>(anchorPoint.y() - oldRect.top()) / static_cast<double>(oldRect.height()));
	}
	m_zoomPercent = newZoomPercent;
	QRect newRect = videoDrawRect();
	QPointF newTopLeft(anchorPoint.x() - contentPoint.x() * newRect.width(), anchorPoint.y() - contentPoint.y() * newRect.height());
	QPointF wantedCenter(newTopLeft.x() + newRect.width() / 2.0, newTopLeft.y() + newRect.height() / 2.0);
	QPointF windowCenter(width() / 2.0, height() / 2.0);
	m_panOffset = wantedCenter - windowCenter;
	m_zoomTipHideMs = m_elapsedTimer.elapsed() + 1000;
	LumaPlayerLogger::log("Zoom changed, zoom=%d, anchor=(%d,%d)", m_zoomPercent, anchorPoint.x(), anchorPoint.y());
	update();
}

void LumaPlayer::moveHoveredLoopPoint(int32_t frameOffset)
{
	if (m_hoverLoopPoint < 0 || frameOffset == 0)
	{
		return;
	}
	int32_t pointType = m_hoverLoopPoint;
	bool pointIsSet = pointType == 0 ? m_snapshot.m_hasLoopA != 0 : m_snapshot.m_hasLoopB != 0;
	if (!pointIsSet)
	{
		return;
	}
	int64_t markerTime100ns = pointType == 0 ? m_snapshot.m_loopAStart100ns : m_snapshot.m_loopBEnd100ns;
    // 暂停独立排在移动之前，即使松键取消了尚未开始的移动，也保持暂停
    if (m_snapshot.m_state == LumaPlayerCoreCStatePlaying)
    {
        int32_t pauseResult = m_core.pauseAsync();
        if (pauseResult != LumaPlayerCoreCResultSuccess)
        {
            LumaPlayerLogger::log("Pause before loop move failed, result=%d", pauseResult);
            return;
        }
        m_snapshot.m_state = LumaPlayerCoreCStatePaused;
    }
	int32_t result = m_core.moveLoopPointAsync(pointType, frameOffset);
	if (result != LumaPlayerCoreCResultSuccess)
	{
		LumaPlayerLogger::log("Move loop point post failed, point=%d, offset=%d, result=%d", pointType, frameOffset, result);
		return;
	}
    // 端点编辑接管时间轴，清除上次拖动尚未收敛的显示位置
    m_hasDragPosition = false;
	m_lastActiveLoopPoint = pointType;
	m_pendingLoopMovePoint = pointType;
	m_pendingLoopMoveReference100ns = markerTime100ns;
	m_pendingLoopMoveDeadlineMs = m_elapsedTimer.elapsed() + 1000;
	int64_t frameDuration100ns = m_snapshot.m_defaultVideoFrameDuration100ns;
	if (frameDuration100ns <= 0)
	{
		frameDuration100ns = 400000;
	}
	int64_t estimatedTime100ns = markerTime100ns + frameDuration100ns * frameOffset;
	estimatedTime100ns = LumaPlayerHelper::clampTime100ns(estimatedTime100ns, 0, m_snapshot.m_duration100ns);
	QRect track = progressTrackRect();
	QCursor::setPos(mapToGlobal(QPoint(progressTimeToX(estimatedTime100ns), track.center().y())));
	LumaPlayerLogger::log("Move loop point posted, point=%d, offset=%d, oldMarker=%lld", pointType, frameOffset, static_cast<long long>(markerTime100ns));
}

void LumaPlayer::updatePendingLoopMove()
{
	if (m_pendingLoopMovePoint < 0)
	{
		return;
	}
	bool pointIsSet = m_pendingLoopMovePoint == 0 ? m_snapshot.m_hasLoopA != 0 : m_snapshot.m_hasLoopB != 0;
	int64_t markerTime100ns = m_pendingLoopMovePoint == 0 ? m_snapshot.m_loopAStart100ns : m_snapshot.m_loopBEnd100ns;
	if (pointIsSet && markerTime100ns != m_pendingLoopMoveReference100ns)
	{
		QRect track = progressTrackRect();
		QCursor::setPos(mapToGlobal(QPoint(progressTimeToX(markerTime100ns), track.center().y())));
		LumaPlayerLogger::log("Move loop point applied, point=%d, marker=%lld", m_pendingLoopMovePoint, static_cast<long long>(markerTime100ns));
		m_pendingLoopMovePoint = -1;
		return;
	}
	if (m_elapsedTimer.elapsed() >= m_pendingLoopMoveDeadlineMs)
	{
		LumaPlayerLogger::log("Move loop point completed without marker change, point=%d", m_pendingLoopMovePoint);
		m_pendingLoopMovePoint = -1;
	}
}

void LumaPlayer::updateOverlayTargets()
{
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
	m_bottomTargetVisible = bottomActive || (m_lastBottomOverlayActiveMs > 0 && nowMs - m_lastBottomOverlayActiveMs <= g_config.m_mouseIdleHideMs);
}

void LumaPlayer::updateOverlayAnimation()
{
	int targetTop = m_topTargetVisible ? topOverlayHeight() : 0;
	int targetBottom = m_bottomTargetVisible ? bottomOverlayHeight() : 0;
	int oldTop = m_topVisibleHeight;
	int oldBottom = m_bottomVisibleHeight;
	if (m_topVisibleHeight < targetTop)
	{
		m_topVisibleHeight = std::min(targetTop, m_topVisibleHeight + g_config.m_overlayStep);
	}
	else if (m_topVisibleHeight > targetTop)
	{
		m_topVisibleHeight = std::max(targetTop, m_topVisibleHeight - g_config.m_overlayStep);
	}
	if (m_bottomVisibleHeight < targetBottom)
	{
		m_bottomVisibleHeight = std::min(targetBottom, m_bottomVisibleHeight + g_config.m_overlayStep);
	}
	else if (m_bottomVisibleHeight > targetBottom)
	{
		m_bottomVisibleHeight = std::max(targetBottom, m_bottomVisibleHeight - g_config.m_overlayStep);
	}
	if (oldTop != m_topVisibleHeight || oldBottom != m_bottomVisibleHeight)
	{
		update();
	}
}

HitArea LumaPlayer::hitTest(const QPoint& point) const
{
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
	if (m_bottomVisibleHeight > 0 && progressTrackRect().adjusted(0, -1, 0, 1).contains(point))
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
		newLeft = std::min(newLeft, m_pressWindowGeometry.right() - g_config.m_minWindowWidth + 1);
		newGeometry.setLeft(newLeft);
	}
	if ((m_resizeEdge & ResizeRight) != 0)
	{
		int newRight = m_pressWindowGeometry.right() + delta.x();
		newRight = std::max(newRight, newGeometry.left() + g_config.m_minWindowWidth - 1);
		newGeometry.setRight(newRight);
	}
	if ((m_resizeEdge & ResizeTop) != 0)
	{
		int newTop = m_pressWindowGeometry.top() + delta.y();
		newTop = std::min(newTop, m_pressWindowGeometry.bottom() - g_config.m_minWindowHeight + 1);
		newGeometry.setTop(newTop);
	}
	if ((m_resizeEdge & ResizeBottom) != 0)
	{
		int newBottom = m_pressWindowGeometry.bottom() + delta.y();
		newBottom = std::max(newBottom, newGeometry.top() + g_config.m_minWindowHeight - 1);
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
	int span = std::max(1, track.width() - 1);
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
	int span = std::max(1, track.width() - 1);
	double ratio = static_cast<double>(position) / static_cast<double>(duration);
	return track.left() + static_cast<int>(ratio * span + 0.5);
}

int64_t LumaPlayer::displayPosition100ns() const
{
	return m_hasDragPosition ? m_dragPosition100ns : m_snapshot.m_position100ns;
}

double LumaPlayer::currentVideoBaseScale(const QSize& videoSize) const
{
	if (!videoSize.isValid())
	{
		return 1.0;
	}
	if (isFullScreen() || isMaximized())
	{
		double widthScale = static_cast<double>(width()) / static_cast<double>(videoSize.width());
		double heightScale = static_cast<double>(height()) / static_cast<double>(videoSize.height());
		double scale = std::min(widthScale, heightScale);
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
	return g_config.m_bottomOverlayHeight;
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
	QRect overlay = topOverlayRect();
	return QRect(overlay.left() + 8, overlay.top() + 4, 20, 20);
}

QRect LumaPlayer::minimizeButtonRect() const
{
	QRect overlay = topOverlayRect();
	return QRect(overlay.right() - 77, overlay.top() + 4, 20, 20);
}

QRect LumaPlayer::maximizeButtonRect() const
{
	QRect overlay = topOverlayRect();
	return QRect(overlay.right() - 51, overlay.top() + 4, 20, 20);
}

QRect LumaPlayer::closeButtonRect() const
{
	QRect overlay = topOverlayRect();
	return QRect(overlay.right() - 25, overlay.top() + 4, 20, 20);
}

QRect LumaPlayer::plusButtonRect() const
{
	int size = std::min(width(), height()) / 8;
	size = LumaPlayerHelper::clampInt(size, 38, 76);
	return QRect((width() - size) / 2, (height() - size) / 2, size, size);
}

QRect LumaPlayer::playButtonRect() const
{
	QRect overlay = bottomOverlayRect();
	int size = g_config.m_playButtonSize;
	return QRect((width() - size) / 2, overlay.bottom() - size - 3, size, size);
}

QRect LumaPlayer::progressTrackRect() const
{
	QRect overlay = bottomOverlayRect();
	int left = 82;
	int right = 82;
	return QRect(left, overlay.top() + g_config.m_progressTrackTop, std::max(1, width() - left - right), 4);
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
	int drawWidth = std::max(1, static_cast<int>(videoSize.width() * scale + 0.5));
	int drawHeight = std::max(1, static_cast<int>(videoSize.height() * scale + 0.5));
	int left = static_cast<int>((width() - drawWidth) / 2.0 + m_panOffset.x() + 0.5);
	int top = static_cast<int>((height() - drawHeight) / 2.0 + m_panOffset.y() + 0.5);
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
	QRect hitBand = track.adjusted(-8, -14, 8, 18);
	if (!hitBand.contains(point))
	{
		return;
	}
	bool hitA = m_snapshot.m_hasLoopA != 0 && std::abs(point.x() - progressTimeToX(m_snapshot.m_loopAStart100ns)) <= 8;
	bool hitB = m_snapshot.m_hasLoopB != 0 && std::abs(point.x() - progressTimeToX(m_snapshot.m_loopBEnd100ns)) <= 8;
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
		QColor circleColor = hover ? QColor(95, 95, 95, 210) : QColor(70, 70, 70, 190);
		QPen pen(QColor(210, 210, 210, 220));
		pen.setWidth(2);
		painter.setPen(pen);
		painter.setBrush(circleColor);
		painter.drawEllipse(plusRect.adjusted(1, 1, -1, -1));
		int cx = plusRect.center().x();
		int cy = plusRect.center().y();
		int len = plusRect.width() / 4;
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
	painter.setBrush(QColor(16, 16, 16, 220));
	painter.drawRect(overlay);
	painter.setPen(QColor(255, 255, 255, 55));
	painter.drawLine(overlay.bottomLeft(), overlay.bottomRight());
	LumaPlayerHelper::paintLoadIcon(painter, loadButtonRect(), m_hoverArea == HitLoadButton);
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
	painter.setBrush(QColor(16, 16, 16, 220));
	painter.drawRect(overlay);
	painter.setPen(QColor(255, 255, 255, 55));
	painter.drawLine(overlay.topLeft(), overlay.topRight());

	QFont textFont = font();
	textFont.setPointSize(8);
	painter.setFont(textFont);
	painter.setPen(QColor(235, 235, 235, 230));
	int64_t displayPosition = displayPosition100ns();
	painter.drawText(QRect(6, overlay.top() + g_config.m_progressTrackTop - 8, 72, 20), Qt::AlignVCenter | Qt::AlignLeft, LumaPlayerHelper::formatTime(displayPosition, false));
	painter.drawText(QRect(width() - 78, overlay.top() + g_config.m_progressTrackTop - 8, 72, 20), Qt::AlignVCenter | Qt::AlignRight, LumaPlayerHelper::formatTime(m_snapshot.m_duration100ns, false));

	QRect track = progressTrackRect();
	int progressX = progressTimeToX(displayPosition);
	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(255, 255, 255, 70));
	painter.drawRoundedRect(track, 2, 2);
	QRect played(track.left(), track.top(), std::max(0, progressX - track.left()), track.height());
	painter.setBrush(QColor(245, 245, 245, 210));
	painter.drawRoundedRect(played, 2, 2);
	painter.setBrush(QColor(255, 255, 255, 240));
	painter.drawEllipse(QPoint(progressX, track.center().y()), m_dragProgress ? 6 : 5, m_dragProgress ? 6 : 5);

	bool drawA = m_snapshot.m_hasLoopA != 0;
	bool drawB = m_snapshot.m_hasLoopB != 0;
	bool drawAOnTop = false;
	if (drawA && drawB)
	{
		int ax = progressTimeToX(m_snapshot.m_loopAStart100ns);
		int bx = progressTimeToX(m_snapshot.m_loopBEnd100ns);
		drawAOnTop = std::abs(ax - bx) <= 1 && m_lastActiveLoopPoint == 0;
	}
	if (drawAOnTop)
	{
		paintLoopMarker(painter, m_snapshot.m_loopBEnd100ns, LumaPlayerHelper::qcn(L"B"), m_hoverLoopPoint == 1);
		paintLoopMarker(painter, m_snapshot.m_loopAStart100ns, LumaPlayerHelper::qcn(L"A"), m_hoverLoopPoint == 0);
	}
	else
	{
		if (drawA)
		{
			paintLoopMarker(painter, m_snapshot.m_loopAStart100ns, LumaPlayerHelper::qcn(L"A"), m_hoverLoopPoint == 0);
		}
		if (drawB)
		{
			paintLoopMarker(painter, m_snapshot.m_loopBEnd100ns, LumaPlayerHelper::qcn(L"B"), m_hoverLoopPoint == 1);
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
	int size = std::min(width(), height()) / 7;
	size = LumaPlayerHelper::clampInt(size, 42, 96);
	QRect circleRect((width() - size) / 2, (height() - size) / 2, size, size);
	painter.save();
	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(0, 0, 0, 95));
	painter.drawEllipse(circleRect);
	QPoint p1(circleRect.left() + size * 40 / 100, circleRect.top() + size * 30 / 100);
	QPoint p2(circleRect.left() + size * 40 / 100, circleRect.top() + size * 70 / 100);
	QPoint p3(circleRect.left() + size * 72 / 100, circleRect.top() + size * 50 / 100);
	QPoint triangle[3] = { p1, p2, p3 };
	painter.setBrush(QColor(255, 255, 255, 175));
	painter.drawPolygon(triangle, 3);
	painter.restore();
}

void LumaPlayer::paintZoomTip(QPainter& painter)
{
	if (m_elapsedTimer.elapsed() > m_zoomTipHideMs)
	{
		return;
	}
	QRect tipRect(width() / 2 - 42, height() / 2 - 16, 84, 32);
	painter.save();
	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(0, 0, 0, 180));
	painter.drawRoundedRect(tipRect, 4, 4);
	QFont tipFont = font();
	tipFont.setPointSize(11);
	painter.setFont(tipFont);
	painter.setPen(QColor(255, 255, 255, 235));
	painter.drawText(tipRect, Qt::AlignCenter, QString("%1%").arg(m_zoomPercent));
	painter.restore();
}

void LumaPlayer::paintTitleButton(QPainter& painter, const QRect& rect, HitArea hitArea)
{
	bool hover = m_hoverArea == hitArea;
	painter.save();
	painter.setPen(Qt::NoPen);
	painter.setBrush(hover ? QColor(255, 255, 255, 38) : QColor(255, 255, 255, 8));
	painter.drawRoundedRect(rect, 3, 3);
	QPen pen(hitArea == HitCloseButton && hover ? QColor(255, 120, 120, 240) : QColor(245, 245, 245, 220));
	pen.setWidth(2);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	if (hitArea == HitMinButton)
	{
		painter.drawLine(QPoint(rect.left() + 5, rect.center().y() + 4), QPoint(rect.right() - 5, rect.center().y() + 4));
	}
	else if (hitArea == HitMaxButton)
	{
		if (isMaximized() || isFullScreen())
		{
			QRect back(rect.left() + 6, rect.top() + 5, 8, 8);
			QRect front(rect.left() + 4, rect.top() + 8, 8, 8);
			painter.drawRect(back);
			painter.drawRect(front);
		}
		else
		{
			painter.drawRect(rect.adjusted(5, 5, -5, -5));
		}
	}
	else if (hitArea == HitCloseButton)
	{
		painter.drawLine(rect.left() + 6, rect.top() + 6, rect.right() - 6, rect.bottom() - 6);
		painter.drawLine(rect.right() - 6, rect.top() + 6, rect.left() + 6, rect.bottom() - 6);
	}
	painter.restore();
}

void LumaPlayer::paintPlayIcon(QPainter& painter, const QRect& rect)
{
	bool hover = m_hoverArea == HitPlayButton;
	painter.save();
	painter.setPen(Qt::NoPen);
	painter.setBrush(hover ? QColor(255, 255, 255, 45) : QColor(255, 255, 255, 18));
	painter.drawEllipse(rect);
	painter.setBrush(QColor(255, 255, 255, 230));
	if (LumaPlayerHelper::isPlayingState(m_snapshot.m_state))
	{
		painter.drawRect(QRect(rect.left() + rect.width() * 3 / 10, rect.top() + rect.height() / 4, rect.width() / 7, rect.height() / 2));
		painter.drawRect(QRect(rect.left() + rect.width() * 6 / 10, rect.top() + rect.height() / 4, rect.width() / 7, rect.height() / 2));
	}
	else
	{
		QPoint p1(rect.left() + rect.width() / 3, rect.top() + rect.height() / 4);
		QPoint p2(rect.left() + rect.width() / 3, rect.bottom() - rect.height() / 4);
		QPoint p3(rect.right() - rect.width() / 4, rect.center().y());
		QPoint triangle[3] = { p1, p2, p3 };
		painter.drawPolygon(triangle, 3);
	}
	painter.restore();
}

void LumaPlayer::paintLoopMarker(QPainter& painter, int64_t markerTime100ns, const QString& label, bool hovered)
{
	bool isBPoint = label == LumaPlayerHelper::qcn(L"B");
	int x = progressTimeToX(markerTime100ns);
	QRect track = progressTrackRect();
	int radius = hovered ? 6 : 5;
	QColor color = isBPoint ? QColor(255, 205, 92, 235) : QColor(93, 200, 255, 235);
	painter.save();
	painter.setPen(Qt::NoPen);
	painter.setBrush(color);
	painter.drawEllipse(QPoint(x, track.center().y()), radius, radius);
	QFont markerFont = font();
	markerFont.setPointSize(8);
	painter.setFont(markerFont);
	painter.setPen(color);
	painter.drawText(QRect(x - 10, track.bottom() + 1, 20, 12), Qt::AlignCenter, label);
	if (hovered)
	{
		int64_t displayTime = markerTime100ns;
		if (isBPoint && displayTime > 0)
		{
			displayTime = std::max<int64_t>(0, displayTime - g_config.m_millisecond100ns);
		}
		QRect overlay = bottomOverlayRect();
		QString timeText = LumaPlayerHelper::formatTime(displayTime, true);
		QFontMetrics metrics(painter.font());
		int labelWidth = metrics.width(timeText) + 12;
		int labelHeight = metrics.height() + 4;
		int labelLeft = LumaPlayerHelper::clampInt(x - labelWidth / 2, 4, std::max(4, width() - labelWidth - 4));
		QRect timeRect(labelLeft, track.top() - labelHeight - 5, labelWidth, labelHeight);
		painter.setPen(QColor(255, 255, 255, 230));
		painter.drawText(timeRect, Qt::AlignCenter, timeText);
	}
	painter.restore();

}