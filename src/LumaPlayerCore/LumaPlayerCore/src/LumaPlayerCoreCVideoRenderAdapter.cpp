#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerCoreCVideoRenderAdapter.h"

LumaPlayerCoreCVideoRenderAdapter::LumaPlayerCoreCVideoRenderAdapter() :
m_callbacks()
{

}

void LumaPlayerCoreCVideoRenderAdapter::setCallbacks(const LumaPlayerCoreCRenderCallbacks& callbacks)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_callbacks = callbacks;
}

void LumaPlayerCoreCVideoRenderAdapter::clear()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	memset(&m_callbacks, 0, sizeof(m_callbacks));
}

bool LumaPlayerCoreCVideoRenderAdapter::openVideo(const LumaPlayerVideoFormat& format)
{
	try
	{
		LumaPlayerCoreCRenderCallbacks callbacks = callbackSnapshot();
		if (callbacks.m_openVideo == nullptr)
		{
			return true;
		}
		LumaPlayerCoreCVideoFormat cFormat;
		cFormat.m_width = format.m_width;
		cFormat.m_height = format.m_height;
		return callbacks.m_openVideo(&cFormat, callbacks.m_userData) != 0;
	}
	catch (...)
	{
		return false;
	}
}

void LumaPlayerCoreCVideoRenderAdapter::closeVideo()
{
	try
	{
		LumaPlayerCoreCRenderCallbacks callbacks = callbackSnapshot();
		if (callbacks.m_closeVideo != nullptr)
		{
			callbacks.m_closeVideo(callbacks.m_userData);
		}
	}
	catch (...)
	{

	}
}

bool LumaPlayerCoreCVideoRenderAdapter::renderVideo(const LumaPlayerVideoFrame& frame)
{
	try
	{
		LumaPlayerCoreCRenderCallbacks callbacks = callbackSnapshot();
		if (callbacks.m_renderVideo == nullptr)
		{
			return true;
		}
		LumaPlayerCoreCVideoFrame cFrame;
		cFrame.m_bgraPixels = frame.m_bgraPixels.empty() ? nullptr : &frame.m_bgraPixels[0];
		cFrame.m_bgraSize = frame.m_bgraPixels.size();
		cFrame.m_width = frame.m_width;
		cFrame.m_height = frame.m_height;
		cFrame.m_stride = frame.m_stride;
		cFrame.m_timestamp100ns = frame.m_timestamp100ns;
		cFrame.m_duration100ns = frame.m_duration100ns;
		cFrame.m_frameIndex = frame.m_frameIndex;
		cFrame.m_keyFrame = frame.m_keyFrame ? 1 : 0;
		return callbacks.m_renderVideo(&cFrame, callbacks.m_userData) != 0;
	}
	catch (...)
	{
		return false;
	}
}

LumaPlayerCoreCRenderCallbacks LumaPlayerCoreCVideoRenderAdapter::callbackSnapshot() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_callbacks;
}