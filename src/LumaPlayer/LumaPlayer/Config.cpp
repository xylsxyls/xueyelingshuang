#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "Config.h"
#include "CStringManager/CStringManagerAPI.h"
#include "LumaPlayerHelper.h"
#include <algorithm>
#include <cmath>
#include <QFont>
#include <QFontMetrics>

Config::Config() :
m_windowTitle(QStringLiteral("流光播放器")),
m_loadText(QStringLiteral("加载视频")),
m_videoFileFilter(QStringLiteral("视频文件 (*.mp4 *.avi);;所有文件 (*.*)")),
m_setAText(QStringLiteral("设置A点")),
m_setBText(QStringLiteral("设置B点")),
m_clearABText(QStringLiteral("清除AB点")),
m_aLabel(QStringLiteral("A")),
m_bLabel(QStringLiteral("B")),
m_closeText(QStringLiteral("×")),
m_confirmText(QStringLiteral("确认")),
m_timeMeasureText(QStringLiteral("00:00:00")),
m_zoomTextFormat("%d%%"),
m_timeTextFormat("%02d:%02d:%02d"),
m_preciseTimeTextFormat("%02d:%02d:%02d.%03d"),
m_rgbaFormat("rgba(%d,%d,%d,%d)"),
m_menuStyleFormat("QMenu{background:%s;color:%s;border:%dpx solid %s;} QMenu::item{padding:%dpx %dpx %dpx %dpx;} QMenu::item:selected{background:%s;}"),
m_emptySizeDivisor(8),
m_emptyMinSize(38),
m_emptyMaxSize(76),
m_emptyBorderInset(1),
m_emptyStroke(2.0),
m_emptyArmDivisor(4),
m_centerTipSizeDivisor(7),
m_centerTipMinSize(42),
m_centerTipMaxSize(96),
m_zoomTipWidth(84),
m_zoomTipHeight(32),
m_zoomTipRadius(4.0),
m_zoomTipFontSize(11),
m_uiRefreshMs(16),
m_zoomStep(5),
m_zoomFineStep(1),
m_rateStep(100),
m_fallbackDesktopSize(QSize(1280, 720)),
m_initialDesktopFraction(0.5),
m_largeMediaInitialScale(0.5),
m_markerOverlapTolerance(1),
m_menuPaddingVertical(5),
m_menuPaddingLeft(20),
m_menuPaddingRight(26),
m_menuBorderWidth(1),
m_menuBackgroundColor(QColor(20, 20, 20, 235)),
m_menuBorderColor(QColor(255, 255, 255, 80)),
m_menuSelectionColor(QColor(255, 255, 255, 35)),
m_timeTextColor(QColor(235, 235, 235, 230)),
m_playbackPositionColor(QColor(255, 255, 255, 240)),
m_iconColor(QColor(245, 245, 245, 220)),
m_zoomTextColor(QColor(255, 255, 255, 235)),
m_buttonHoverColor(QColor(255, 255, 255, 38)),
m_buttonColor(QColor(255, 255, 255, 8)),
m_playButtonColor(QColor(255, 255, 255, 18)),
m_centerTipBackgroundColor(QColor(0, 0, 0, 95)),
m_emptyButtonColor(QColor(70, 70, 70, 190)),
m_emptyHoverColor(QColor(95, 95, 95, 210)),
m_emptyIconColor(QColor(210, 210, 210, 220)),
m_videoBackgroundColor(QColor(0, 0, 0)),
m_overlayBorderColor(QColor(255, 255, 255, 55)),
m_overlayColor(QColor(16, 16, 16, 220)),
m_textColor(QColor(255, 255, 255, 230)),
m_playHoverColor(QColor(255, 255, 255, 45)),
m_trackColor(QColor(255, 255, 255, 70)),
m_centerTipIconColor(QColor(255, 255, 255, 175)),
m_tipBackgroundColor(QColor(0, 0, 0, 180)),
m_titleButtonSize(22),
m_titleButtonGap(6),
m_titleMargin(8),
m_titleButtonTop(5),
m_windowButtonGroupGap(10),
m_timeWidth(72),
m_timeMargin(6),
m_timeTrackGap(6),
m_minTrackWidth(64),
m_minVideoHeight(24),
m_trackHeight(4),
m_trackHitExtra(2),
m_playButtonBottom(6),
m_timeFontSize(8),
m_fontSize(10),
m_fontFamily(QStringLiteral("楷体")),
m_markerWidth(2),
m_markerHoverWidth(3),
m_markerHeight(12),
m_markerHitWidth(8),
m_markerHitTop(14),
m_markerHitBottom(18),
m_markerLabelWidth(20),
m_markerLabelHeight(14),
m_markerLabelGap(2),
m_timestampPadding(6),
m_timestampGap(6),
m_positionRadius(5),
m_positionDragRadius(6),
m_cornerRadius(3.0),
m_iconCanvasSize(24.0),
m_iconStroke(1.6),
m_pinAngle(35.0),
m_helpWidth(480),
m_helpHeight(390),
m_helpMargin(20),
m_helpSpacing(14),
m_helpTitleSize(15),
m_helpButtonWidth(80),
m_helpButtonHeight(30),
m_themeColor(QColor(35, 140, 255)),
m_loopAColor(QColor(127, 209, 165, 235)),
m_loopBColor(QColor(255, 205, 92, 235)),
m_disabledColor(QColor(110, 115, 125, 160)),
m_helpBackground(QColor(24, 28, 36)),
m_loadIcon(QPainterPath()),
m_resetIcon(QPainterPath()),
m_pinIcon(QPainterPath()),
m_minimizeIcon(QPainterPath()),
m_maximizeIcon(QPainterPath()),
m_restoreIcon(QPainterPath()),
m_closeIcon(QPainterPath()),
m_playIcon(QPainterPath()),
m_pauseIcon(QPainterPath()),
m_questionIcon(QPainterPath()),
m_timeTextOffset(2),
m_trackExtension(20),
m_buttonPressOpacity(0.25),
m_buttonPressColor(m_themeColor.lighter(140)),
m_closeButtonColor(QColor(205, 48, 48)),
m_closePressDarkening(140),
m_resetText(QStringLiteral("重置")),
m_pinText(QStringLiteral("固定工具栏")),
m_unpinText(QStringLiteral("取消固定工具栏")),
m_helpTitle(QStringLiteral("关于流光播放器")),
m_helpTooltip(QStringLiteral("帮助")),
m_versionLabel(QStringLiteral("版本 1.0")),
m_helpText(QStringLiteral("加载：点击文件夹或中央加号选择视频，也可双击关联的视频文件。\n播放：空格或播放按钮切换播放/暂停，双击画面切换全屏。\n跳转：左右键前进/后退%g秒；进度条支持点击和拖动。\nAB循环：右键进度条设置A/B；悬停端点后按左右键逐帧调整。手动跳出B点后正常播放，回到区间后恢复循环。\n画面：拖动平移，滚轮缩放，Ctrl+滚轮精调；Ctrl+上下键调整倍速。\n重置：恢复居中、缩放和倍速，保留AB、播放进度和播放状态。\n图钉：斜放时工具栏自动显隐；竖直时固定显示并为视频留出空间。")),
m_keyboardSeekSeconds(2.0),
m_logicQueueLimit(512),
m_corePendingLimit(512),
m_topOverlayHeight(32),
m_bottomOverlayHeight(89),
m_playButtonSize(34),
m_progressTrackTop(26),
m_overlayStep(7),
m_mouseIdleHideMs(1000),
m_resizeMargin(7),
m_progressTriggerMargin(15),
m_topTriggerMargin(18),
m_minWindowWidth(160),
m_minWindowHeight(90),
m_minZoomPercent(10),
m_maxZoomPercent(500),
m_previewDragIntervalMs(16),
m_audioServiceIntervalMs(5),
m_second100ns(10000000),
m_millisecond100ns(10000),
m_audioMaxQueue100ns(30000000),
m_debugEnabled(false),
m_startupMediaPath(),
m_logMaxFileBytes(20 * 1024 * 1024),
m_logMaxFileCount(8),
m_logInitialized(false),
m_coreConfig()
{

    m_loadIcon.moveTo(3, 8); m_loadIcon.lineTo(3, 5); m_loadIcon.lineTo(9, 5);
    m_loadIcon.lineTo(11, 8); m_loadIcon.lineTo(21, 8); m_loadIcon.lineTo(19, 19);
    m_loadIcon.lineTo(3, 19); m_loadIcon.closeSubpath();
    m_loadIcon.moveTo(10, 11); m_loadIcon.lineTo(10, 16); m_loadIcon.lineTo(15, 13.5); m_loadIcon.closeSubpath();
    m_resetIcon.moveTo(5, 9); m_resetIcon.cubicTo(8, 2, 20, 5, 20, 13);
    m_resetIcon.cubicTo(20, 21, 8, 24, 4, 16); m_resetIcon.moveTo(5, 4); m_resetIcon.lineTo(5, 10); m_resetIcon.lineTo(11, 10);
    m_pinIcon.moveTo(8, 3); m_pinIcon.lineTo(16, 3); m_pinIcon.moveTo(9, 3);
    m_pinIcon.lineTo(9, 10); m_pinIcon.lineTo(6, 14); m_pinIcon.lineTo(18, 14);
    m_pinIcon.lineTo(15, 10); m_pinIcon.lineTo(15, 3); m_pinIcon.moveTo(12, 14); m_pinIcon.lineTo(12, 22);
    m_minimizeIcon.moveTo(6, 17); m_minimizeIcon.lineTo(18, 17);
    m_maximizeIcon.addRect(QRectF(6, 6, 12, 12));
    m_restoreIcon.addRect(QRectF(8, 5, 10, 10)); m_restoreIcon.addRect(QRectF(5, 9, 10, 10));
    m_closeIcon.moveTo(7, 7); m_closeIcon.lineTo(17, 17); m_closeIcon.moveTo(17, 7); m_closeIcon.lineTo(7, 17);
    m_playIcon.moveTo(9, 6); m_playIcon.lineTo(9, 18); m_playIcon.lineTo(19, 12); m_playIcon.closeSubpath();
    m_pauseIcon.addRect(QRectF(7, 6, 3, 12)); m_pauseIcon.addRect(QRectF(14, 6, 3, 12));
    m_questionIcon.moveTo(7, 7); m_questionIcon.cubicTo(7, 1, 20, 2, 18, 9);
    m_questionIcon.cubicTo(17, 12, 12, 12, 12, 16); m_questionIcon.moveTo(12, 20); m_questionIcon.lineTo(12, 21);
}

Config& Config::instance()
{
	static Config s_config;
	return s_config;
}

void Config::init(const QStringList& arguments)
{
    const QFontMetrics timeMetrics(QFont(m_fontFamily, m_timeFontSize));
    m_timeWidth = (std::max)(m_timeWidth, timeMetrics.width(m_timeMeasureText));
    m_progressTrackTop = (std::max)(m_progressTrackTop, timeMetrics.height() + m_timestampGap);
    m_bottomOverlayHeight = (std::max)(m_bottomOverlayHeight, m_progressTrackTop + m_trackHeight +
        m_markerLabelGap + m_markerLabelHeight + m_playButtonSize + m_playButtonBottom);
    m_minWindowWidth = (std::max)(7 * m_titleButtonSize + 6 * m_titleButtonGap +
        2 * m_titleMargin + m_windowButtonGroupGap,
        2 * (m_timeMargin + m_timeWidth + m_timeTrackGap) + m_minTrackWidth);
    m_minWindowHeight = m_topOverlayHeight + m_bottomOverlayHeight + m_minVideoHeight;
    if (!std::isfinite(m_keyboardSeekSeconds) || m_keyboardSeekSeconds <= 0.0)
    {
        m_keyboardSeekSeconds = 2.0;
    }
	m_debugEnabled = LumaPlayerHelper::hasDebugArgument(arguments);
	m_startupMediaPath.clear();
	for (int32_t index = 1; index < arguments.size(); ++index)
	{
		const QString& argument = arguments.at(index);
		if (!argument.isEmpty() && argument.compare(QStringLiteral("debug"), Qt::CaseInsensitive) != 0)
		{
			m_startupMediaPath = argument;
			break;
		}
	}
	lumaPlayerCoreDefaultConfig(&m_coreConfig);
}

void Config::uninit()
{
	m_debugEnabled = false;
	m_startupMediaPath.clear();
	m_logInitialized.store(false);
}

QString Config::menuStyleSheet() const
{
    const std::string background = CStringManager::Format(m_rgbaFormat.c_str(),
        m_menuBackgroundColor.red(), m_menuBackgroundColor.green(), m_menuBackgroundColor.blue(), m_menuBackgroundColor.alpha());
    const std::string border = CStringManager::Format(m_rgbaFormat.c_str(),
        m_menuBorderColor.red(), m_menuBorderColor.green(), m_menuBorderColor.blue(), m_menuBorderColor.alpha());
    const std::string selection = CStringManager::Format(m_rgbaFormat.c_str(),
        m_menuSelectionColor.red(), m_menuSelectionColor.green(), m_menuSelectionColor.blue(), m_menuSelectionColor.alpha());
    const std::string textColor = m_textColor.name().toStdString();
    return QString::fromStdString(CStringManager::Format(m_menuStyleFormat.c_str(),
        background.c_str(), textColor.c_str(), m_menuBorderWidth, border.c_str(),
        m_menuPaddingVertical, m_menuPaddingRight, m_menuPaddingVertical, m_menuPaddingLeft, selection.c_str()));
}