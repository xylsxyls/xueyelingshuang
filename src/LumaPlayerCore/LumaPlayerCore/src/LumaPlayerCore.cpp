#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerCore.h"
#include "PlayerEngine.h"
#include "LogManager/LogManagerAPI.h"

#include <new>
#include <chrono>

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
    const LumaPlayerCoreResult result = m_engine->init(config);
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

void LumaPlayerCore::uninit()
{
	if (m_engine != nullptr)
	{
        const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        if (m_logEnabled.load())
        {
            LOGINFO("Core shutdown begin");
        }
        m_engine->uninit();
        if (m_logEnabled.load())
        {
            LOGINFO("Core shutdown complete elapsedMs=%lld",
                static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()));
        }
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
        config.m_archiveOldLog = true;
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
    const LumaPlayerCoreResult result = m_engine->openMedia(filePath);
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::openMediaAsync(const std::string& filePath)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->openMediaAsync(filePath);
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
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
    const LumaPlayerCoreResult result = m_engine->play();
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::playAsync()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->playAsync();
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::pause()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->pause();
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::pauseAsync()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->pauseAsync();
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::stop()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->stop();
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::seekTo(int64_t position100ns, bool keepPlayState)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->seekTo(position100ns, keepPlayState);
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::seekToAsync(int64_t position100ns, bool keepPlayState)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->seekToAsync(position100ns, keepPlayState);
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::previewFrame(int64_t position100ns)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->previewFrame(position100ns);
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::previewFrameAsync(int64_t position100ns)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->previewFrameAsync(position100ns);
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::setLoopAByCurrentFrame()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->setLoopAByCurrentFrame();
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::setLoopAAtPosition(int64_t position100ns)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->setLoopAAtPosition(position100ns);
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::setLoopAAtPositionAsync(int64_t position100ns)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->setLoopAAtPositionAsync(position100ns);
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::setLoopBByCurrentFrame()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->setLoopBByCurrentFrame();
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::setLoopBAtPosition(int64_t position100ns)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->setLoopBAtPosition(position100ns);
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::setLoopBAtPositionAsync(int64_t position100ns)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->setLoopBAtPositionAsync(position100ns);
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::clearLoop()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->clearLoop();
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::clearLoopAsync()
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->clearLoopAsync();
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::moveLoopPoint(LumaPlayerCoreLoopPointType pointType, int32_t frameOffset)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->moveLoopPoint(pointType, frameOffset);
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::moveLoopPointAsync(LumaPlayerCoreLoopPointType pointType, int32_t frameOffset)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->moveLoopPointAsync(pointType, frameOffset);
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::setPlaybackRatePermille(int32_t ratePermille)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->setPlaybackRatePermille(ratePermille);
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
}

LumaPlayerCoreResult LumaPlayerCore::setPlaybackRatePermilleAsync(int32_t ratePermille)
{
	if (m_engine == nullptr)
	{
		return LumaPlayerCoreResultInternalError;
	}
    const LumaPlayerCoreResult result = m_engine->setPlaybackRatePermilleAsync(ratePermille);
    if (m_logEnabled.load())
    {
        if (result == LumaPlayerCoreResultSuccess)
        {
            LOGINFO("Core operation result=%d", static_cast<int32_t>(result));
        }
        else
        {
            LOGERROR("Core operation result=%d, %s", static_cast<int32_t>(result),
                kLumaPlayerCoreResultDescriptions.at(result).c_str());
        }
    }
    return result;
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

void LumaPlayerCore::cancelLoopPointMove()
{
	if (m_engine != nullptr)
	{
		m_engine->cancelLoopPointMove();
	}
}

LumaPlayerCoreResult LumaPlayerCore::submitAsyncEx(const LumaPlayerCoreRequest& request,
    const LumaPlayerCoreCompletionCallback& callback)
{
    if (m_engine == nullptr)
    {
        return LumaPlayerCoreResultNotInit;
    }
    const LumaPlayerCoreResult result = m_engine->submitAsyncEx(request, callback);
    if (m_logEnabled.load() && (request.m_operation != LumaPlayerCoreOperationPreview ||
        result != LumaPlayerCoreResultSuccess))
    {
        LOGINFO("Core request submitted id=%llu generation=%llu operation=%d result=%d",
            static_cast<unsigned long long>(request.m_requestId),
            static_cast<unsigned long long>(request.m_mediaGeneration),
            static_cast<int32_t>(request.m_operation), static_cast<int32_t>(result));
    }
    return result;
}