inline FFmpegCppRational::FFmpegCppRational()
    : numerator(0)
    , denominator(1)
{
}

inline FFmpegCppRational::FFmpegCppRational(int32_t numeratorValue, int32_t denominatorValue)
    : numerator(numeratorValue)
    , denominator(denominatorValue)
{
}

inline double FFmpegCppRational::toDouble(double defaultValue) const
{
    if (denominator == 0)
    {
        return defaultValue;
    }
    return static_cast<double>(numerator) / static_cast<double>(denominator);
}

inline FFmpegCppStreamInfo::FFmpegCppStreamInfo()
    : streamIndex(-1)
    , mediaType(FFmpegCppMediaTypeUnknown)
    , codecName()
    , codecLongName()
    , profile()
    , width(0)
    , height(0)
    , rotationDegree(0)
    , durationMilliseconds(0)
    , bitRate(0)
    , frameRate()
    , timeBase()
    , sampleRate(0)
    , channels(0)
    , language()
{
}

inline bool FFmpegCppStreamInfo::isVideo() const
{
    return mediaType == FFmpegCppMediaTypeVideo;
}

inline bool FFmpegCppStreamInfo::isAudio() const
{
    return mediaType == FFmpegCppMediaTypeAudio;
}

inline FFmpegCppMediaInfo::FFmpegCppMediaInfo()
    : filePath()
    , formatName()
    , formatLongName()
    , fileSizeBytes(0)
    , durationMilliseconds(0)
    , bitRate(0)
    , hasVideo(false)
    , hasAudio(false)
    , mainVideoStreamIndex(-1)
    , mainAudioStreamIndex(-1)
    , streams()
{
}

inline void FFmpegCppMediaInfo::reset()
{
    filePath.clear();
    formatName.clear();
    formatLongName.clear();
    fileSizeBytes = 0;
    durationMilliseconds = 0;
    bitRate = 0;
    hasVideo = false;
    hasAudio = false;
    mainVideoStreamIndex = -1;
    mainAudioStreamIndex = -1;
    streams.clear();
}

inline bool FFmpegCppMediaInfo::getMainVideoStreamInfo(FFmpegCppStreamInfo* streamInfo) const
{
    if (streamInfo == nullptr)
    {
        return false;
    }

    for (std::vector<FFmpegCppStreamInfo>::const_iterator iter = streams.begin(); iter != streams.end(); ++iter)
    {
        if (iter->streamIndex == mainVideoStreamIndex && iter->isVideo())
        {
            *streamInfo = *iter;
            return true;
        }
    }
    return false;
}

inline bool FFmpegCppMediaInfo::getMainAudioStreamInfo(FFmpegCppStreamInfo* streamInfo) const
{
    if (streamInfo == nullptr)
    {
        return false;
    }

    for (std::vector<FFmpegCppStreamInfo>::const_iterator iter = streams.begin(); iter != streams.end(); ++iter)
    {
        if (iter->streamIndex == mainAudioStreamIndex && iter->isAudio())
        {
            *streamInfo = *iter;
            return true;
        }
    }
    return false;
}

inline FFmpegCppImageData::FFmpegCppImageData()
    : imageFormat(FFmpegCppImageFormatJpeg)
    , mimeType("image/jpeg")
    , width(0)
    , height(0)
    , timestampMilliseconds(0)
    , data()
{
}

inline void FFmpegCppImageData::reset()
{
    imageFormat = FFmpegCppImageFormatJpeg;
    mimeType = "image/jpeg";
    width = 0;
    height = 0;
    timestampMilliseconds = 0;
    data.clear();
}

inline bool FFmpegCppImageData::empty() const
{
    return data.empty();
}

inline FFmpegCppFrameExtractOption::FFmpegCppFrameExtractOption()
    : timestampMilliseconds(0)
    , maxWidth(0)
    , maxHeight(0)
    , keepAspectRatio(true)
    , applyRotation(true)
    , imageFormat(FFmpegCppImageFormatJpeg)
    , jpegQuality(85)
    , timeoutMilliseconds(10000)
    , allowHardwareDecode(false)
{
}

inline FFmpegCppTranscodeOption::FFmpegCppTranscodeOption()
    : outputContainer("mp4")
    , videoCodec(FFmpegCppVideoCodecH264)
    , audioCodec(FFmpegCppAudioCodecCopy)
    , maxWidth(0)
    , maxHeight(0)
    , keepAspectRatio(true)
    , applyRotation(true)
    , videoBitRateKbps(0)
    , audioBitRateKbps(128)
    , crf(23)
    , preset("medium")
    , fastStart(true)
    , overwriteOutput(false)
    , removeAudio(false)
    , allowHardwareEncode(false)
    , timeoutMilliseconds(0)
{
}

inline FFmpegCppProgress::FFmpegCppProgress()
    : processedMilliseconds(0)
    , totalMilliseconds(0)
    , outputBytes(0)
    , percent(0.0)
    , finished(false)
    , cancelled(false)
    , message()
{
}