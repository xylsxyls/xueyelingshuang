#include "NetCryptoTestCases.h"
#include <cstdio>
#include "NetCryptoCertificateTest.h"
#include <chrono>
#include <stdexcept>

// 模块独立验证期限；此入口不联网、不修改证书或正式SDK。
static std::chrono::steady_clock::time_point g_cryptoDeadline;

/** 复用正式测试用例的检查点，由外部另加进程级超时
*/
void netTestCheckpoint()
{
    if (std::chrono::steady_clock::now() > g_cryptoDeadline)
    {
        throw std::runtime_error("crypto case deadline");
    }
}

/** 运行密码分层回归，完整网关另执行相同用例
@return 全部成功为0，否则为1
*/
int main()
{
    const char* names[] = {"hashBoundary", "derivationBoundary", "partsBoundary", "certificateBoundary"};
    void (*cases[])() = {&NetCryptoTestCases::hashBoundary, &NetCryptoTestCases::derivationBoundary,
        &NetCryptoTestCases::partsBoundary, &NetCryptoCertificateTest::run};
    unsigned failed = 0;
    for (unsigned index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index)
    {
        g_cryptoDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
        try
        {
            cases[index]();
            std::printf("PASS %s\n", names[index]);
        }
        catch (const std::exception& error)
        {
            ++failed;
            std::printf("FAIL %s: %s\n", names[index], error.what());
        }
        catch (...)
        {
            ++failed;
            std::printf("FAIL %s: unexpected exception\n", names[index]);
        }
    }
    return failed == 0 ? 0 : 1;
}