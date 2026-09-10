#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerCoreCAudioRenderAdapter.h"

LumaPlayerCoreCAudioRenderAdapter::LumaPlayerCoreCAudioRenderAdapter() :
m_callbacks()
{

}

void LumaPlayerCoreCAudioRenderAdapter::setCallbacks(const LumaPlayerCoreCRenderCallbacks& callbacks)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_callbacks = callbacks;
}

void LumaPlayerCoreCAudioRenderAdapter::clear()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	memset(&m_callbacks, 0, sizeof(m_callbacks));
}

bool LumaPlayerCoreCAudioRenderAdapter::openAudio(const LumaPlayerAudioFormat& format)
{
	try
	{
		LumaPlayerCoreCRenderCallbacks callbacks = callbackSnapshot();
		if (callbacks.m_openAudio == nullptr)
		{
			return true;
		}
		LumaPlayerCoreCAudioFormat cFormat;
		cFormat.m_sampleRate = format.m_sampleRate;
		cFormat.m_channels = format.m_channels;
		cFormat.m_bitsPerSample = format.m_bitsPerSample;
		return callbacks.m_openAudio(&cFormat, callbacks.m_userData) != 0;
	}
	catch (...)
	{
		return false;
	}
}

void LumaPlayerCoreCAudioRenderAdapter::closeAudio()
{
	try
	{
		LumaPlayerCoreCRenderCallbacks callbacks = callbackSnapshot();
		if (callbacks.m_closeAudio != nullptr)
		{
			callbacks.m_closeAudio(callbacks.m_userData);
		}
	}
	catch (...)
	{

	}
}

void LumaPlayerCoreCAudioRenderAdapter::flushAudio()
{
	try
	{
		LumaPlayerCoreCRenderCallbacks callbacks = callbackSnapshot();
		if (callbacks.m_flushAudio != nullptr)
		{
			callbacks.m_flushAudio(callbacks.m_userData);
		}
	}
	catch (...)
	{

	}
}

void LumaPlayerCoreCAudioRenderAdapter::pauseAudio()
{
	try
	{
		LumaPlayerCoreCRenderCallbacks callbacks = callbackSnapshot();
		if (callbacks.m_pauseAudio != nullptr)
		{
			callbacks.m_pauseAudio(callbacks.m_userData);
		}
	}
	catch (...)
	{

	}
}

void LumaPlayerCoreCAudioRenderAdapter::resumeAudio()
{
	try
	{
		LumaPlayerCoreCRenderCallbacks callbacks = callbackSnapshot();
		if (callbacks.m_resumeAudio != nullptr)
		{
			callbacks.m_resumeAudio(callbacks.m_userData);
		}
	}
	catch (...)
	{

	}
}

bool LumaPlayerCoreCAudioRenderAdapter::renderAudio(const LumaPlayerAudioFrame& frame)
{
	try
	{
		LumaPlayerCoreCRenderCallbacks callbacks = callbackSnapshot();
		if (callbacks.m_renderAudio == nullptr)
		{
			return true;
		}
		LumaPlayerCoreCAudioFrame cFrame;
		cFrame.m_pcmData = frame.m_pcmData.empty() ? nullptr : &frame.m_pcmData[0];
		cFrame.m_pcmSize = frame.m_pcmData.size();
		cFrame.m_format.m_sampleRate = frame.m_format.m_sampleRate;
		cFrame.m_format.m_channels = frame.m_format.m_channels;
		cFrame.m_format.m_bitsPerSample = frame.m_format.m_bitsPerSample;
		cFrame.m_timestamp100ns = frame.m_timestamp100ns;
		cFrame.m_duration100ns = frame.m_duration100ns;
		return callbacks.m_renderAudio(&cFrame, callbacks.m_userData) != 0;
	}
	catch (...)
	{
		return false;
	}
}

int64_t LumaPlayerCoreCAudioRenderAdapter::bufferedDuration100ns() const
{
	try
	{
		LumaPlayerCoreCRenderCallbacks callbacks = callbackSnapshot();
		if (callbacks.m_bufferedAudio == nullptr)
		{
			return 0;
		}
		return callbacks.m_bufferedAudio(callbacks.m_userData);
	}
	catch (...)
	{
		return 0;
	}
}

LumaPlayerCoreCRenderCallbacks LumaPlayerCoreCAudioRenderAdapter::callbackSnapshot() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_callbacks;
}