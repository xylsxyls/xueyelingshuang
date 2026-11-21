#pragma once

/** Windows 服务端白盒用例通过 SDK 头访问请求处理上下文。版本脚本为该用例准备 HttpService 静态库，并为客户端和测试选择器准备 HttpRequest、PlatformTest 动态库。Wasm 客户端链接对应位宽的静态库。
*/

/** 客户端在提交阶段拒绝缺少主机、空括号主机、畸形端口和越界端口，GET 和 POST 失败后仍可设定首次请求并发上限。服务端 OPTIONS 的 Allow 和跨源方法列表均须包含已支持的 HEAD；206 和 304 分别使用 Partial Content 和 Not Modified，未登记的 599 使用服务器错误类别。 */

/** 服务端用例 1 用原生套接字读取至 EOF，HEAD /large 只收报头且 Content-Length=200000；204、304 不发送长度和正文，205 声明长度零并且无正文。该用例还验证注册包含 NUL 的路由失败，按不同大小写重复设置同一响应头后仅保留最新值。 */

#include <stdint.h>

/** 本机服务端的稳定场景编号。客户端由下文各模式选择实际源站及信任场景。 */
enum HttpWorkServiceCase { kServiceFunctional = 1, kServicePressure = 2 };

/** 客户端源站场景的稳定编号。单请求场景为瞬时，最后一项覆盖共同在途的订阅。 */
enum HttpWorkClientCase
{
    kClientBinaryGet = 3, kClientRange, kClientConditional, kClientRedirect, kClientMissing,
    kClientBinaryPost, kClientInvalidRange, kClientSizeLimit, kClientEmpty, kClientCoalescing
};

/** 测量由阻塞 socket 发起的新连接 GET，核对状态、正文及对端关闭
@param [in] port 已启动的 HttpService 回环端口
*/
void RunHttpServiceSocketBenchmark(uint16_t port);

/** 检查无正文响应的线上字节
@param [in] port 回环服务端口
@param [in] request 完整 HTTP/1.0 请求
@param [in] status 预期状态码
@param [in] length 预期 Content-Length，-1 要求省略此字段
@return 状态、长度字段以及实际的零字节正文均符合预期时返回 true
*/
bool CheckHttpServiceBodyless(uint16_t port, const char* request, int32_t status, int32_t length);

/** 通过写入两条实际回环连接验证单工作线程、长连接和请求超时
@param [in] port 当前可在回环上独占的端口
@return 三项网络行为都符合预期时返回 true
*/
bool CheckHttpServiceOptions(uint16_t port);

/** HttpWorkTest覆盖HttpRequest和HttpService。Windows/Linux直接运行测试本机回环服务。
服务端场景 1 检查默认值、路由、HTTP 字段、CORS、正文上限、实际保活与关闭。场景 2 检查多线程和多进程。无参或 --all 执行全部，--instant、--stress 或 --cases 1,2 选择对应范围。
跨源媒体验证注册路由的 OPTIONS 预检和独立 GET 响应，要求两者给出配置的允许源、预检允许 Range、If-Range 和 If-None-Match、实际响应暴露 Content-Range 和 ETag。关闭 CORS 时预检须省略允许字段。本次 Chrome 跨源读取实际 Windows 服务，得到状态 206、bytes 3-5/6、实体标签 "sample" 和正文 def。
场景 1 还从已保存设置启动两种兼容 run 入口，核对真实 CORS 头及超默认长度的正文；同步阻塞旧回调、替换路由并发出第二个请求，检查两个请求分别使用的处理器及旧引用释放。
客户端各模式在发起请求前逐字节检查 URL 中所有 ASCII 控制字符、空格和反斜线，均应立即拒绝。
客户端的 --limits 以 setTimeoutMs 配置实际等待时限，原生 --reuse 对 setMaxRedirects 的 0、1、-1 和非法值检查真实重定向行为及原值保持。
独立模式 --capacity-default 和 --capacity-unlimited 分别提交 33 个不同的在途请求，核对默认接纳 32 个和取消上限后接纳全部 33 个，并等待所有已接纳请求超时返回。
原生 --large-ca-default 与 --large-ca-unlimited 使用同一份 65537 字节的有效私有 CA 输入，核对服务器的两次实际连接号，验证默认跳过缓存及显式 -1 后复用。
服务端使用真实分块 POST 核对多块合并后的内容，以及多块累计超过正文上限时的 413 状态。
HttpRequest五个原生平台与Wasm使用相同客户端用例。--client origin [CA.pem] 后可加 --all、--instant、--stress 或 --cases 3,5-8。
3–11 依次检查二进制 GET、Range、条件 GET、重定向、404、二进制 POST、无效范围、响应上限、空正文；12 检查并发订阅合并。
完整运行覆盖GET/POST二进制、
206 Range和If-Range、ETag/304、重定向、404、416、响应大小限制和204空正文；
--reject origin [CA.pem]确认TLS错误且应用未收到正文。--limits origin 使用独立阻塞源站，验证并发上限、到期回调和满额时的请求合并。
原生 --uncached-ca 使用两份独立私有 CA，按真实连接号确认禁用私有 CA 缓存立即生效。原生 --reuse 还验证 0 和 1 次重定向额度。Wasm使用宿主信任，私有CA由宿主配置。
HttpWorkFixture [--openssl 可执行文件] 测试程序，会创建并清理仅监听本机的临时源站。
源站可用 HttpWorkFixture --serve --port 58123 独立启动，只监听127.0.0.1。
原生/Node支持 --public https://域名/ 发送一次无凭据GET并按默认CA验证。并发用例在完全相同的两个请求已提交后释放源站，
验证仅一次请求抵达源站且两位调用者都收到结果。原生所有者在回调外调用shutdown等待任务结束，之后提交会被拒绝。
浏览器和微信可用CMake的HTTPWORK_WASM_HOST=ON生成模块，调用HttpWorkHost.js中的runHttpWorkTest；
在微信中另传包内wasm路径并接入wx.request，正式部署需配置可信域名。浏览器跨域媒体须暴露Content-Range与ETag。
Node使用 --node-ca 并传入 node 和 js 两个参数；浏览器和微信以异步httpWorkTestResult获取最终结果。
双位宽包使用 LibraryTestWidth.js 的 runPair(run32, run64, mode, probePath)，run32/run64 各自加载相应工厂并调用 runHttpWorkTest。
mode 为 auto 时探测 64 位表和内存，返回实际运行的 bits 和 result。指定 32 或 64 会直接验证该位宽；加载和业务用例失败向调用者返回错误。
微信把 LibraryTestMemory64.wasm 放入包根或传入对应包内路径。Node 的模块化包同样可以用 CTest 直接运行 HTTP 和证书回归。
可用 仓库双位宽打包工具指定 HttpWorkTest --wasm32 build32 --wasm64 build64 --output pair --origin http://127.0.0.1:58123 打包。

Android/iOS 用目标工具链配置 CMake 并启用 BUILD_TESTING。SDK_ROOT 指向同架构的 HttpRequest、CEncodeDecode 及其第三方依赖，
目标 curl 8.23.0 必须支持该平台的系统证书信任。Apple 和 Android 实际链接目标 SDK/lib/libcurl.a；配置时传 -DCURL_DIR=<同次静态 curl 安装的 CMake 包目录>，并用 CMAKE_PREFIX_PATH 指定目标 TLS 依赖。省略 CURL_DIR 时只读取当前 SDK/lib/cmake/CURL。配置核对静态导出归档与 SDK 归档的 SHA-256，由静态导出提供 TLS、框架和链接选项。缺少配置包、只提供共享目标或归档不匹配时配置失败。BIT 可显式指定 32、64、arm32、arm64；arm64 使用 libmbedtls.a。
构建 HttpWorkTest 后运行 ctest -R HttpRequestLoopback --output-on-failure。PLATFORM_TEST_DEVICE 可指定 Android 序列号或模拟器 UDID；
Android 需要 adb，按需通过 PLATFORM_TEST_RUNTIME_LIBRARIES 传入 libc++_shared.so 等目标库。CTest 在宿主创建临时回环源站，
Android 用 adb reverse 连接并清理本次建立的映射，iOS 模拟器直接访问宿主回环地址。
iOS 真机需签名，指定 PLATFORM_TEST_DEVICE 和 HTTPWORK_TEST_ORIGIN（设备可达的测试源站地址）。局域网源站需要在设备上允许本地网络访问。
该源站需提供与 HttpWorkFixture 相同的二进制正文、Range、重定向、ETag、held 和 release 端点。原生客户端性能用例还需要 /keep-media，返回相同 5 字节正文并允许复用当前连接。Android 与模拟器也可指定该源站。
移动设备用例验证 --client 流程。桌面和 Node 的临时 CA、主机名错误与不受信证书检查由 HttpWorkFixture 的 --openssl 模式执行。
*/

/** 性能测试与运行边界
2026-11-22，Windows x64 VS2013 Release，i9-14900HX，进程绑定逻辑处理器 8、10。HttpService 使用 16 个工作线程，关闭保持连接。每次串行建立一个回环 TCP 连接，以 HTTP/1.0 请求 /test?name=bench，校验状态 200、正文 GET:bench（9 字节）并关闭。先运行一个完整预热进程，随后五个独立进程各自校准批量并执行九轮。以下为各进程中位数的中位数、[Q1, Q3] 及全距，单位 µs/请求。
原生套接字发送、收至 EOF 并关闭　364.991 [325.188, 399.305]，全距 313.253–442.420。
CivetWeb mg_download 和 mg_close_connection 测试客户端　426.489 [419.464, 463.669]，全距 367.677–555.947。该数包含客户端上下文分配及双端关闭等待，进程间波动较大，仅供诊断。服务端路由自身耗时、并发吞吐和峰值内存缺少实测数据。
2026-11-22，i9-14900HX，Windows x64 VS2013 Release 及 Node 双位宽 Release 访问独立进程的回环源站。逻辑处理器 8、10，每次仅有一个在途请求。计时从提交 httpGet 至完成回调入口，每次校验 HTTP 200 和 5 字节二进制正文。各配置预热一个完整进程并采五个独立进程，每个进程每种场景另预热 10 次并测 90 次。以下为各进程中位数的中位数、[Q1, Q3] 及全距，单位 µs/请求。
Windows x64 / 关闭连接的 /video/sample　481.500 [471.850, 497.800]，全距 456.850–538.450。
Windows x64 / 允许复用的 /keep-media　112.250 [107.200, 113.150]，全距 103.000–116.500。
Node Wasm32 / 关闭连接的 /video/sample　1063.000 [942.700, 1115.350]，全距 933.000–1278.450。
Node Wasm64 / 关闭连接的 /video/sample　1110.650 [1108.500, 1159.800]，全距 1040.050–1183.400。
原生复用场景对同一源站连续发送 GET；Wasm 的连接池由宿主管理。本次缺少外网传输、TLS 握手及多请求并发的性能数据。
CMake 设置 -DLIBRARY_TEST_BENCHMARKS=ON 后运行 ctest -V -L performance。服务端使用 HttpServiceBenchmark，客户端使用 HttpRequestBenchmark。
运行 HttpWorkFixture --benchmark <HttpWorkTest可执行文件> 可启动独占源站测量客户端；HttpWorkTest --benchmark 单独测量服务端。
Node 双宽包可用 node run-node.js 32 --benchmark 或 64；浏览器和微信页面加 benchmark=1。直接调用 runHttpWorkTest 可传 { benchmark: true, print } 配置。
服务端用例用真实连接检查工作线程数、保持连接、请求超时、自定义及关闭 CORS，并检查启动前设置、运行中拒绝替换和超过默认 1 MiB 的正文。原生 --reuse 在同一进程从 POST 切到 GET，按源站端口及实际 accept 编号验证复用，检查二进制正文、Range、If-Range、If-None-Match、Content-Type、完成回调及换源后旧连接释放。私有 CA 切至系统 CA 和另一私有 CA 时，两个不受信 POST 均在抵达应用前被拒绝。Node 与 Chrome 双位宽、微信开发者工具基础库 3.17.4 的 32 位通过客户端用例。该微信宿主强制 Wasm64 加载时报 CompileError。
Linux、macOS、Android、iOS 及微信真机待设备实测。桌面或 Node 要执行证书用例，配置时提供本机 OpenSSL CLI 的绝对路径 -Dtest_openssl=<路径>。
*/

/** 服务端用例直接核对 HttpResponse::setStatus 的状态码与默认原因，并通过 HttpInterface::handle 核对回调设置的自定义状态、原因、头和值及正文。两种 run 路由批次均在有效项之后加入无效项，要求保持停止、保存错误、释放部分注册的所有权；成功重试后原服务用例继续可用。 */

/** Windows、Linux 的借用路由用例：派生服务将自身注册表作为 run 的输入重启。须保留一个注册项并经回环 POST 收到 200 和原处理器的正文。 */