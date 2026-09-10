#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "Config.h"
#include "LumaPlayerCoreBridge.h"

#include "LumaPlayerAudioRender.h"
#include "LumaPlayerLogger.h"
#include "LumaPlayerVideoRender.h"

LumaPlayerCoreBridge::LumaPlayerCoreBridge(LumaPlayerAudioRender* audioRender, LumaPlayerVideoRender* videoRender) :
m_handle(lumaPlayerCoreCreate()),
m_audioRender(audioRender),
m_videoRender(videoRender),
m_initialized(false)
{

}

LumaPlayerCoreBridge::~LumaPlayerCoreBridge()
{
	uninit();
	if (m_handle != nullptr)
	{
		lumaPlayerCoreSetRenderCallbacks(m_handle, nullptr);

		lumaPlayerCoreDestroy(m_handle);
		m_handle = nullptr;
	}
}

int32_t LumaPlayerCoreBridge::init(bool debugEnabled)
{
	if (m_handle == nullptr || m_audioRender == nullptr || m_videoRender == nullptr)
	{
		return LumaPlayerCoreCResultInternalError;
	}
	if (m_initialized)
	{
		return LumaPlayerCoreCResultSuccess;
	}

	int32_t audioResult = m_audioRender->init(g_config.m_audioServiceIntervalMs);
	if (audioResult != LumaPlayerCoreCResultSuccess)
	{
		return audioResult;
	}

	LumaPlayerCoreCRenderCallbacks callbacks = {};
	callbacks.m_openAudio = &LumaPlayerCoreBridge::openAudioCallback;
	callbacks.m_closeAudio = &LumaPlayerCoreBridge::closeAudioCallback;
	callbacks.m_flushAudio = &LumaPlayerCoreBridge::flushAudioCallback;
	callbacks.m_pauseAudio = &LumaPlayerCoreBridge::pauseAudioCallback;
	callbacks.m_resumeAudio = &LumaPlayerCoreBridge::resumeAudioCallback;
	callbacks.m_renderAudio = &LumaPlayerCoreBridge::renderAudioCallback;
	callbacks.m_bufferedAudio = &LumaPlayerCoreBridge::bufferedAudioCallback;
	callbacks.m_openVideo = &LumaPlayerCoreBridge::openVideoCallback;
	callbacks.m_closeVideo = &LumaPlayerCoreBridge::closeVideoCallback;
	callbacks.m_renderVideo = &LumaPlayerCoreBridge::renderVideoCallback;
	callbacks.m_userData = this;
	lumaPlayerCoreSetRenderCallbacks(m_handle, &callbacks);

	lumaPlayerCoreSetLogEnabled(m_handle, 1);

	int32_t result = lumaPlayerCoreInitWithConfig(m_handle, &g_config.m_coreConfig);
	m_initialized = result == LumaPlayerCoreCResultSuccess;
	if (!m_initialized)
	{
		m_audioRender->uninit();
	}
	return result;
}

void LumaPlayerCoreBridge::uninit()
{
	if (m_handle == nullptr || !m_initialized)
	{
		return;
	}
	lumaPlayerCoreUninit(m_handle);
	m_audioRender->uninit();
	m_initialized = false;
}

LumaPlayerCoreCSnapshot LumaPlayerCoreBridge::snapshot() const
{
	LumaPlayerCoreCSnapshot result = {};
	if (m_handle != nullptr)
	{
		lumaPlayerCoreGetSnapshot(m_handle, &result);
	}
	return result;
}

std::string LumaPlayerCoreBridge::lastError() const
{
	if (m_handle == nullptr)
	{
		return std::string();
	}
	char buffer[2048] = { 0 };
	lumaPlayerCoreGetLastError(m_handle, buffer, sizeof(buffer));
	try
	{
		return std::string(buffer);
	}
	catch (...)
	{
		return std::string();
	}
}

int32_t LumaPlayerCoreBridge::openMediaAsync(const std::string& filePath)
{
	return m_handle != nullptr ? lumaPlayerCoreOpenMediaAsync(m_handle, filePath.c_str()) : LumaPlayerCoreCResultNotInit;
}

int32_t LumaPlayerCoreBridge::playAsync()
{
	return m_handle != nullptr ? lumaPlayerCorePlayAsync(m_handle) : LumaPlayerCoreCResultNotInit;
}

int32_t LumaPlayerCoreBridge::pauseAsync()
{
	return m_handle != nullptr ? lumaPlayerCorePauseAsync(m_handle) : LumaPlayerCoreCResultNotInit;
}

int32_t LumaPlayerCoreBridge::seekToAsync(int64_t position100ns, bool keepPlayState)
{
	return m_handle != nullptr ? lumaPlayerCoreSeekToAsync(m_handle, position100ns, keepPlayState ? 1 : 0) : LumaPlayerCoreCResultNotInit;
}

int32_t LumaPlayerCoreBridge::previewFrameAsync(int64_t position100ns)
{
	return m_handle != nullptr ? lumaPlayerCorePreviewFrameAsync(m_handle, position100ns) : LumaPlayerCoreCResultNotInit;
}

int32_t LumaPlayerCoreBridge::setLoopAAtPositionAsync(int64_t position100ns)
{
	return m_handle != nullptr ? lumaPlayerCoreSetLoopAAtPositionAsync(m_handle, position100ns) : LumaPlayerCoreCResultNotInit;
}

int32_t LumaPlayerCoreBridge::setLoopBAtPositionAsync(int64_t position100ns)
{
	return m_handle != nullptr ? lumaPlayerCoreSetLoopBAtPositionAsync(m_handle, position100ns) : LumaPlayerCoreCResultNotInit;
}

int32_t LumaPlayerCoreBridge::clearLoopAsync()
{
	return m_handle != nullptr ? lumaPlayerCoreClearLoopAsync(m_handle) : LumaPlayerCoreCResultNotInit;
}

int32_t LumaPlayerCoreBridge::moveLoopPointAsync(int32_t pointType, int32_t frameOffset)
{
	return m_handle != nullptr ? lumaPlayerCoreMoveLoopPointAsync(m_handle, pointType, frameOffset) : LumaPlayerCoreCResultNotInit;
}

int32_t LumaPlayerCoreBridge::setPlaybackRatePermilleAsync(int32_t ratePermille)
{
	return m_handle != nullptr ? lumaPlayerCoreSetPlaybackRatePermilleAsync(m_handle, ratePermille) : LumaPlayerCoreCResultNotInit;
}

int32_t LumaPlayerCoreBridge::openAudioCallback(const LumaPlayerCoreCAudioFormat* format, void* userData)
{
	LumaPlayerCoreBridge* bridge = static_cast<LumaPlayerCoreBridge*>(userData);
	return bridge != nullptr && bridge->m_audioRender != nullptr && format != nullptr &&
		bridge->m_audioRender->openAudio(*format) ? 1 : 0;
}

void LumaPlayerCoreBridge::closeAudioCallback(void* userData)
{
	LumaPlayerCoreBridge* bridge = static_cast<LumaPlayerCoreBridge*>(userData);
	if (bridge != nullptr && bridge->m_audioRender != nullptr)
	{
		bridge->m_audioRender->closeAudio();
	}
}

void LumaPlayerCoreBridge::flushAudioCallback(void* userData)
{
	LumaPlayerCoreBridge* bridge = static_cast<LumaPlayerCoreBridge*>(userData);
	if (bridge != nullptr && bridge->m_audioRender != nullptr)
	{
		bridge->m_audioRender->flushAudio();
	}
}

void LumaPlayerCoreBridge::pauseAudioCallback(void* userData)
{
	LumaPlayerCoreBridge* bridge = static_cast<LumaPlayerCoreBridge*>(userData);
	if (bridge != nullptr && bridge->m_audioRender != nullptr)
	{
		bridge->m_audioRender->pauseAudio();
	}
}

void LumaPlayerCoreBridge::resumeAudioCallback(void* userData)
{
	LumaPlayerCoreBridge* bridge = static_cast<LumaPlayerCoreBridge*>(userData);
	if (bridge != nullptr && bridge->m_audioRender != nullptr)
	{
		bridge->m_audioRender->resumeAudio();
	}
}

int32_t LumaPlayerCoreBridge::renderAudioCallback(const LumaPlayerCoreCAudioFrame* frame, void* userData)
{
	LumaPlayerCoreBridge* bridge = static_cast<LumaPlayerCoreBridge*>(userData);
	return bridge != nullptr && bridge->m_audioRender != nullptr && frame != nullptr &&
		bridge->m_audioRender->renderAudio(*frame) ? 1 : 0;
}

int64_t LumaPlayerCoreBridge::bufferedAudioCallback(void* userData)
{
	LumaPlayerCoreBridge* bridge = static_cast<LumaPlayerCoreBridge*>(userData);
	return bridge != nullptr && bridge->m_audioRender != nullptr ? bridge->m_audioRender->bufferedDuration100ns() : 0;
}

int32_t LumaPlayerCoreBridge::openVideoCallback(const LumaPlayerCoreCVideoFormat* format, void* userData)
{
	LumaPlayerCoreBridge* bridge = static_cast<LumaPlayerCoreBridge*>(userData);
	return bridge != nullptr && bridge->m_videoRender != nullptr && format != nullptr &&
		bridge->m_videoRender->openVideo(*format) ? 1 : 0;
}

void LumaPlayerCoreBridge::closeVideoCallback(void* userData)
{
	LumaPlayerCoreBridge* bridge = static_cast<LumaPlayerCoreBridge*>(userData);
	if (bridge != nullptr && bridge->m_videoRender != nullptr)
	{
		bridge->m_videoRender->closeVideo();
	}
}

int32_t LumaPlayerCoreBridge::renderVideoCallback(const LumaPlayerCoreCVideoFrame* frame, void* userData)
{
	LumaPlayerCoreBridge* bridge = static_cast<LumaPlayerCoreBridge*>(userData);
	return bridge != nullptr && bridge->m_videoRender != nullptr && frame != nullptr &&
		bridge->m_videoRender->renderVideo(*frame) ? 1 : 0;
}

void LumaPlayerCoreBridge::cancelLoopPointMove()
{
	lumaPlayerCoreCancelLoopPointMove(m_handle);
}