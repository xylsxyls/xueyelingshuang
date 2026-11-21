#include "KissFFTCpp.h"
#include "KissFFT/kiss_fftr.h"
#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "kissfftd.lib")
#else
#pragma comment(lib, "kissfft.lib")
#endif
#endif
#include <climits>
#include <cstdlib>

KissFFTCpp::KissFFTCpp() :
m_plan(nullptr),
m_bins(nullptr),
m_length(0),
m_inverse(false)
{

}

KissFFTCpp::~KissFFTCpp()
{
    std::free(m_plan);
    std::free(m_bins);
}

bool KissFFTCpp::open(int32_t length, bool inverse)
{
    // 按每点三组复数的上界限制计划和工作区，兼顾底层int运算及32位分配范围
    if (length < 2 || (length & 1) != 0
        || length > INT_MAX / (3 * static_cast<int>(sizeof(kiss_fft_cpx))))
    {
        return false;
    }
    // 计划只取决于点数和方向，重复打开时沿用原工作区
    if (m_length == length && m_inverse == inverse)
    {
        return true;
    }
    kiss_fftr_cfg plan = kiss_fftr_alloc(length, inverse ? 1 : 0, nullptr, nullptr);
    if (plan == nullptr)
    {
        return false;
    }
    void* bins = std::malloc(static_cast<size_t>(length / 2 + 1) * sizeof(kiss_fft_cpx));
    if (bins == nullptr)
    {
        std::free(plan);
        return false;
    }
    std::free(m_plan);
    std::free(m_bins);
    m_plan = plan;
    m_bins = bins;
    m_length = length;
    m_inverse = inverse;
    return true;
}

bool KissFFTCpp::forward(const double* input, size_t inputCount,
    KissFFTCppComplex* output, size_t outputCount)
{
    if (m_plan == nullptr || m_inverse || input == nullptr || output == nullptr
        || inputCount != static_cast<size_t>(m_length)
        || outputCount != static_cast<size_t>(m_length / 2 + 1))
    {
        return false;
    }
    kiss_fft_cpx* bins = static_cast<kiss_fft_cpx*>(m_bins);
    kiss_fftr(static_cast<kiss_fftr_cfg>(m_plan), input, bins);
    for (size_t i = 0; i < outputCount; ++i)
    {
        output[i].real = bins[i].r;
        output[i].imaginary = bins[i].i;
    }
    return true;
}

bool KissFFTCpp::inverse(const KissFFTCppComplex* input, size_t inputCount,
    double* output, size_t outputCount)
{
    if (m_plan == nullptr || !m_inverse || input == nullptr || output == nullptr
        || inputCount != static_cast<size_t>(m_length / 2 + 1)
        || outputCount != static_cast<size_t>(m_length))
    {
        return false;
    }
    kiss_fft_cpx* bins = static_cast<kiss_fft_cpx*>(m_bins);
    for (size_t i = 0; i < inputCount; ++i)
    {
        bins[i].r = input[i].real;
        bins[i].i = input[i].imaginary;
    }
    kiss_fftri(static_cast<kiss_fftr_cfg>(m_plan), bins, output);
    return true;
}

// 使用示例（独立消费者，已在 Windows x64 Release 验证）
//#include "KissFFTCpp/KissFFTCppAPI.h"
//#include <cmath>
//#include <cstdio>
//
//int main()
//{
//    // 一个四点单位脉冲，预期三个非负频点的实部均为1
//    const double input[] = {1.0, 0.0, 0.0, 0.0};
//    KissFFTCppComplex spectrum[3] = {};
//    double restored[4] = {};
//    KissFFTCpp forward;
//    KissFFTCpp inverse;
//    if (!forward.open(4, false) || !inverse.open(4, true) ||
//        !forward.forward(input, 4, spectrum, 3) || !inverse.inverse(spectrum, 3, restored, 4))
//    {
//        return 1;
//    }
//    for (size_t i = 0; i < 3; ++i)
//    {
//        if (std::abs(spectrum[i].real - 1.0) > 1e-10 || std::abs(spectrum[i].imaginary) > 1e-10)
//        {
//            return 2;
//        }
//    }
//    // 逆变换未归一化，除以点数即可还原时域样本
//    for (size_t i = 0; i < 4; ++i)
//    {
//        if (std::abs(restored[i] / 4 - input[i]) > 1e-10)
//        {
//            return 3;
//        }
//    }
//    std::printf("DC=%.1f, recovered impulse=%.1f\n", spectrum[0].real, restored[0] / 4);
//    return 0;
//}