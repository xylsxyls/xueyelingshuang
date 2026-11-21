#pragma once
#include <stdint.h>

/** CEncodeDecode 测试，适用于 Windows、Linux、macOS、Android、iOS、Wasm32 和 Wasm64。
SHA-256、HMAC 和 PBKDF2 与公开固定向量逐字节比较，覆盖输入切分、空输入、容量不足、
输入输出重叠、重复完成、生命周期及擦除。P-256 用例在内存生成独立的测试 CA，验证 CSR、
签发、域名和信任链、密钥配对、摘要签名、参数错误和随机回调失败。
TLS 用例使用 mbedTLS 上游公开 localhost 测试证书，按 17 字节分段握手，传输带 NUL 的明文，
验证有序关闭、错误域名、默认 10000 毫秒及自定义 100 毫秒握手超时，并检查非法配置保留原值。测试在内存中运行，无需网络或外部密钥。
旧格式使用 MD5、SM4 单轮及百万轮标准向量和 AES/RSA 往返，RSA 同时检查最大输入长度。SM2 检查启用后的往返或未启用时的失败结果。
ID 5 的 RSA 分别调用密钥生成、OAEP 加密和解密。有效显式回调在三处均须实际调用，190 字节明文加密产生 256 字节密文，191 字节应被拒绝。失败回调先写入一个字节再返回非零或抛异常，两种情况均需停止，entropyBytes 清零指定范围并保留两侧哨兵，RSA 生成时清空预填公私钥，加解密返回空串。在五个原生端使用空回调生成另一对密钥并完成 OAEP 往返；Wasm 用已有有效密钥检查空回调加解密失败，空回调生成也失败并清空输出。
测试工程自身保存证书验证器、TLS 素材和 SHA-256 累计长度白盒用例。
功能 ID 1 散列、2 派生与擦除、3 多段摘要、4 P-256 证书、5 历史加密和百万轮 SM4、6 受信握手、7 错误域名、8 默认和自定义截止时间。
ID 9 核对默认输入 32768、显式输入 41000 和输出 7 字节处的真实背压，随后用独立的按需增长队列传输 65549 字节明文，检查输出借用期和跨块字节顺序。
无参或 --all 执行全部，用 --cases 1,6-8 指定范围，--instant 与 --stress 分别选择瞬时和较重的固定向量。执行 CEncodeDecodeTest 或 CTest。返回 0 表示全部通过，失败时输出步骤，测试数据不写盘。
*/

/** CEncodeDecodeTest 的稳定功能编号，5 为固定百万轮向量的压力场景。 */
enum CEncodeDecodeTestCase
{
    kCryptoHash = 1, kCryptoDerivation = 2, kCryptoParts = 3, kCryptoCertificate = 4,
    kCryptoLegacy = 5, kCryptoTlsTrusted = 6, kCryptoTlsWrongHost = 7, kCryptoTlsDeadline = 8, kCryptoTlsQueueLimits = 9
};

// 仅用于复现内存测试的字节序列，生成的测试密钥绝不能用于实际通信。
int32_t cEncodeTestEntropy(void* context, uint8_t* output, uint32_t size);

/** 定义 RSA 测试熵源响应一次取熵请求的方式
*/
enum class CEncodeDecodeTestRsaEntropyMode
{
    // 使用可复现的测试字节完整填充缓冲区
    Fill,
    // 部分写入后返回非零错误码
    Reject,
    // 部分写入后抛出测试异常
    Throw
};

/** 记录 RSA 显式熵源的调用次数和响应方式
*/
struct CEncodeDecodeTestRsaEntropy
{
    // 当前取熵请求使用的响应方式
    CEncodeDecodeTestRsaEntropyMode m_mode;
    // 已收到的调用次数，用于验证上下文传递和熵源选择
    uint32_t m_calls;
};

/** Wasm 宿主测试入口
用与 SDK 匹配的 Emscripten 工具链配置本工程，指定 -DBIT=wasm32 或 wasm64、
-DSDK_ROOT=<对应 SDK>、-DLIBRARY_TEST_HOST=ON，构建后运行 ctest --output-on-failure 可验证 Node。
浏览器通过 HTTP 打开构建目录的 LibraryTestHost.html?test=CEncodeDecodeTest。
微信把生成的 CEncodeDecodeTest.js、CEncodeDecodeTest.wasm、LibraryTestHost.js 放入同一分包，调用
await require('./LibraryTestHost')(require('./CEncodeDecodeTest'), { wasmPath: '包内路径/CEncodeDecodeTest.wasm' })。
内存位宽由宿主实际支持范围决定，实例化失败会报告失败。
*/

/** Linux 与 macOS 的 CMake 入口
从本工程目录执行 cmake -S . -B build -DSDK_ROOT=<当前系统及架构的SDK> -DBIT=<64或arm64> -DCMAKE_BUILD_TYPE=Release，
执行 cmake --build build，再在 build 内运行 ctest --output-on-failure。交叉编译时需另传目标工具链；
只有与目标 ABI 匹配的系统才能直接运行产物。
*/

/** 原生移动端测试
Android 用对应 ABI 的原生 SDK 与 NDK 工具链构建，adb 连接设备后运行 ctest --output-on-failure。
CTest 在设备的独占私有临时目录执行并读取 C++ 退出码。可用 PLATFORM_TEST_DEVICE 指定序列号；
SDK 外的共享运行库（例如 libc++_shared.so）用 PLATFORM_TEST_RUNTIME_LIBRARIES 指定绝对文件路径。
iOS 用 Xcode 生成器、CMAKE_SYSTEM_NAME=iOS 和 iphoneos 或 iphonesimulator 的同架构 SDK 构建。
模拟器启动后运行 ctest -C Release --output-on-failure。真机配置签名开发团队和 PLATFORM_TEST_DEVICE
为设备 UDID 后运行同一命令（需要 devicectl），也可以在 Xcode 运行并读取屏幕的 PASS/FAIL 和退出码。
每次运行拥有自己的可写临时目录，结束后清理本次文件。
从本测试工程的 CMakeLists.txt 所在目录配置，以下命令以 arm64 为例：
cmake -S . -B build-android -DCMAKE_TOOLCHAIN_FILE=<NDK>/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DBIT=arm64 -DSDK_ROOT=<Android目标SDK> -DCMAKE_BUILD_TYPE=Release
cmake -S . -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_OSX_ARCHITECTURES=arm64 -DBIT=arm64 -DSDK_ROOT=<iOS模拟器目标SDK> -DCMAKE_BUILD_TYPE=Release
使用真机时将 iphonesimulator 改为 iphoneos，传入该设备 SDK 与 -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=<团队标识>。
选定构建目录执行 cmake --build . --config Release，再执行 ctest -C Release --output-on-failure。
*/

/** 性能测试与运行边界
2026-11-22，Intel Core i9-14900HX，Release。32 字节 MD5 返回 32 个十六进制字符。SHA-256、HMAC-SHA256 处理 4096 字节，HMAC 密钥 32 字节。各次读取实际摘要组成校验和。逻辑处理器 8，一次完整进程预热后，各目标顺序采五个独立进程，每项自动校准批量后测九轮。结果为各进程中位数的中位数、[Q1, Q3] 及全距。
Windows x64 / MD5（ns/调用）　235.002 [233.929, 245.816]，全距 218.185–268.828
Windows x64 / SHA-256（µs/调用）　16.614 [16.506, 16.990]，全距 16.367–17.933
Windows x64 / HMAC-SHA256（µs/调用）　17.408 [17.294, 17.419]，全距 17.008–18.077
Node Wasm32 / MD5（ns/调用）　210.080 [206.781, 211.270]，全距 204.376–251.942
Node Wasm32 / SHA-256（µs/调用）　16.150 [15.996, 16.885]，全距 15.560–17.384
Node Wasm32 / HMAC-SHA256（µs/调用）　17.507 [16.902, 17.871]，全距 16.488–18.896
Node Wasm64 / MD5（ns/调用）　313.966 [302.869, 316.230]，全距 284.024–320.370
Node Wasm64 / SHA-256（µs/调用）　18.069 [17.812, 19.604]，全距 17.042–20.193
Node Wasm64 / HMAC-SHA256（µs/调用）　19.431 [18.382, 19.929]，全距 18.013–25.978
TLS 握手、密文队列及密钥生成缺少本版时延、吞吐和内存峰值数据。
传 --benchmark 可输出原始九轮及校验和。配置 -DLIBRARY_TEST_BENCHMARKS=ON 后使用 ctest -V -L performance。
双位宽包 Node 使用 node run-node.js 32 --benchmark 或 64，浏览器或微信页面加 benchmark=1。
已运行 Windows Win32/x64 Debug/Release、Node/Chrome 的 32/64 位和微信开发者工具 32 位。
微信开发者工具基础库 3.17.4 强制 Wasm64 在模块实例化时报告 CompileError。Linux、macOS、Android、iOS 及微信真机待设备实测。
*/