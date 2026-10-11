#pragma once

#include <stdint.h>
#include <string>

/** Creates independently playable MP4 time windows from published media. */
class CookVideoSegmentService
{
public:
	static bool createSegment(const std::string& sourcePath,
	                          const std::string& cacheDirectory,
	                          const std::string& videoId,
	                          int32_t sourceVersionSeconds,
	                          int64_t maxSegmentBytes,
	                          int32_t startMilliseconds,
	                          int32_t durationMilliseconds,
	                          std::string& segmentPath,
	                          int32_t& actualStartMilliseconds,
	                          int32_t& mediaDurationMilliseconds,
	                          std::string& errorMessage);
};
