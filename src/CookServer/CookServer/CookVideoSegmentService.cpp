#include "CookVideoSegmentService.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "CookHelper.h"
#include "CSystem/CSystemAPI.h"
extern "C"
{
#include <ffmpeg/libavcodec/avcodec.h>
#include <ffmpeg/libavformat/avformat.h>
#include <ffmpeg/libavutil/error.h>
#include <ffmpeg/libavutil/mathematics.h>
}
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>
#include <windows.h>
#endif

#ifdef _WIN32
namespace
{
struct SegmentPacket
{
	AVPacket* packet;
	int streamIndex;
	int64_t timestampUs;

	SegmentPacket() : packet(nullptr), streamIndex(-1), timestampUs(0) {}
};

std::mutex& segmentLockFor(const std::string& path)
{
	static std::mutex locks[32];
	return locks[std::hash<std::string>()(path) % 32];
}

std::string ffmpegErrorText(int error)
{
	char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
	if (av_strerror(error, buffer, sizeof(buffer)) < 0)
	{
		return "FFmpeg error " + std::to_string(error);
	}
	return buffer;
}

std::string safePathPart(const std::string& value)
{
	static const char hex[] = "0123456789abcdef";
	std::string result;
	for (size_t i = 0; i < value.size(); ++i)
	{
		unsigned char ch = static_cast<unsigned char>(value[i]);
		if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
		    (ch >= '0' && ch <= '9') || ch == '-' || ch == '_')
		{
			result.push_back(static_cast<char>(ch));
		}
		else
		{
			result.push_back('_');
			result.push_back(hex[(ch >> 4) & 0x0f]);
			result.push_back(hex[ch & 0x0f]);
		}
	}
	return result.empty() ? "video" : result;
}

int64_t packetTimestampUs(const AVPacket* packet, const AVStream* stream)
{
	int64_t timestamp = packet->dts != AV_NOPTS_VALUE ? packet->dts : packet->pts;
	return timestamp == AV_NOPTS_VALUE ? AV_NOPTS_VALUE : av_rescale_q(timestamp, stream->time_base, AV_TIME_BASE_Q);
}

void freeBufferedPackets(std::vector<SegmentPacket>& packets)
{
	for (size_t i = 0; i < packets.size(); ++i)
	{
		av_packet_free(&packets[i].packet);
	}
	packets.clear();
}

bool writePacket(AVFormatContext* outputContext,
	             AVFormatContext* inputContext,
	             const std::vector<int>& streamMapping,
	             AVPacket* packet,
	             int64_t segmentStartUs,
	             std::string& errorMessage)
{
	const int inputIndex = packet->stream_index;
	if (inputIndex < 0 || inputIndex >= static_cast<int>(streamMapping.size()) || streamMapping[inputIndex] < 0)
	{
		return true;
	}
	AVStream* inputStream = inputContext->streams[inputIndex];
	AVStream* outputStream = outputContext->streams[streamMapping[inputIndex]];
	int64_t offset = av_rescale_q(segmentStartUs, AV_TIME_BASE_Q, outputStream->time_base);
	if (packet->pts != AV_NOPTS_VALUE)
	{
		packet->pts = av_rescale_q_rnd(packet->pts, inputStream->time_base, outputStream->time_base,
			static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)) - offset;
	}
	if (packet->dts != AV_NOPTS_VALUE)
	{
		packet->dts = av_rescale_q_rnd(packet->dts, inputStream->time_base, outputStream->time_base,
			static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)) - offset;
	}
	packet->duration = av_rescale_q(packet->duration, inputStream->time_base, outputStream->time_base);
	packet->pos = -1;
	packet->stream_index = streamMapping[inputIndex];
	int result = av_interleaved_write_frame(outputContext, packet);
	if (result < 0)
	{
		errorMessage = "write MP4 segment packet failed: " + ffmpegErrorText(result);
		return false;
	}
	return true;
}

bool extractSegment(const std::string& sourcePath,
	                const std::string& temporaryPath,
	                int32_t startMilliseconds,
	                int32_t durationMilliseconds,
	                int32_t& actualStartMilliseconds,
	                int32_t& mediaDurationMilliseconds,
	                std::string& errorMessage)
{
	AVFormatContext* inputContext = nullptr;
	AVFormatContext* outputContext = nullptr;
	AVPacket* packet = nullptr;
	AVDictionary* muxOptions = nullptr;
	std::vector<SegmentPacket> initialPackets;
	std::vector<int> streamMapping;
	bool outputOpened = false;
	bool headerWritten = false;
	bool resultOk = false;
	int videoStreamIndex = -1;
	int result = avformat_open_input(&inputContext, sourcePath.c_str(), nullptr, nullptr);
	if (result < 0)
	{
		errorMessage = "open source video failed: " + ffmpegErrorText(result);
		goto cleanup;
	}
	result = avformat_find_stream_info(inputContext, nullptr);
	if (result < 0)
	{
		errorMessage = "read source video metadata failed: " + ffmpegErrorText(result);
		goto cleanup;
	}
	if (inputContext->duration <= 0)
	{
		errorMessage = "source video duration is unavailable";
		goto cleanup;
	}
	mediaDurationMilliseconds = static_cast<int32_t>(std::min<int64_t>(INT32_MAX, inputContext->duration / 1000));
	for (unsigned int i = 0; i < inputContext->nb_streams; ++i)
	{
		AVStream* stream = inputContext->streams[i];
		if (stream != nullptr && stream->codecpar != nullptr && stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStreamIndex = static_cast<int>(i);
			break;
		}
	}
	if (videoStreamIndex < 0 || mediaDurationMilliseconds <= 0)
	{
		errorMessage = "source video has no playable video stream";
		goto cleanup;
	}
	if (startMilliseconds < 0 || startMilliseconds >= mediaDurationMilliseconds || durationMilliseconds <= 0)
	{
		errorMessage = "requested video segment is outside media duration";
		goto cleanup;
	}
	{
		AVStream* videoStream = inputContext->streams[videoStreamIndex];
		int64_t seekTimestamp = av_rescale_q(startMilliseconds, AVRational{1, 1000}, videoStream->time_base);
		result = av_seek_frame(inputContext, videoStreamIndex, seekTimestamp, AVSEEK_FLAG_BACKWARD);
		if (result < 0)
		{
			errorMessage = "seek source video failed: " + ffmpegErrorText(result);
			goto cleanup;
		}
		avformat_flush(inputContext);
	}

	streamMapping.assign(inputContext->nb_streams, -1);
	result = avformat_alloc_output_context2(&outputContext, nullptr, "mp4", temporaryPath.c_str());
	if (result < 0 || outputContext == nullptr)
	{
		errorMessage = result < 0 ? "create MP4 segment failed: " + ffmpegErrorText(result) : "create MP4 segment failed";
		goto cleanup;
	}
	for (unsigned int i = 0; i < inputContext->nb_streams; ++i)
	{
		AVStream* inputStream = inputContext->streams[i];
		if (inputStream == nullptr || inputStream->codecpar == nullptr)
		{
			continue;
		}
		AVMediaType type = inputStream->codecpar->codec_type;
		if (type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO)
		{
			continue;
		}
		AVStream* outputStream = avformat_new_stream(outputContext, nullptr);
		if (outputStream == nullptr)
		{
			errorMessage = "create MP4 segment stream failed";
			goto cleanup;
		}
		streamMapping[i] = outputStream->index;
		result = avcodec_parameters_copy(outputStream->codecpar, inputStream->codecpar);
		if (result < 0)
		{
			errorMessage = "copy MP4 segment stream metadata failed: " + ffmpegErrorText(result);
			goto cleanup;
		}
		outputStream->codecpar->codec_tag = 0;
		outputStream->time_base = inputStream->time_base;
	}
	if ((outputContext->oformat->flags & AVFMT_NOFILE) == 0)
	{
		result = avio_open(&outputContext->pb, temporaryPath.c_str(), AVIO_FLAG_WRITE);
		if (result < 0)
		{
			errorMessage = "open MP4 segment output failed: " + ffmpegErrorText(result);
			goto cleanup;
		}
		outputOpened = true;
	}
	av_dict_set(&muxOptions, "movflags", "faststart", 0);

	packet = av_packet_alloc();
	if (packet == nullptr)
	{
		errorMessage = "allocate video segment packet failed";
		goto cleanup;
	}
	{
		const int64_t endUs = static_cast<int64_t>(startMilliseconds + durationMilliseconds) * 1000;
		int64_t segmentStartUs = AV_NOPTS_VALUE;
		bool foundVideoKeyFrame = false;
		int packetCount = 0;
		while (!foundVideoKeyFrame && packetCount < 2048)
		{
			result = av_read_frame(inputContext, packet);
			if (result < 0)
			{
				errorMessage = "read source video segment failed: " + ffmpegErrorText(result);
				goto cleanup;
			}
			++packetCount;
			int streamIndex = packet->stream_index;
			AVStream* stream = streamIndex >= 0 && streamIndex < static_cast<int>(inputContext->nb_streams) ? inputContext->streams[streamIndex] : nullptr;
			if (stream == nullptr || (stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO && stream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO))
			{
				av_packet_unref(packet);
				continue;
			}
			if (streamIndex == videoStreamIndex && (packet->flags & AV_PKT_FLAG_KEY) == 0)
			{
				av_packet_unref(packet);
				continue;
			}
			int64_t timestampUs = packetTimestampUs(packet, stream);
			if (timestampUs == AV_NOPTS_VALUE)
			{
				av_packet_unref(packet);
				continue;
			}
			if (segmentStartUs == AV_NOPTS_VALUE || timestampUs < segmentStartUs) segmentStartUs = timestampUs;
			SegmentPacket saved;
			saved.packet = av_packet_clone(packet);
			saved.streamIndex = streamIndex;
			saved.timestampUs = timestampUs;
			av_packet_unref(packet);
			if (saved.packet == nullptr)
			{
				errorMessage = "copy initial video segment packet failed";
				goto cleanup;
			}
			initialPackets.push_back(saved);
			if (streamIndex == videoStreamIndex) foundVideoKeyFrame = true;
		}
		if (!foundVideoKeyFrame || segmentStartUs == AV_NOPTS_VALUE)
		{
			errorMessage = "no key frame found near requested video segment";
			goto cleanup;
		}
		actualStartMilliseconds = static_cast<int32_t>(std::max<int64_t>(0, segmentStartUs / 1000));
		result = avformat_write_header(outputContext, &muxOptions);
		if (result < 0)
		{
			errorMessage = "write MP4 segment header failed: " + ffmpegErrorText(result);
			goto cleanup;
		}
		headerWritten = true;
		for (size_t i = 0; i < initialPackets.size(); ++i)
		{
			if (initialPackets[i].timestampUs <= endUs &&
			    !writePacket(outputContext, inputContext, streamMapping, initialPackets[i].packet, segmentStartUs, errorMessage))
			{
				goto cleanup;
			}
		}
		freeBufferedPackets(initialPackets);

		while (true)
		{
			result = av_read_frame(inputContext, packet);
			if (result == AVERROR_EOF) break;
			if (result < 0)
			{
				errorMessage = "read source video segment failed: " + ffmpegErrorText(result);
				goto cleanup;
			}
			int streamIndex = packet->stream_index;
			AVStream* stream = streamIndex >= 0 && streamIndex < static_cast<int>(inputContext->nb_streams) ? inputContext->streams[streamIndex] : nullptr;
			if (stream == nullptr || streamMapping[streamIndex] < 0)
			{
				av_packet_unref(packet);
				continue;
			}
			int64_t timestampUs = packetTimestampUs(packet, stream);
			if (timestampUs != AV_NOPTS_VALUE && timestampUs >= endUs)
			{
				av_packet_unref(packet);
				if (streamIndex == videoStreamIndex) break;
				continue;
			}
			if (!writePacket(outputContext, inputContext, streamMapping, packet, segmentStartUs, errorMessage))
			{
				av_packet_unref(packet);
				goto cleanup;
			}
			av_packet_unref(packet);
		}
	}
	result = av_write_trailer(outputContext);
	if (result < 0)
	{
		errorMessage = "finish MP4 segment failed: " + ffmpegErrorText(result);
		goto cleanup;
	}
	resultOk = true;

cleanup:
	freeBufferedPackets(initialPackets);
	av_packet_free(&packet);
	av_dict_free(&muxOptions);
	if (outputOpened && outputContext != nullptr)
	{
		avio_closep(&outputContext->pb);
	}
	if (outputContext != nullptr) avformat_free_context(outputContext);
	if (inputContext != nullptr) avformat_close_input(&inputContext);
	return resultOk;
}

uint64_t sourceLastWriteStamp(const std::string& sourcePath)
{
	WIN32_FILE_ATTRIBUTE_DATA attributes = {};
	if (!GetFileAttributesExA(sourcePath.c_str(), GetFileExInfoStandard, &attributes)) return 0;
	ULARGE_INTEGER stamp;
	stamp.LowPart = attributes.ftLastWriteTime.dwLowDateTime;
	stamp.HighPart = attributes.ftLastWriteTime.dwHighDateTime;
	return stamp.QuadPart;
}
}
#endif

bool CookVideoSegmentService::createSegment(const std::string& sourcePath,
	                                       const std::string& cacheDirectory,
	                                       const std::string& videoId,
	                                       int32_t sourceVersionSeconds,
	                                       int64_t maxSegmentBytes,
	                                       int32_t startMilliseconds,
	                                       int32_t durationMilliseconds,
	                                       std::string& segmentPath,
	                                       int32_t& actualStartMilliseconds,
	                                       int32_t& mediaDurationMilliseconds,
	                                       std::string& errorMessage)
{
#ifndef _WIN32
	(void)sourcePath;
	(void)cacheDirectory;
	(void)videoId;
	(void)sourceVersionSeconds;
	(void)maxSegmentBytes;
	(void)startMilliseconds;
	(void)durationMilliseconds;
	segmentPath.clear();
	actualStartMilliseconds = 0;
	mediaDurationMilliseconds = 0;
	errorMessage = "time-segment generation is currently supported by the Windows server build";
	return false;
#else
	segmentPath.clear();
	errorMessage.clear();
	actualStartMilliseconds = 0;
	mediaDurationMilliseconds = 0;
	if (sourcePath.empty() || cacheDirectory.empty() || videoId.empty() || maxSegmentBytes <= 0 || startMilliseconds < 0 || durationMilliseconds <= 0)
	{
		errorMessage = "invalid video segment arguments";
		return false;
	}
	std::ifstream source(sourcePath.c_str(), std::ios::binary | std::ios::ate);
	if (!source.good() || source.tellg() <= 0)
	{
		errorMessage = "source video is empty or unavailable";
		return false;
	}
	const int64_t sourceBytes = static_cast<int64_t>(source.tellg());
	const uint64_t sourceWriteStamp = sourceLastWriteStamp(sourcePath);
	std::ostringstream name;
	name << safePathPart(videoId) << '_' << sourceVersionSeconds << '_' << sourceBytes << '_' << sourceWriteStamp
	     << '_' << startMilliseconds << '_' << durationMilliseconds << ".mp4";
	segmentPath = CookHelper::appendPathSeparator(cacheDirectory) + name.str();
	std::mutex& lock = segmentLockFor(segmentPath);
	std::lock_guard<std::mutex> guard(lock);
	if (CSystem::DirOrFileExist(segmentPath))
	{
		std::ifstream cached(segmentPath.c_str(), std::ios::binary | std::ios::ate);
		std::ifstream metadata((segmentPath + ".meta").c_str());
		int cachedActualStart = 0;
		int cachedDuration = 0;
		const std::streamoff cachedBytes = cached.tellg();
		if (cached.good() && cachedBytes > 0 && cachedBytes <= maxSegmentBytes &&
		    metadata >> cachedActualStart >> cachedDuration && cachedDuration > 0)
		{
			actualStartMilliseconds = cachedActualStart;
			mediaDurationMilliseconds = cachedDuration;
			return true;
		}
		std::remove(segmentPath.c_str());
		std::remove((segmentPath + ".meta").c_str());
	}
	if (!CookHelper::ensureDirectoryTree(cacheDirectory))
	{
		errorMessage = "create video segment cache directory failed";
		return false;
	}
	int64_t sequence = 0;
	static std::atomic<int64_t> nextTemporaryId(0);
	sequence = ++nextTemporaryId;
	std::string temporaryPath = segmentPath + ".tmp_" + std::to_string(sequence);
	if (!extractSegment(sourcePath, temporaryPath, startMilliseconds, durationMilliseconds,
	                    actualStartMilliseconds, mediaDurationMilliseconds, errorMessage))
	{
		std::remove(temporaryPath.c_str());
		segmentPath.clear();
		return false;
	}
	std::ifstream output(temporaryPath.c_str(), std::ios::binary | std::ios::ate);
	if (!output.good() || output.tellg() <= 0)
	{
		std::remove(temporaryPath.c_str());
		segmentPath.clear();
		errorMessage = "generated video segment is empty";
		return false;
	}
	if (static_cast<int64_t>(output.tellg()) > maxSegmentBytes)
	{
		std::remove(temporaryPath.c_str());
		segmentPath.clear();
		errorMessage = "generated video segment exceeds configured size limit";
		return false;
	}
	output.close();
	std::remove(segmentPath.c_str());
	if (std::rename(temporaryPath.c_str(), segmentPath.c_str()) != 0)
	{
		std::remove(temporaryPath.c_str());
		segmentPath.clear();
		errorMessage = "publish generated video segment failed";
		return false;
	}
	std::ofstream metadata((segmentPath + ".meta").c_str(), std::ios::out | std::ios::trunc);
	if (metadata.good())
	{
		metadata << actualStartMilliseconds << ' ' << mediaDurationMilliseconds << '\n';
	}
	return true;
#endif
}
