#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "Config.h"
#include "LumaPlayerAudioRender.h"
#include "LumaPlayerAudioTask.h"
#include "LogManager/LogManagerAPI.h"

#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QIODevice>
#include <QMutexLocker>

#include <climits>
#include <chrono>

LumaPlayerAudioRender::LumaPlayerAudioRender() :
m_threadId(0),
m_deviceFailed(false),
m_pendingOpen(false),
m_pendingFlush(false),
m_pendingClose(false),
m_shouldPauseAudio(true),
m_pendingPlaybackState(false),
m_queueGeneration(0),
m_deviceGeneration(0),
m_audioOutput(nullptr),
m_audioDevice(nullptr)
{
	m_format.m_sampleRate = 0;
	m_format.m_channels = 0;
	m_format.m_bitsPerSample = 0;
}

LumaPlayerAudioRender::~LumaPlayerAudioRender()
{
	uninit();
}

int32_t LumaPlayerAudioRender::init(int32_t intervalMs)
{
	if (intervalMs <= 0)
	{
		return LumaPlayerCoreCResultInvalidParam;
	}
	if (m_threadId != 0)
	{
		return LumaPlayerCoreCResultSuccess;
	}
	try
	{
		m_task = std::make_shared<LumaPlayerAudioTask>(this, intervalMs);
		m_threadId = CTaskThreadManager::Instance().Init();
		std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(m_threadId);
		if (thread == nullptr)
		{
			uninit();
			return LumaPlayerCoreCResultThreadInitFailed;
		}
		thread->PostTask(m_task, 1);
	}
	catch (...)
	{
		uninit();
		return LumaPlayerCoreCResultThreadInitException;
	}
	return LumaPlayerCoreCResultSuccess;
}

void LumaPlayerAudioRender::uninit()
{
	if (m_task != nullptr)
	{
		m_task->StopTask();
	}
	if (m_threadId != 0)
	{
        const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        LOGINFO("Audio thread stopping managerId=%u", m_threadId);
        CTaskThreadManager::Instance().Uninit(m_threadId);
        LOGINFO("Audio thread stopped managerId=%u elapsedMs=%lld", m_threadId,
            static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()));
        m_threadId = 0;
	}
	m_task.reset();
}

bool LumaPlayerAudioRender::openAudio(const LumaPlayerCoreCAudioFormat& format)
{
	if (format.m_sampleRate <= 0 || format.m_channels <= 0 ||
		format.m_bitsPerSample != 16 || format.m_channels > INT_MAX / 2 ||
		format.m_sampleRate > INT_MAX / (format.m_channels * 2))
	{
		return false;
	}
	QMutexLocker locker(&m_mutex);
	m_deviceFailed = false;
	++m_deviceGeneration;
	m_format = format;
	m_pcmQueue.clear();
	++m_queueGeneration;
	m_pendingOpen = true;
	m_pendingFlush = false;
	m_pendingClose = false;
	m_shouldPauseAudio = true;
	m_pendingPlaybackState = true;
	return true;
}

void LumaPlayerAudioRender::closeAudio()
{
	QMutexLocker locker(&m_mutex);
	++m_deviceGeneration;
	m_pcmQueue.clear();
	++m_queueGeneration;
	m_pendingClose = true;
	m_pendingOpen = false;
	m_pendingFlush = false;
	m_shouldPauseAudio = true;
	m_pendingPlaybackState = false;
}

void LumaPlayerAudioRender::flushAudio()
{
	QMutexLocker locker(&m_mutex);
	m_pcmQueue.clear();
	++m_queueGeneration;
	m_pendingFlush = true;
}

void LumaPlayerAudioRender::pauseAudio()
{
	QMutexLocker locker(&m_mutex);
	m_shouldPauseAudio = true;
	m_pendingPlaybackState = true;
}

void LumaPlayerAudioRender::resumeAudio()
{
	QMutexLocker locker(&m_mutex);
	m_shouldPauseAudio = false;
	m_pendingPlaybackState = true;
}

bool LumaPlayerAudioRender::renderAudio(const LumaPlayerCoreCAudioFrame& frame)
{
	if (frame.m_pcmData == nullptr || frame.m_pcmSize == 0 || frame.m_pcmSize > static_cast<size_t>(INT_MAX))
	{
		return false;
	}
	QMutexLocker locker(&m_mutex);
	if (m_deviceFailed || m_pendingClose || !isFormatValid() ||
		frame.m_format.m_sampleRate != m_format.m_sampleRate || frame.m_format.m_channels != m_format.m_channels ||
		frame.m_format.m_bitsPerSample != m_format.m_bitsPerSample ||
		frame.m_pcmSize % static_cast<size_t>(bytesPerFrame()) != 0 ||
		frame.m_pcmSize > static_cast<size_t>(INT_MAX - m_pcmQueue.size()))
	{
		return false;
	}
	m_pcmQueue.append(reinterpret_cast<const char*>(frame.m_pcmData), static_cast<int>(frame.m_pcmSize));
	trimQueueNoLock();
	return true;
}

int64_t LumaPlayerAudioRender::bufferedDuration100ns() const
{
	QMutexLocker locker(&m_mutex);
	return bytesToDuration100ns(static_cast<size_t>(m_pcmQueue.size()));
}

void LumaPlayerAudioRender::processAudioQueue()
{
	bool closeDevice = false;
	bool openDevice = false;
	bool flushDevice = false;
	bool applyPlaybackState = false;
	bool shouldPauseAudio = true;
	uint64_t deviceGeneration = 0;
	LumaPlayerCoreCAudioFormat format;
	format.m_sampleRate = 0;
	format.m_channels = 0;
	format.m_bitsPerSample = 0;
	{
		QMutexLocker locker(&m_mutex);
		closeDevice = m_pendingClose;
		openDevice = m_pendingOpen;
		flushDevice = m_pendingFlush;
		applyPlaybackState = m_pendingPlaybackState;
		shouldPauseAudio = m_shouldPauseAudio;
		format = m_format;
		deviceGeneration = m_deviceGeneration;
		m_pendingClose = false;
		m_pendingOpen = false;
		m_pendingFlush = false;
		m_pendingPlaybackState = false;
	}

	if (closeDevice || openDevice)
	{
		if (m_audioOutput != nullptr)
		{
			m_audioOutput->stop();
			delete m_audioOutput;
			m_audioOutput = nullptr;
			m_audioDevice = nullptr;
		}
	}
	if (closeDevice && !openDevice)
	{
        LOGINFO("Audio output closed in audio thread");
		return;
	}
	if (openDevice && format.m_sampleRate > 0 && format.m_channels > 0 &&
		format.m_bitsPerSample > 0 && format.m_bitsPerSample % 8 == 0)
	{
		QAudioFormat qtFormat;
		qtFormat.setCodec("audio/pcm");
		qtFormat.setSampleRate(format.m_sampleRate);
		qtFormat.setChannelCount(format.m_channels);
		qtFormat.setSampleSize(format.m_bitsPerSample);
		qtFormat.setByteOrder(QAudioFormat::LittleEndian);
		qtFormat.setSampleType(QAudioFormat::SignedInt);
		QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultOutputDevice();
		if (!deviceInfo.isFormatSupported(qtFormat))
		{
            LOGINFO("Audio device does not support PCM format, sampleRate=%d, channels=%d, bits=%d", qtFormat.sampleRate(), qtFormat.channelCount(), qtFormat.sampleSize());
			QMutexLocker locker(&m_mutex);
			if (deviceGeneration == m_deviceGeneration)
			{
				m_deviceFailed = true;
				m_pcmQueue.clear();
			}
			return;
		}
		m_audioOutput = new(std::nothrow) QAudioOutput(deviceInfo, qtFormat);
		if (m_audioOutput != nullptr)
		{
			int32_t bytesPerSecond = format.m_sampleRate * format.m_channels * (format.m_bitsPerSample / 8);
			int32_t bufferSize = bytesPerSecond / 10;
			if (bufferSize < 4096)
			{
				bufferSize = 4096;
			}
			m_audioOutput->setBufferSize(bufferSize);
			m_audioDevice = m_audioOutput->start();
			if (m_audioDevice == nullptr)
			{
				QMutexLocker locker(&m_mutex);
				m_deviceFailed = deviceGeneration == m_deviceGeneration || m_deviceFailed;
			}
            LOGINFO("Audio output opened, sampleRate=%d, channels=%d, bits=%d, bufferBytes=%d, deviceReady=%d", format.m_sampleRate, format.m_channels, format.m_bitsPerSample, bufferSize, m_audioDevice != nullptr ? 1 : 0);
		}
		else
		{
            LOGINFO("Audio output allocation failed");
			QMutexLocker locker(&m_mutex);
			m_deviceFailed = deviceGeneration == m_deviceGeneration || m_deviceFailed;
		}
	}
	if (flushDevice && m_audioOutput != nullptr)
	{
		m_audioOutput->reset();
		m_audioDevice = m_audioOutput->start();
		if (m_audioDevice == nullptr)
		{
			QMutexLocker locker(&m_mutex);
			m_deviceFailed = deviceGeneration == m_deviceGeneration || m_deviceFailed;
		}
		applyPlaybackState = true;
        LOGINFO("Audio output flushed, deviceReady=%d", m_audioDevice != nullptr ? 1 : 0);
	}
	if (applyPlaybackState && m_audioOutput != nullptr)
	{
		if (shouldPauseAudio)
		{
			m_audioOutput->suspend();
		}
		else
		{
			m_audioOutput->resume();
		}
        LOGINFO("Audio playback state applied, paused=%d, qtState=%d, qtError=%d", shouldPauseAudio ? 1 : 0, static_cast<int>(m_audioOutput->state()), static_cast<int>(m_audioOutput->error()));
	}
	if (m_audioOutput == nullptr || m_audioDevice == nullptr || shouldPauseAudio)
	{
		return;
	}

	for (int32_t writeIndex = 0; writeIndex < 4; ++writeIndex)
	{
		qint64 bytesFree = m_audioOutput->bytesFree();
		if (bytesFree <= 0)
		{
			break;
		}
		QByteArray data;
		uint64_t queueGeneration = 0;
		{
			QMutexLocker locker(&m_mutex);
			if (m_pendingFlush || m_pendingClose || m_pendingOpen || m_shouldPauseAudio || m_pcmQueue.isEmpty())
			{
				break;
			}
			int32_t copyBytes = bytesFree > INT_MAX ? INT_MAX : static_cast<int32_t>(bytesFree);
			if (copyBytes > m_pcmQueue.size())
			{
				copyBytes = m_pcmQueue.size();
			}
			data = m_pcmQueue.left(copyBytes);
			queueGeneration = m_queueGeneration;
		}
		if (data.isEmpty())
		{
			break;
		}
		qint64 writtenBytes = m_audioDevice->write(data.constData(), data.size());
		if (writtenBytes <= 0)
		{
			if (writtenBytes < 0)
			{
				{
					QMutexLocker locker(&m_mutex);
					m_deviceFailed = deviceGeneration == m_deviceGeneration || m_deviceFailed;
				}
                LOGINFO("Audio write failed, qtState=%d, qtError=%d", static_cast<int>(m_audioOutput->state()), static_cast<int>(m_audioOutput->error()));
			}
			break;
		}
		{
			QMutexLocker locker(&m_mutex);
			if (queueGeneration == m_queueGeneration)
			{
				int32_t removeBytes = writtenBytes > m_pcmQueue.size() ? m_pcmQueue.size() : static_cast<int32_t>(writtenBytes);
				m_pcmQueue.remove(0, removeBytes);
			}
		}
	}
}

void LumaPlayerAudioRender::shutdownInAudioThread()
{
	if (m_audioOutput != nullptr)
	{
		m_audioOutput->stop();
		delete m_audioOutput;
		m_audioOutput = nullptr;
		m_audioDevice = nullptr;
	}
	QMutexLocker locker(&m_mutex);
	m_pcmQueue.clear();
	++m_queueGeneration;
	m_pendingOpen = false;
	m_pendingFlush = false;
	m_pendingClose = false;
	m_shouldPauseAudio = true;
	m_pendingPlaybackState = false;
}

int64_t LumaPlayerAudioRender::bytesToDuration100ns(size_t byteCount) const
{
	int32_t bytesPerFrame = this->bytesPerFrame();
	if (bytesPerFrame <= 0 || m_format.m_sampleRate <= 0)
	{
		return 0;
	}
	int64_t sampleCount = static_cast<int64_t>(byteCount / static_cast<size_t>(bytesPerFrame));
	return sampleCount * 10000000 / m_format.m_sampleRate;
}

int32_t LumaPlayerAudioRender::bytesPerFrame() const
{
	if (!isFormatValid())
	{
		return 0;
	}
	return m_format.m_channels * (m_format.m_bitsPerSample / 8);
}

bool LumaPlayerAudioRender::isFormatValid() const
{
	return m_format.m_sampleRate > 0 && m_format.m_channels > 0 &&
		m_format.m_bitsPerSample > 0 && m_format.m_bitsPerSample % 8 == 0;
}

void LumaPlayerAudioRender::trimQueueNoLock()
{
	int64_t maxBytes = 0;
	int32_t bytesPerFrame = this->bytesPerFrame();
	if (bytesPerFrame > 0 && m_format.m_sampleRate > 0)
	{
		maxBytes = (g_config.m_audioMaxQueue100ns * m_format.m_sampleRate / 10000000) * bytesPerFrame;
	}
	if (maxBytes <= 0)
	{
		maxBytes = 4 * 1024 * 1024;
	}
	if (m_pcmQueue.size() > maxBytes)
	{
		const int64_t excess = m_pcmQueue.size() - maxBytes;
		const int64_t alignment = bytesPerFrame > 0 ? bytesPerFrame : 1;
		const int64_t removeBytes = ((excess + alignment - 1) / alignment) * alignment;
		m_pcmQueue.remove(0, static_cast<int>(removeBytes));
		++m_queueGeneration;
	}
}