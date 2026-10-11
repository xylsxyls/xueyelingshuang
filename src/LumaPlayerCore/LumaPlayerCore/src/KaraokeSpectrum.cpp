#include "KaraokeSpectrum.h"
#include "KissFFTCpp/KissFFTCppAPI.h"
#include <cmath>

const int KaraokeSpectrum::FftSize;
const int KaraokeSpectrum::Hop;
const int KaraokeSpectrum::Bins;
const int KaraokeSpectrum::Steps;
const int KaraokeSpectrum::Frames;
const int KaraokeSpectrum::Elements;

bool KaraokeSpectrum::analyze(const std::vector<float>& audio, std::vector<float>* spectrum)
{
    if (audio.size() != Frames * 2 || spectrum == nullptr)
    {
        return false;
    }
    spectrum->assign(Elements, 0);
    std::vector<double> frame(FftSize), window(FftSize);
    std::vector<KissFFTCppComplex> bins(FftSize / 2 + 1);
    for (int i = 0; i < FftSize; ++i)
    {
        window[i] = 0.5 * (1.0 - std::cos(6.28318530717958647692 * i / FftSize));
    }
    KissFFTCpp fft;
    if (!fft.open(FftSize, false))
    {
        return false;
    }
    for (int channel = 0; channel < 2; ++channel)
    {
        for (int t = 0; t < Steps; ++t)
        {
            for (int i = 0; i < FftSize; ++i)
            {
                int sample = t * Hop + i - FftSize / 2;
                if (sample < 0)
                {
                    sample = -sample;
                }
                if (sample >= Frames)
                {
                    sample = 2 * (Frames - 1) - sample;
                }
                frame[i] = audio[sample * 2 + channel] * window[i];
            }
            if (!fft.forward(frame.data(), frame.size(), bins.data(), bins.size()))
            {
                return false;
            }
            for (int f = 0; f < Bins; ++f)
            {
                const size_t index = (channel * 2 * Bins + f) * Steps + t;
                (*spectrum)[index] = static_cast<float>(bins[f].real);
                (*spectrum)[index + Bins * Steps] = static_cast<float>(bins[f].imaginary);
            }
        }
    }
    return true;
}

bool KaraokeSpectrum::synthesize(const std::vector<float>& spectrum, std::vector<float>* audio)
{
    if (spectrum.size() != Elements || audio == nullptr)
    {
        return false;
    }
    audio->assign(Frames * 2, 0);
    std::vector<double> frame(FftSize), window(FftSize), envelope(Frames + FftSize, 0);
    std::vector<double> signal((Frames + FftSize) * 2, 0);
    std::vector<KissFFTCppComplex> bins(FftSize / 2 + 1);
    for (int i = 0; i < FftSize; ++i)
    {
        window[i] = 0.5 * (1.0 - std::cos(6.28318530717958647692 * i / FftSize));
    }
    KissFFTCpp fft;
    if (!fft.open(FftSize, true))
    {
        return false;
    }
    for (int channel = 0; channel < 2; ++channel)
    {
        for (int t = 0; t < Steps; ++t)
        {
            bins[Bins].real = bins[Bins].imaginary = 0;
            for (int f = 0; f < Bins; ++f)
            {
                const size_t index = (channel * 2 * Bins + f) * Steps + t;
                bins[f].real = spectrum[index];
                bins[f].imaginary = spectrum[index + Bins * Steps];
            }
            if (!fft.inverse(bins.data(), bins.size(), frame.data(), frame.size()))
            {
                return false;
            }
            for (int i = 0; i < FftSize; ++i)
            {
                const int sample = t * Hop + i;
                signal[sample * 2 + channel] += frame[i] * window[i] / FftSize;
                if (channel == 0)
                {
                    envelope[sample] += window[i] * window[i];
                }
            }
        }
    }
    for (int i = 0; i < Frames; ++i)
    {
        for (int c = 0; c < 2; ++c)
        {
            const double value = signal[(i + FftSize / 2) * 2 + c] / envelope[i + FftSize / 2];
            if (!std::isfinite(value))
            {
                return false;
            }
            (*audio)[i * 2 + c] = static_cast<float>(value);
        }
    }
    return true;
}