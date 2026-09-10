#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "Config.h"
#include "LumaPlayerHelper.h"

Config::Config() :
m_topOverlayHeight(28),
m_bottomOverlayHeight(88),
m_playButtonSize(34),
m_progressTrackTop(32),
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
m_logInitialized(false),
m_coreConfig()
{

}

Config& Config::instance()
{
	static Config s_config;
	return s_config;
}

void Config::init(const QStringList& arguments)
{
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