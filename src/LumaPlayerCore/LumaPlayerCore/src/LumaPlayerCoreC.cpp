#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerCoreC.h"
#include "LumaPlayerCore.h"
#include "LumaPlayerCoreHelper.h"
#include "LumaPlayerCoreCContext.h"
#include "LumaPlayerCoreCVideoRenderAdapter.h"
#include "LumaPlayerCoreCAudioRenderAdapter.h"

#include <mutex>
#include <new>
#include <string>
#include <string.h>

static_assert(LumaPlayerCoreCResultSuccess == LumaPlayerCoreResultSuccess, "C and C++ result values must match");
static_assert(LumaPlayerCoreCResultInvalidParam == LumaPlayerCoreResultInvalidParam, "C and C++ result values must match");
static_assert(LumaPlayerCoreCResultNotInit == LumaPlayerCoreResultNotInit, "C and C++ result values must match");
static_assert(LumaPlayerCoreCResultNotOpen == LumaPlayerCoreResultNotOpen, "C and C++ result values must match");
static_assert(LumaPlayerCoreCResultOpenFailed == LumaPlayerCoreResultOpenFailed, "C and C++ result values must match");
static_assert(LumaPlayerCoreCResultStateError == LumaPlayerCoreResultStateError, "C and C++ result values must match");
static_assert(LumaPlayerCoreCResultCanceled == LumaPlayerCoreResultCanceled, "C and C++ result values must match");
static_assert(LumaPlayerCoreCResultTimeout == LumaPlayerCoreResultTimeout, "C and C++ result values must match");
static_assert(LumaPlayerCoreCResultInternalError == LumaPlayerCoreResultInternalError, "C and C++ result values must match");
static_assert(LumaPlayerCoreCResultEnd == LumaPlayerCoreResultEnd, "C and C++ result values must match");
static_assert(LumaPlayerCoreCLoopPointA == LumaPlayerCoreLoopPointA, "C and C++ loop point values must match");
static_assert(LumaPlayerCoreCLoopPointB == LumaPlayerCoreLoopPointB, "C and C++ loop point values must match");

LumaPlayerCoreHandle lumaPlayerCoreCreate()
{
	LumaPlayerCoreCContext* instance = nullptr;
	try
	{
		instance = new(std::nothrow) LumaPlayerCoreCContext();
		if (instance == nullptr)
		{
			return nullptr;
		}
		instance->m_core = nullptr;
		instance->m_audioRender = nullptr;
		instance->m_videoRender = nullptr;
		instance->m_core = new(std::nothrow) LumaPlayerCore();
		instance->m_audioRender = new(std::nothrow) LumaPlayerCoreCAudioRenderAdapter();
		instance->m_videoRender = new(std::nothrow) LumaPlayerCoreCVideoRenderAdapter();
		if (instance->m_core == nullptr || instance->m_audioRender == nullptr || instance->m_videoRender == nullptr)
		{
			lumaPlayerCoreDestroy(instance);
			return nullptr;
		}
		return instance;
	}
	catch (...)
	{
		lumaPlayerCoreDestroy(instance);
		return nullptr;
	}
}

void lumaPlayerCoreDestroy(LumaPlayerCoreHandle handle)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	if (instance == nullptr)
	{
		return;
	}
	LumaPlayerCoreHelper::invokeVoid(instance, "C API exception in lumaPlayerCoreDestroy/uninit", [](LumaPlayerCore& core)
	{
		core.uninit();
		core.setAudioRender(nullptr);
		core.setVideoRender(nullptr);

	});
	delete instance->m_core;
	delete instance->m_audioRender;
	delete instance->m_videoRender;
	delete instance;
}

int32_t lumaPlayerCoreInit(LumaPlayerCoreHandle handle)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreInit", [](LumaPlayerCore& core)
	{
		return core.init();
	});
}

void lumaPlayerCoreUninit(LumaPlayerCoreHandle handle)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	LumaPlayerCoreHelper::invokeVoid(instance, "C API exception in lumaPlayerCoreUninit", [](LumaPlayerCore& core)
	{
		core.uninit();
	});
}

void lumaPlayerCoreSetLogEnabled(LumaPlayerCoreHandle handle, int32_t enabled)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	LumaPlayerCoreHelper::invokeVoid(instance, "C API exception in lumaPlayerCoreSetLogEnabled", [enabled](LumaPlayerCore& core)
	{
		core.setLogEnabled(enabled != 0);
	});
}

void lumaPlayerCoreSetRenderCallbacks(LumaPlayerCoreHandle handle, const LumaPlayerCoreCRenderCallbacks* callbacks)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	if (instance == nullptr || instance->m_core == nullptr || instance->m_audioRender == nullptr || instance->m_videoRender == nullptr)
	{
		return;
	}
	try
	{
		if (callbacks == nullptr)
		{
			instance->m_core->setAudioRender(nullptr);
			instance->m_core->setVideoRender(nullptr);
			instance->m_audioRender->clear();
			instance->m_videoRender->clear();
			return;
		}
		instance->m_audioRender->setCallbacks(*callbacks);
		instance->m_videoRender->setCallbacks(*callbacks);
		instance->m_core->setAudioRender(instance->m_audioRender);
		instance->m_core->setVideoRender(instance->m_videoRender);
	}
	catch (...)
	{

	}
}

int32_t lumaPlayerCoreOpenMedia(LumaPlayerCoreHandle handle, const char* filePath)
{
	if (filePath == nullptr)
	{
		return LumaPlayerCoreCResultInvalidParam;
	}
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreOpenMedia", [filePath](LumaPlayerCore& core)
	{
		return core.openMedia(filePath);
	});
}

int32_t lumaPlayerCoreOpenMediaAsync(LumaPlayerCoreHandle handle, const char* filePath)
{
	if (filePath == nullptr)
	{
		return LumaPlayerCoreCResultInvalidParam;
	}
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreOpenMediaAsync", [filePath](LumaPlayerCore& core)
	{
		return core.openMediaAsync(filePath);
	});
}

void lumaPlayerCoreCloseMedia(LumaPlayerCoreHandle handle)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	LumaPlayerCoreHelper::invokeVoid(instance, "C API exception in lumaPlayerCoreCloseMedia", [](LumaPlayerCore& core)
	{
		core.closeMedia();
	});
}

int32_t lumaPlayerCorePlay(LumaPlayerCoreHandle handle)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCorePlay", [](LumaPlayerCore& core)
	{
		return core.play();
	});
}

int32_t lumaPlayerCorePlayAsync(LumaPlayerCoreHandle handle)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCorePlayAsync", [](LumaPlayerCore& core)
	{
		return core.playAsync();
	});
}

int32_t lumaPlayerCorePause(LumaPlayerCoreHandle handle)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCorePause", [](LumaPlayerCore& core)
	{
		return core.pause();
	});
}

int32_t lumaPlayerCorePauseAsync(LumaPlayerCoreHandle handle)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCorePauseAsync", [](LumaPlayerCore& core)
	{
		return core.pauseAsync();
	});
}

int32_t lumaPlayerCoreStop(LumaPlayerCoreHandle handle)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreStop", [](LumaPlayerCore& core)
	{
		return core.stop();
	});
}

int32_t lumaPlayerCoreSeekTo(LumaPlayerCoreHandle handle, int64_t position100ns, int32_t keepPlayState)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreSeekTo", [position100ns, keepPlayState](LumaPlayerCore& core)
	{
		return core.seekTo(position100ns, keepPlayState != 0);
	});
}

int32_t lumaPlayerCoreSeekToAsync(LumaPlayerCoreHandle handle, int64_t position100ns, int32_t keepPlayState)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreSeekToAsync", [position100ns, keepPlayState](LumaPlayerCore& core)
	{
		return core.seekToAsync(position100ns, keepPlayState != 0);
	});
}

int32_t lumaPlayerCorePreviewFrame(LumaPlayerCoreHandle handle, int64_t position100ns)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCorePreviewFrame", [position100ns](LumaPlayerCore& core)
	{
		return core.previewFrame(position100ns);
	});
}

int32_t lumaPlayerCorePreviewFrameAsync(LumaPlayerCoreHandle handle, int64_t position100ns)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCorePreviewFrameAsync", [position100ns](LumaPlayerCore& core)
	{
		return core.previewFrameAsync(position100ns);
	});
}

int32_t lumaPlayerCoreSetLoopAByCurrentFrame(LumaPlayerCoreHandle handle)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreSetLoopAByCurrentFrame", [](LumaPlayerCore& core)
	{
		return core.setLoopAByCurrentFrame();
	});
}

int32_t lumaPlayerCoreSetLoopAAtPosition(LumaPlayerCoreHandle handle, int64_t position100ns)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreSetLoopAAtPosition", [position100ns](LumaPlayerCore& core)
	{
		return core.setLoopAAtPosition(position100ns);
	});
}

int32_t lumaPlayerCoreSetLoopAAtPositionAsync(LumaPlayerCoreHandle handle, int64_t position100ns)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreSetLoopAAtPositionAsync", [position100ns](LumaPlayerCore& core)
	{
		return core.setLoopAAtPositionAsync(position100ns);
	});
}

int32_t lumaPlayerCoreSetLoopBByCurrentFrame(LumaPlayerCoreHandle handle)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreSetLoopBByCurrentFrame", [](LumaPlayerCore& core)
	{
		return core.setLoopBByCurrentFrame();
	});
}

int32_t lumaPlayerCoreSetLoopBAtPosition(LumaPlayerCoreHandle handle, int64_t position100ns)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreSetLoopBAtPosition", [position100ns](LumaPlayerCore& core)
	{
		return core.setLoopBAtPosition(position100ns);
	});
}

int32_t lumaPlayerCoreSetLoopBAtPositionAsync(LumaPlayerCoreHandle handle, int64_t position100ns)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreSetLoopBAtPositionAsync", [position100ns](LumaPlayerCore& core)
	{
		return core.setLoopBAtPositionAsync(position100ns);
	});
}

int32_t lumaPlayerCoreClearLoop(LumaPlayerCoreHandle handle)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreClearLoop", [](LumaPlayerCore& core)
	{
		return core.clearLoop();
	});
}

int32_t lumaPlayerCoreClearLoopAsync(LumaPlayerCoreHandle handle)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreClearLoopAsync", [](LumaPlayerCore& core)
	{
		return core.clearLoopAsync();
	});
}

int32_t lumaPlayerCoreMoveLoopPoint(LumaPlayerCoreHandle handle, int32_t pointType, int32_t frameOffset)
{
	if (pointType != LumaPlayerCoreCLoopPointA && pointType != LumaPlayerCoreCLoopPointB)
	{
		return LumaPlayerCoreCResultInvalidParam;
	}
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreMoveLoopPoint", [pointType, frameOffset](LumaPlayerCore& core)
	{
		return core.moveLoopPoint(static_cast<LumaPlayerCoreLoopPointType>(pointType), frameOffset);
	});
}

int32_t lumaPlayerCoreMoveLoopPointAsync(LumaPlayerCoreHandle handle, int32_t pointType, int32_t frameOffset)
{
	if (pointType != LumaPlayerCoreCLoopPointA && pointType != LumaPlayerCoreCLoopPointB)
	{
		return LumaPlayerCoreCResultInvalidParam;
	}
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreMoveLoopPointAsync", [pointType, frameOffset](LumaPlayerCore& core)
	{
		return core.moveLoopPointAsync(static_cast<LumaPlayerCoreLoopPointType>(pointType), frameOffset);
	});
}

int32_t lumaPlayerCoreSetPlaybackRatePermille(LumaPlayerCoreHandle handle, int32_t ratePermille)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreSetPlaybackRatePermille", [ratePermille](LumaPlayerCore& core)
	{
		return core.setPlaybackRatePermille(ratePermille);
	});
}

int32_t lumaPlayerCoreSetPlaybackRatePermilleAsync(LumaPlayerCoreHandle handle, int32_t ratePermille)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	return LumaPlayerCoreHelper::invokeResult(instance, "C API exception in lumaPlayerCoreSetPlaybackRatePermilleAsync", [ratePermille](LumaPlayerCore& core)
	{
		return core.setPlaybackRatePermilleAsync(ratePermille);
	});
}

int32_t lumaPlayerCoreGetSnapshot(LumaPlayerCoreHandle handle, LumaPlayerCoreCSnapshot* snapshot)
{
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	if (snapshot == nullptr)
	{
		return 0;
	}
	memset(snapshot, 0, sizeof(*snapshot));
	if (instance == nullptr || instance->m_core == nullptr)
	{
		return 0;
	}
	try
	{
		LumaPlayerSnapshot cppSnapshot = instance->m_core->snapshot();
		snapshot->m_result = static_cast<int32_t>(cppSnapshot.m_result);
		snapshot->m_state = static_cast<int32_t>(cppSnapshot.m_state);
		snapshot->m_mediaSerial = cppSnapshot.m_mediaSerial;
		snapshot->m_duration100ns = cppSnapshot.m_mediaInfo.m_duration100ns;
		snapshot->m_position100ns = cppSnapshot.m_position100ns;
		snapshot->m_defaultVideoFrameDuration100ns = cppSnapshot.m_mediaInfo.m_defaultVideoFrameDuration100ns;
		snapshot->m_ratePermille = cppSnapshot.m_ratePermille;
		snapshot->m_hasVideo = cppSnapshot.m_mediaInfo.m_hasVideo ? 1 : 0;
		snapshot->m_hasAudio = cppSnapshot.m_mediaInfo.m_hasAudio ? 1 : 0;
		snapshot->m_videoWidth = cppSnapshot.m_mediaInfo.m_videoFormat.m_width;
		snapshot->m_videoHeight = cppSnapshot.m_mediaInfo.m_videoFormat.m_height;
		snapshot->m_hasLoopA = cppSnapshot.m_loopRange.m_aPoint.m_isSet ? 1 : 0;
		snapshot->m_hasLoopB = cppSnapshot.m_loopRange.m_bPoint.m_isSet ? 1 : 0;
		snapshot->m_loopAStart100ns = cppSnapshot.m_loopRange.m_aPoint.m_frameStart100ns;
		snapshot->m_loopBEnd100ns = cppSnapshot.m_loopRange.m_bPoint.m_frameEnd100ns;
		return 1;
	}
	catch (...)
	{
		memset(snapshot, 0, sizeof(*snapshot));

		return 0;
	}
}

size_t lumaPlayerCoreGetLastError(LumaPlayerCoreHandle handle, char* buffer, size_t bufferSize)
{
	if (buffer != nullptr && bufferSize > 0)
	{
		buffer[0] = '\0';
	}
	LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
	if (instance == nullptr || instance->m_core == nullptr)
	{
		return 0;
	}
	try
	{
		std::string errorText = instance->m_core->lastError();
		size_t requiredSize = errorText.size() + 1;
		if (buffer != nullptr && bufferSize > 0)
		{
			size_t copySize = errorText.size();
			if (copySize >= bufferSize)
			{
				copySize = bufferSize - 1;
			}
			if (copySize > 0)
			{
				memcpy(buffer, errorText.data(), copySize);
			}
			buffer[copySize] = '\0';
		}
		return requiredSize;
	}
	catch (...)
	{

		return 0;
	}
}

int32_t lumaPlayerCoreDefaultConfig(LumaPlayerCoreCConfig* config)
{
	if (config == nullptr)
	{
		return LumaPlayerCoreCResultInvalidParam;
	}
	LumaPlayerCoreConfig defaults;
	config->m_controlCommandTimeoutMs = defaults.m_controlCommandTimeoutMs;
	config->m_openCommandTimeoutMs = defaults.m_openCommandTimeoutMs;
	config->m_idleWaitMs = defaults.m_idleWaitMs;
	config->m_videoEarlyTolerance100ns = defaults.m_videoEarlyTolerance100ns;
	config->m_audioLead100ns = defaults.m_audioLead100ns;
	config->m_loopBufferMaxBytes = defaults.m_loopBufferMaxBytes;
	config->m_defaultFrameDuration100ns = defaults.m_defaultFrameDuration100ns;
	config->m_defaultRatePermille = defaults.m_defaultRatePermille;
	config->m_minRatePermille = defaults.m_minRatePermille;
	config->m_maxRatePermille = defaults.m_maxRatePermille;
	return LumaPlayerCoreCResultSuccess;
}

int32_t lumaPlayerCoreInitWithConfig(LumaPlayerCoreHandle handle, const LumaPlayerCoreCConfig* config)
{
	if (config == nullptr)
	{
		return LumaPlayerCoreCResultInvalidParam;
	}
	LumaPlayerCoreConfig options;
	options.m_controlCommandTimeoutMs = config->m_controlCommandTimeoutMs;
	options.m_openCommandTimeoutMs = config->m_openCommandTimeoutMs;
	options.m_idleWaitMs = config->m_idleWaitMs;
	options.m_videoEarlyTolerance100ns = config->m_videoEarlyTolerance100ns;
	options.m_audioLead100ns = config->m_audioLead100ns;
	options.m_loopBufferMaxBytes = config->m_loopBufferMaxBytes;
	options.m_defaultFrameDuration100ns = config->m_defaultFrameDuration100ns;
	options.m_defaultRatePermille = config->m_defaultRatePermille;
	options.m_minRatePermille = config->m_minRatePermille;
	options.m_maxRatePermille = config->m_maxRatePermille;
	return LumaPlayerCoreHelper::invokeResult(LumaPlayerCoreHelper::castHandle(handle), "init", [&options](LumaPlayerCore& core)
	{
		return core.init(options);
	});
}

const char* lumaPlayerCoreResultDescription(int32_t result)
{
	std::map<LumaPlayerCoreResult, std::string>::const_iterator it = kLumaPlayerCoreResultDescriptions.find(static_cast<LumaPlayerCoreResult>(result));
	return it != kLumaPlayerCoreResultDescriptions.end() ? it->second.c_str() : "未知的播放器错误码";
}

int32_t lumaPlayerCoreCancelLoopPointMove(LumaPlayerCoreHandle handle)
{
    LumaPlayerCoreCContext* instance = LumaPlayerCoreHelper::castHandle(handle);
    return LumaPlayerCoreHelper::invokeResult(instance, "cancel loop move", [](LumaPlayerCore& core)
    {
        core.cancelLoopPointMove();
        return LumaPlayerCoreResultSuccess;
    });
}