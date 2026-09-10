#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerVideoRender.h"

#include <QMutexLocker>

#include <climits>
#include <limits>

LumaPlayerVideoRender::LumaPlayerVideoRender() :
m_frameStart100ns(0),
m_frameEnd100ns(0),
m_frameSerial(0)
{

}

bool LumaPlayerVideoRender::openVideo(const LumaPlayerCoreCVideoFormat& format)
{
	QMutexLocker locker(&m_mutex);
	m_videoSize = QSize(format.m_width, format.m_height);
	m_image = QImage();
	m_frameStart100ns = 0;
	m_frameEnd100ns = 0;
	++m_frameSerial;
	return format.m_width > 0 && format.m_height > 0;
}

void LumaPlayerVideoRender::closeVideo()
{
	QMutexLocker locker(&m_mutex);
	m_image = QImage();
	m_videoSize = QSize();
	m_frameStart100ns = 0;
	m_frameEnd100ns = 0;
	++m_frameSerial;
}

bool LumaPlayerVideoRender::renderVideo(const LumaPlayerCoreCVideoFrame& frame)
{
	if (frame.m_bgraPixels == nullptr || frame.m_bgraSize == 0 || frame.m_width <= 0 ||
		frame.m_height <= 0 || frame.m_timestamp100ns < 0 || frame.m_duration100ns < 0 || frame.m_width > INT_MAX / 4 || frame.m_stride < frame.m_width * 4 ||
		static_cast<size_t>(frame.m_stride) > frame.m_bgraSize / static_cast<size_t>(frame.m_height))
	{
		return false;
	}
	QImage frameImage(frame.m_bgraPixels, frame.m_width, frame.m_height, frame.m_stride, QImage::Format_ARGB32);
	if (frameImage.isNull())
	{
		return false;
	}
	QImage ownedImage = frameImage.copy();
	if (ownedImage.isNull())
	{
		return false;
	}
	QMutexLocker locker(&m_mutex);
	m_image = ownedImage;
	m_videoSize = QSize(frame.m_width, frame.m_height);
	m_frameStart100ns = frame.m_timestamp100ns;
	const int64_t maximum = (std::numeric_limits<int64_t>::max)();
	m_frameEnd100ns = frame.m_duration100ns > 0 && frame.m_timestamp100ns > maximum - frame.m_duration100ns ?
		maximum : frame.m_timestamp100ns + frame.m_duration100ns;
	++m_frameSerial;
	return true;
}

bool LumaPlayerVideoRender::copyFrame(QImage* image, int64_t* frameStart100ns, int64_t* frameEnd100ns, uint64_t* frameSerial) const
{
	QMutexLocker locker(&m_mutex);
	if (image != nullptr)
	{
		// 图像发布后不再原地修改，Qt隐式共享保留像素生命周期，避免GUI整帧复制
		*image = m_image;
	}
	if (frameStart100ns != nullptr)
	{
		*frameStart100ns = m_frameStart100ns;
	}
	if (frameEnd100ns != nullptr)
	{
		*frameEnd100ns = m_frameEnd100ns;
	}
	if (frameSerial != nullptr)
	{
		*frameSerial = m_frameSerial;
	}
	return !m_image.isNull();
}

uint64_t LumaPlayerVideoRender::frameSerial() const
{
	QMutexLocker locker(&m_mutex);
	return m_frameSerial;
}

QSize LumaPlayerVideoRender::videoSize() const
{
	QMutexLocker locker(&m_mutex);
	return m_videoSize;
}