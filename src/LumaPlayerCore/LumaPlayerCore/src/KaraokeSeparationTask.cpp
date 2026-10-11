#include "KaraokeSeparationTask.h"
#include "KaraokeSpectrum.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>

KaraokeSeparationTask::KaraokeSeparationTask(KaraokeStorage* source, size_t frames,
    size_t start, size_t total, size_t step, size_t border) :
m_start(start),
m_prediction(KaraokeSpectrum::Elements),
m_done(false),
m_analyzeMs(0),
m_reconstructMs(0),
m_source(source),
m_frames(frames),
m_total(total),
m_step(step),
m_border(border),
m_valid((std::min)(size_t(KaraokeSpectrum::Frames), total - start)),
m_reconstruct(false),
m_cancel(false)
{

}

void KaraokeSeparationTask::analyze()
{
    const size_t chunk = KaraokeSpectrum::Frames;
    std::vector<float> audio(chunk * 2, 0);
    const size_t begin = m_start < m_border ? m_border - m_start : 0;
    const size_t end = (std::min)(chunk, m_frames + m_border - m_start);
    if (begin < end && !m_source->read((m_start + begin - m_border) * 8, &audio[begin * 2], (end - begin) * 8))
    {
        m_error = m_source->error();
        return;
    }
    if (!KaraokeSpectrum::analyze(audio, &m_spectrum))
    {
        m_error = "Audio spectrum analysis failed";
        return;
    }
    // Upstream MDX preprocessing removes the first three frequency bins on all four planes.
    for (size_t plane = 0; plane < 4; ++plane)
    {
        std::fill(m_spectrum.begin() + plane * KaraokeSpectrum::Bins * KaraokeSpectrum::Steps,
            m_spectrum.begin() + plane * KaraokeSpectrum::Bins * KaraokeSpectrum::Steps + 3 * KaraokeSpectrum::Steps, 0.0f);
    }
}

void KaraokeSeparationTask::DoTask()
{
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    try
    {
        if (m_cancel.load())
        {
            m_error = "Vocal separation canceled";
        }
        else if (m_reconstruct)
        {
            if (!KaraokeSpectrum::synthesize(m_prediction, &m_separated))
            {
                m_error = "Invalid model result during audio reconstruction";
            }
        }
        else
        {
            analyze();
        }
    }
    catch (const std::exception& error)
    {
        m_error = error.what();
    }
    catch (...)
    {
        m_error = "Unexpected separation CPU stage failure";
    }
    const int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    if (m_reconstruct)
    {
        m_reconstructMs = elapsed;
    }
    else
    {
        m_analyzeMs = elapsed;
    }
    m_done.store(true);
}

void KaraokeSeparationTask::StopTask()
{
    m_cancel.store(true);
}

void KaraokeSeparationTask::reconstruct()
{
    m_reconstruct = true;
    m_done.store(false);
}

bool KaraokeSeparationTask::accumulate(KaraokeStorage* output)
{
    const size_t chunk = KaraokeSpectrum::Frames;
    if (output == nullptr)
    {
        m_error = "Missing separation reconstruction storage";
        return false;
    }
    if (m_start >= m_frames + m_border)
    {
        return true;
    }
    const size_t begin = m_start < m_border ? m_border - m_start : 0;
    const size_t end = (std::min)(m_valid, m_frames + m_border - m_start);
    if (begin >= end)
    {
        return true;
    }
    const size_t base = m_start + begin - m_border;
    const size_t count = end - begin;
    std::vector<float> sums(count * 3);
    if (!output->read(base * 12, sums.data(), sums.size() * 4))
    {
        m_error = output->error();
        return false;
    }
    for (size_t frame = base; frame < base + count; ++frame)
    {
        const size_t i = frame - m_start + m_border;
        const float weight = static_cast<float>(0.5 * (1.0 - std::cos(6.28318530717958647692 * i / (chunk - 1))));
        const size_t index = frame - base;
        sums[index * 3 + 2] += weight;
        for (size_t c = 0; c < 2; ++c)
        {
            sums[index * 3 + c] += m_separated[i * 2 + c] * weight;
        }
    }
    if (!output->write(base * 12, sums.data(), sums.size() * 4))
    {
        m_error = output->error();
        return false;
    }
    return true;
}