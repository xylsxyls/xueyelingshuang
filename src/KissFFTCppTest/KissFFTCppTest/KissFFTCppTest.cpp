#include "KissFFTCppTest.h"
#include "PlatformTest/PlatformTestAPI.h"
#include <cstring>
#include "KissFFTCpp/KissFFTCppAPI.h"
#include <climits>
#include <cstdio>
#include <cmath>
#include <vector>

#define CHECK(expression) do { ++checks; if (!(expression)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expression); return 1; } } while (false)

/** 使用公开接口运行固定输入的性能测试
@return 正确性校验通过为0，失败返回非0
*/
static int RunBenchmarks()
{
    const int32_t length = 1024;
    std::vector<double> input(length, 0.0), restored(length);
    std::vector<KissFFTCppComplex> bins(length / 2 + 1);
    input[0] = 1.0;
    KissFFTCpp forward, inverse;
    if (!forward.open(length, false) || !inverse.open(length, true))
    {
        return 1;
    }
    // 同长度同方向重复打开后变换一个单位脉冲，用频谱实部校验实际计算
    LibraryBenchmark::run("kissfft.open_forward.1024", [&](uint32_t count) -> uint64_t
    {
        uint64_t valid = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            if (forward.open(length, false) && forward.forward(input.data(), length, bins.data(), bins.size()))
            {
                valid += static_cast<uint64_t>(bins[37].real);
            }
        }
        if (valid != count)
        {
            throw std::runtime_error("invalid impulse spectrum");
        }
        return valid;
    });
    // 每次在两个既有计划上完成一对变换，检查未归一化的时间域输出
    LibraryBenchmark::run("kissfft.forward_inverse.1024", [&](uint32_t count) -> uint64_t
    {
        uint64_t valid = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            if (forward.forward(input.data(), length, bins.data(), bins.size()) &&
                inverse.inverse(bins.data(), bins.size(), restored.data(), length))
            {
                valid += static_cast<uint64_t>(restored[0]);
            }
        }
        if (valid != static_cast<uint64_t>(count) * length)
        {
            throw std::runtime_error("invalid inverse FFT");
        }
        return valid;
    });
    for (size_t i = 1; i < restored.size(); ++i)
    {
        if (restored[i] != 0)
        {
            return 1;
        }
    }
    return 0;
}

/** 默认运行全部功能组，可选择一个ID、列出选项或单独运行性能模式
@param [in] argc 参数数量
@param [in] argv 命令行参数
@return 通过返回0，断言失败返回1，未知参数返回2
*/
static int RunSuite(int argc, char* argv[])
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return RunBenchmarks();
    }

    const char* ids[] = {"invalid", "dft-2", "dft-6", "dft-16", "dft-64", "dft-120", "direction", "dft-prime"};
    int selected = -1;
    if (argc == 2 && std::strcmp(argv[1], "--list") == 0)
    {
        std::puts("invalid dft-2 dft-6 dft-16 dft-64 dft-120 direction dft-prime");
        return 0;
    }
    if (argc > 1)
    {
        if (argc != 2 || std::strncmp(argv[1], "--case=", 7) != 0)
        {
            return 2;
        }
        for (int i = 0; i < static_cast<int>(sizeof(ids) / sizeof(ids[0])); ++i)
        {
            if (std::strcmp(argv[1] + 7, ids[i]) == 0)
            {
                selected = i;
                break;
            }
        }
        if (selected < 0)
        {
            return 2;
        }
    }
    int checks = 0;
    KissFFTCpp forward, inverse;
    double initialSamples[] = {1.0, -1.0};
    KissFFTCppComplex initialBins[2] = {};
    if (selected <= 0)
    {
        CHECK(!forward.forward(initialSamples, 2, initialBins, 2));
        CHECK(!inverse.inverse(initialBins, 2, initialSamples, 2));
        CHECK(!forward.open(-2, false));
        CHECK(!forward.open(0, false));
        CHECK(!forward.open(INT_MAX - 1, false));
    }
    const int sizes[] = {2, 6, 16, 64, 120, 14, 34, 194};
    for (size_t j = 0; j < sizeof(sizes) / sizeof(sizes[0]); ++j)
    {
        const int group = j < 5 ? static_cast<int>(j) + 1 : 7;
        if (selected >= 0 && selected != group)
        {
            continue;
        }
        const int count = sizes[j];
        const int bins = count / 2 + 1;
        std::vector<double> samples(count), reconstructed(count);
        std::vector<KissFFTCppComplex> output(bins);
        CHECK(forward.open(count, false) && inverse.open(count, true));
        CHECK(!forward.open(count + 1, false));
        for (int i = 0; i < count; ++i) { samples[i] = (i * 13 % 19) - 7; }
        CHECK(!forward.forward(nullptr, count, output.data(), bins));
        CHECK(!forward.forward(samples.data(), count, nullptr, bins));
        CHECK(!forward.forward(samples.data(), count - 1, output.data(), bins));
        CHECK(!inverse.forward(samples.data(), count, output.data(), bins));
        CHECK(!forward.forward(samples.data(), count, output.data(), bins - 1));
        CHECK(forward.forward(samples.data(), count, output.data(), bins));
        for (int k = 0; k < bins; ++k)
        {
            double real = 0, imaginary = 0;
            for (int i = 0; i < count; ++i)
            {
                const double angle = -2.0 * std::acos(-1.0) * k * i / count;
                real += samples[i] * std::cos(angle);
                imaginary += samples[i] * std::sin(angle);
            }
            CHECK(std::abs(real - output[k].real) < 1e-8 && std::abs(imaginary - output[k].imaginary) < 1e-8);
        }
        CHECK(!inverse.inverse(output.data(), bins, nullptr, count));
        CHECK(!inverse.inverse(nullptr, bins, reconstructed.data(), count));
        CHECK(!inverse.inverse(output.data(), bins - 1, reconstructed.data(), count));
        CHECK(!inverse.inverse(output.data(), bins, reconstructed.data(), count - 1));
        for (int i = 0; i < count; ++i) { CHECK(reconstructed[i] == 0.0); }
        CHECK(!forward.inverse(output.data(), bins, reconstructed.data(), count));
        CHECK(inverse.inverse(output.data(), bins, reconstructed.data(), count));
        for (int i = 0; i < count; ++i) { CHECK(std::abs(reconstructed[i] / count - samples[i]) < 1e-9); }
    }
    if (selected < 0 || selected == 6)
    {
        // 同一对象改变方向后新计划立即生效；重复打开相同计划不影响结果
        CHECK(forward.open(2, true) && forward.open(2, true));
        initialBins[0].real = 0.0;
        initialBins[1].real = 2.0;
        CHECK(forward.inverse(initialBins, 2, initialSamples, 2));
        CHECK(initialSamples[0] == 2.0 && initialSamples[1] == -2.0);
        CHECK(!forward.forward(initialSamples, 2, initialBins, 2));
    }
    std::printf("KissFFTCpp checks=%d failures=0\n", checks);
    return 0;
}

/** 执行所选功能组，默认运行完整套件。
@param [in] argc 命令行参数数量
@param [in] argv 命令行参数数组
@return 0 表示通过，1 表示断言失败，2 表示参数无效
*/
int main(int argc, char* argv[])
{
    const LibraryNamedTestCase cases[] =
    {
        {KissFFTCppTestInvalid, false, "invalid"},
        {KissFFTCppTestDft2, false, "dft-2"},
        {KissFFTCppTestDft6, false, "dft-6"},
        {KissFFTCppTestDft16, false, "dft-16"},
        {KissFFTCppTestDft64, false, "dft-64"},
        {KissFFTCppTestDft120, false, "dft-120"},
        {KissFFTCppTestDirection, false, "direction"},
        {KissFFTCppTestDftPrime, false, "dft-prime"},
    };
    return LibraryCaseSelection::runNamed(argc, argv, cases, RunSuite);
}