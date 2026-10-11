#include "KaraokeMediaWriter.h"

KaraokeMediaWriter::KaraokeMediaWriter()
{

}
KaraokeMediaWriter::~KaraokeMediaWriter()
{

}
bool KaraokeMediaWriter::open(const std::string& path, bool video, int width, int height)
{
    return m_writer.open(path, video, width, height);
}
bool KaraokeMediaWriter::audio(const int16_t* samples, size_t frames, int64_t)
{
    return m_writer.audio(samples, frames);
}
bool KaraokeMediaWriter::video(const unsigned char* pixels, size_t bytes, int64_t time, int64_t)
{
    return m_writer.video(pixels, bytes, time);
}
bool KaraokeMediaWriter::finish()
{
    return m_writer.finish();
}
std::string KaraokeMediaWriter::error() const
{
    return m_writer.error();
}