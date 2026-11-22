#pragma once
#include <stdint.h>

/** CEncodeDecode 功能测试。十组用例均从公开 SDK 入口调用，同一组可独立执行
1　SHA-256 对照空串、abc、百万个 a 的摘要。以 63、64、65 字节分片更新；容量不足可重试，
   完成后拒绝更新。两个独立对象分别保持自己的状态；白盒将其中一个的计数置于上限附近，溢出错误应持续。
2　HMAC 对照 RFC 4231 的短密钥、长密钥和空输入向量。PBKDF2 以 1、2、4096 轮对照固定结果；
   非法指针、长度或轮数被拒绝，输出不变。输入输出允许的重叠应成功，wipe 仅清除指定区间。
3　Parts 在不同位置切分 abc 和 Hi There，结果应与固定摘要一致；零片段、16片段和16 MiB正好可用，
   16 MiB 的连续和分片输入分别对照独立计算的 SHA-256、HMAC 固定值。17片段、总量超限及地址回绕应失败。拒绝请求时保持输出，允许的密钥或正文重叠应成功。
4　在内存中创建 CA、P-256 私钥和 CSR，签发含附加 DNS 名的叶证书。主名、SAN、配对密钥和签名应通过；
   DNS 校验覆盖 63 字节标签、253 字节全名、越界、空标签、首尾连字符和内嵌 NUL；错误信任根、错误域名、错误密钥、以叶证书及配对私钥签发、修改过的摘要及非法日期应失败。私钥或 CSR 任一侧容量不足时，两个实际长度都应返回，两个完整缓冲都应保持原值。错误输入和别名按接口契约
   保留或清零长度。独立解析器检查单叶、曲线、KU、双用途 EKU 扩展的存在和值，并核验 CSR 签名。分别切换客户端、服务端用途时，扩展须只包含选中的 OID；两项都关闭时须省略扩展。验签的三个可选参数有六种不完整组合，每种和超长签名均须返回 ARGUMENT；与证书不匹配的摘要或私钥须返回 STATE。验信任根时，缺少结束 NUL、内嵌 NUL 或长度非法应返回 ARGUMENT。启用 OpenSSL 时分别从 PEM 和 DER 临时文件读取该叶证书，提取的公钥必须与独立解析器输出的 65 字节未压缩曲线点逐字节一致。
5　MD5、AES-128 零块、SM4 单轮及百万轮对照固定值。以两个不同的连续分组核对 SM4 加密和解密的固定答案，错误密钥长度、空输入及残缺分组在两个方向均须返回空。AES 用固定密文核对短密钥补零、超长密钥只取前 16 字节和小写 HEX 解密；另检查跨块往返、错误长度及合法长度内的非 HEX 字符。
   RSA 三个入口均须调用显式熵源，190 字节明文往返成功，191 字节被拒绝，密文为 256 字节。
   回调部分写入后返回非零或抛异常时，随机缓冲被擦除且哨兵不变，生成失败清空预填密钥，加解密失败返回空串。
   原生端验证系统熵源，Wasm 验证缺少回调时失败。启用 USE_OPENSSL 且后端支持 SM2 时检查两种前缀格式、分段长度、往返和 C3 校验；
   默认构建及显式禁用 SM2 的 OpenSSL 配置检查失败时清空输出。库与测试须使用相同的 USE_OPENSSL 配置。空路径与空证书句柄应安全返回。
6　用上游公开测试证书和 localhost，以最多17字节的片段驱动 TLS 握手。两种 appendExact 和 append 提交带 NUL
   明文，对端读取的字节及顺序应一致。feed 全收返回 OK，部分接纳返回 WANT_IO；正容量 read 仅在 OK 时交付明文，等待时返回 WANT_IO 且计数为零。双段追加的第二段参数非法时返回 ARGUMENT，第一段也不得入队。检查待发与排空状态。收到 close_notify 后 read 和 status 均返回 CLOSED，
   将 close_notify 的最后一个分片与传输结束一并交付时仍返回 CLOSED；缺少通知时返回 TRUNCATED。
7　空标签、超过 63 字节的标签、首尾连字符须在配置阶段返回 ARGUMENT。合法单标签及带末尾根点的绝对名称可创建客户端。同一受信证书配上错误域名，客户端进入 PROTOCOL。最后一批握手密文与 EOF 同时交付时应返回相同的 PROTOCOL 和非零 detail；对端握手数据完全缺失时应返回 TRUNCATED。
8　未完成的握手分别检查默认 10000 毫秒及显式 100、60000 毫秒，截止前一毫秒仍在握手，到点进入 TIMEOUT；
   时钟回退进入持续的 ARGUMENT 状态。非法设置 99 或 60001 应保留原时限。
9　输入 32768（默认）或 41000、输出 32768（默认）或 7 字节时检查真实背压；非法配置保留两个上限。
   旧记录因密文背压被冻结时，write、append 的接纳数均为零，appendExact 返回 WANT_IO。排空后重试，新旧明文各交付一次。双向设为 -1，连续排队传送 65549 字节，在消费前核对借用前缀，最终核对完整明文。
10　直接核对 entropyBytes。零长度应成功且不调用熵源；非零长度的空输出和 8193 字节请求应失败且不调用熵源。8192 字节成功时恰好调用一次，完整结果及前后哨兵须匹配；缺少回调时只清零有效请求范围。

无参或 --all 执行全部，--cases 1,6-8 选择编号，--instant 选择除 ID 5 外的用例，--stress 选择 ID 5。
每组开始时按稳定编号重置测试熵源，单独选择与全量运行使用相同的测试随机序列。每组成功返回 0，失败时输出组号和步骤，继续运行其余选中组。全通过时进程返回 0，有失败返回 1，选择参数无效返回 2。
Windows 用 version_debug.sh 准备 Win32 Debug DLL，version_release.sh 准备 x64 Release 静态库，
再用同配置的 VS2013 构建 CEncodeDecodeTest.sln 并运行。另选位宽时为两个依赖准备匹配的 SDK。
*/

/** CEncodeDecodeTest 的稳定功能编号
*/
enum CEncodeDecodeTestCase
{
    // 散列及增量对象生命周期
    kCryptoHash = 1,
    // 消息认证、派生及擦除
    kCryptoDerivation = 2,
    // 分片输入与别名
    kCryptoParts = 3,
    // 证书签发与验证
    kCryptoCertificate = 4,
    // 文本、分组和公钥算法及百万轮向量
    kCryptoLegacy = 5,
    // 受信连接的数据和关闭语义
    kCryptoTlsTrusted = 6,
    // 错误域名拒绝
    kCryptoTlsWrongHost = 7,
    // 握手时限和单调时钟
    kCryptoTlsDeadline = 8,
    // 密文队列容量及借用期
    kCryptoTlsQueueLimits = 9,
    // 熵源长度上限、回调调用次数及输出边界
    kCryptoEntropy = 10
};

/** 生成可复现的测试字节，仅用于本工程串行测试
@param [in] context 保留参数，可空
@param [out] output 可写的 size 字节缓冲
@param [in] size 请求字节数
@return 完整写入后返回 0
*/
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
从本工程目录执行 cmake -S . -B build -DSDK_ROOT=<当前系统及架构的SDK> -DBIT=<目标位宽或架构> -DCMAKE_BUILD_TYPE=Release，
执行 cmake --build build，再在 build 内运行 ctest --output-on-failure。交叉编译时需另传目标工具链。
Linux ARMv7-A 硬浮点使用 arm32，AArch64 使用 arm64。先用同一目标工具链与配置准备
PlatformTest 和 CEncodeDecode 到同一 SDK，再构建本测试工程。交叉编译后可将 CEncodeDecodeTest
复制到对应 Linux 系统执行 ./CEncodeDecodeTest --all；共享链接还需让加载器能找到同一 SDK 的 libCEncodeDecode.so。
预期编号 1 至 10 分别返回 0，末行输出 CEncodeDecodeTest PASS，进程退出码为 0。
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
本轮只执行功能测试。以下三项为已有 Release 样本，本次未重新采样，未验证本轮产物的运行间波动。
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
同为 4096 字节输入，表中三组 HMAC 的中位数都略高于对应 SHA-256；32 字节 MD5 的输入和返回格式不同，不能据此比较算法的等长吞吐。
已有五进程样本中，Wasm64 的 HMAC 最慢一组为 25.978 µs/调用，最快为 18.013 µs/调用，相差约 44%。跨次比较以同宿主、相同版本和配置下的进程中位数及四分位区间为准，同时保留全距观察波动。
分片与增量散列、PBKDF2 各轮数、AES/SM2/SM4、RSA、P-256、TLS 握手、记录处理和队列背压均缺少本轮性能测试数据；
熵源耗时取决于宿主，wipe 成本随擦除长度变化，均缺少独立测量。Linux、macOS、Android、iOS 缺少对应设备数据。
OpenSSL 的 getCert 包括文件 I/O 和解析，getPublicKey 包括曲线点导出及字符串分配，这两个入口缺少独立性能数据。releaseCert 只负责释放句柄，本次不单独计时。
队列配置的设置与读取、status、detail 是常数成本的字段访问，本次不单列性能数据。
单独传 --benchmark 时先核验固定输入的完整摘要，再校准、采样并输出原始九轮及校验和。功能组与基准独立运行，可避免 RSA 密钥生成和 SM4 百万轮影响采样起点。配置 -DLIBRARY_TEST_BENCHMARKS=ON 后使用 ctest -V -L performance。
双位宽包 Node 使用 node run-node.js 32 --benchmark 或 64，浏览器或微信页面加 benchmark=1。
2026-11-22 功能验证：Windows VS2013 的 Win32/x64 Debug、Release，以及 Emscripten 3.1.74
的 Node Wasm32、Wasm64 Release，默认 mbedTLS 配置的 ID 1 至 10 均通过。Windows x64 Release 另以 USE_OPENSSL 运行全部十组：OpenSSL 3.6.5 支持 SM2，验证两种前缀和 C3 拒绝；OpenSSL 3.5.9 的 OPENSSL_NO_SM2 构建验证不可用返回。两者均通过 PEM、DER 文件读取和公钥核对。
CEncodeDecode.cpp 注释示例在 Windows x64 Release、Node Wasm32 和 Wasm64 单独编译运行，退出码均为 0。Linux x64、macOS、Android、iOS 按源码及构建配置核对。
2026-11-22 Linux ARM 功能验证使用 QEMU 10.2.0 和 Debian 6.1 系列内核。ARM32 以 Arm GNU 13.2.1 构建 CEncodeDecode 与测试程序，
使用 Debian armhf 的 glibc、libstdc++，模拟 Cortex-A15，关闭 NEON 与 VFP-D32，保留 VFPv3-D16。ARM64 使用 Zig 0.13.0 的 Clang 18.1.6、
libc++ 和 Debian arm64 的 glibc，模拟 Cortex-A53。两端各用独立 SDK 构建 Release 静态库和共享库，执行 CEncodeDecodeTest --all。
四种组合的 ID 1 至 10 分别返回 0，均输出 PASS。两端另从 CEncodeDecode.cpp 注释提取并静态链接 main 示例，退出码均为 0。
本次未在 ARM 真机运行，未采集 ARM 性能数据。
Windows x64 Release 另核对 --instant、--stress、单组 4、单组 10 和 --cases 1,6-8 的选择结果，倒序范围 8-6 和未知编号 11 均返回 2。
本轮未运行浏览器、微信或移动设备。
*/