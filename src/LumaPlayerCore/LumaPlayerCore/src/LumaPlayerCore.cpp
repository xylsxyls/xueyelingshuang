#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerCore.h"
#include "PlayerEngine.h"
#include "LogManager/LogManagerAPI.h"

#include <new>

LumaPlayerCore::LumaPlayerCore() :
m_engine(nullptr),
m_logEnabled(false)
{
	m_engine = new(std::nothrow) PlayerEngine();
}

LumaPlayerCore::~LumaPlayerCore()
{
	uninit();
	delete m_engine;
	m_engine = nullptr;
}

LumaPlayerCoreResult LumaPlayerCore::init()
{
	return init(LumaPlayerCoreConfig());
}

LumaPlayerCoreResult LumaPlayerCore::init(const LumaPlayerCoreConfig& config)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->init(config));
}

void LumaPlayerCore::uninit()
{
	if (m_engine != nullptr)
	{
		m_engine->uninit();
	}
}

bool LumaPlayerCore::isInit() const
{
	return m_engine != nullptr && m_engine->isInit();
}

void LumaPlayerCore::setLogEnabled(bool enabled)
{
    if (enabled && !m_logEnabled.load())
    {
        LogManagerConfig config;
        // 应用和Core共用同一滚动日志，避免同秒创建两个句柄指向同一实体
        config.m_fileId = 0;
		config.m_maxFileBytes = 20 * 1024 * 1024;
		config.m_maxFileCount = 8;
        config.m_outputConsole = false;
        config.m_archiveOldLog = false;
        LogManager::instance().init(config);
    }
    m_logEnabled.store(enabled);
}

void LumaPlayerCore::setAudioRender(IAudioRender* audioRender)
{
	if (m_engine != nullptr)
	{
		m_engine->setAudioRender(audioRender);
	}
}

void LumaPlayerCore::setVideoRender(IVideoRender* videoRender)
{
	if (m_engine != nullptr)
	{
		m_engine->setVideoRender(videoRender);
	}
}

LumaPlayerCoreResult LumaPlayerCore::openMedia(const std::string& filePath)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->openMedia(filePath));
}

LumaPlayerCoreResult LumaPlayerCore::openMediaAsync(const std::string& filePath)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->openMediaAsync(filePath));
}

void LumaPlayerCore::closeMedia()
{
	if (m_engine != nullptr)
	{
		m_engine->closeMedia();
	}
}

LumaPlayerCoreResult LumaPlayerCore::play()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->play());
}

LumaPlayerCoreResult LumaPlayerCore::playAsync()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->playAsync());
}

LumaPlayerCoreResult LumaPlayerCore::pause()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->pause());
}

LumaPlayerCoreResult LumaPlayerCore::pauseAsync()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->pauseAsync());
}

LumaPlayerCoreResult LumaPlayerCore::stop()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->stop());
}

LumaPlayerCoreResult LumaPlayerCore::seekTo(int64_t position100ns, bool keepPlayState)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->seekTo(position100ns, keepPlayState));
}

LumaPlayerCoreResult LumaPlayerCore::seekToAsync(int64_t position100ns, bool keepPlayState)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->seekToAsync(position100ns, keepPlayState));
}

LumaPlayerCoreResult LumaPlayerCore::previewFrame(int64_t position100ns)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->previewFrame(position100ns));
}

LumaPlayerCoreResult LumaPlayerCore::previewFrameAsync(int64_t position100ns)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->previewFrameAsync(position100ns));
}

LumaPlayerCoreResult LumaPlayerCore::setLoopAByCurrentFrame()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->setLoopAByCurrentFrame());
}

LumaPlayerCoreResult LumaPlayerCore::setLoopAAtPosition(int64_t position100ns)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->setLoopAAtPosition(position100ns));
}

LumaPlayerCoreResult LumaPlayerCore::setLoopAAtPositionAsync(int64_t position100ns)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->setLoopAAtPositionAsync(position100ns));
}

LumaPlayerCoreResult LumaPlayerCore::setLoopBByCurrentFrame()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->setLoopBByCurrentFrame());
}

LumaPlayerCoreResult LumaPlayerCore::setLoopBAtPosition(int64_t position100ns)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->setLoopBAtPosition(position100ns));
}

LumaPlayerCoreResult LumaPlayerCore::setLoopBAtPositionAsync(int64_t position100ns)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->setLoopBAtPositionAsync(position100ns));
}

LumaPlayerCoreResult LumaPlayerCore::clearLoop()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->clearLoop());
}

LumaPlayerCoreResult LumaPlayerCore::clearLoopAsync()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->clearLoopAsync());
}

LumaPlayerCoreResult LumaPlayerCore::moveLoopPoint(LumaPlayerCoreLoopPointType pointType, int32_t frameOffset)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->moveLoopPoint(pointType, frameOffset));
}

LumaPlayerCoreResult LumaPlayerCore::moveLoopPointAsync(LumaPlayerCoreLoopPointType pointType, int32_t frameOffset)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->moveLoopPointAsync(pointType, frameOffset));
}

LumaPlayerCoreResult LumaPlayerCore::setPlaybackRatePermille(int32_t ratePermille)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->setPlaybackRatePermille(ratePermille));
}

LumaPlayerCoreResult LumaPlayerCore::setPlaybackRatePermilleAsync(int32_t ratePermille)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
	return reportResult(m_engine->setPlaybackRatePermilleAsync(ratePermille));
}

LumaPlayerSnapshot LumaPlayerCore::snapshot() const
{
	if (m_engine == nullptr)
	{
		return LumaPlayerSnapshot();
	}
	return m_engine->snapshot();
}

LumaPlayerMediaInfo LumaPlayerCore::mediaInfo() const
{
	if (m_engine == nullptr)
	{
		return LumaPlayerMediaInfo();
	}
	return m_engine->mediaInfo();
}

std::string LumaPlayerCore::lastError() const
{
	if (m_engine == nullptr)
	{
		return kLumaPlayerCoreResultDescriptions.at(LumaPlayerCoreResultInternalError);
	}
	return m_engine->lastError();
}

LumaPlayerCoreResult LumaPlayerCore::reportResult(LumaPlayerCoreResult result) const
{
    if (m_logEnabled.load())
    {
        std::map<LumaPlayerCoreResult, std::string>::const_iterator it = kLumaPlayerCoreResultDescriptions.find(result);
        const char* description = it != kLumaPlayerCoreResultDescriptions.end() ? it->second.c_str() : "未知的播放器错误码";
        LogManager::instance().print(0, result == LumaPlayerCoreResultSuccess ? LogManager::LOG_INFO : LogManager::LOG_ERROR,
            __FILE__, __FUNCTION__, "", "", 0, "result=%d，%s", static_cast<int32_t>(result), description);
    }
    return result;
}

void LumaPlayerCore::cancelLoopPointMove()
{
	if (m_engine != nullptr)
	{
		m_engine->cancelLoopPointMove();
	}
}