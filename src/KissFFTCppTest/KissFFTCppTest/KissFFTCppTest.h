#pragma once

/** 功能用例与独立预期
无参数运行全部功能组；--list 列出可选ID，--case=<ID>只运行该组及其素材准备。未知ID或选项退出2。
invalid：未打开的对象拒绝正逆变换；零、负数和超出分配上界的点数均返回false。
dft-2、dft-6、dft-16、dft-64、dft-120：分别对该偶数点数生成确定性时域样本，用直接DFT逐频点核对实部和虚部，误差小于1e-8；逆变换除以N后逐点误差小于1e-9。拒绝空指针、方向及缓冲长度不匹配；无效计划变更保留原计划。
dft-prime：用14、34、194点覆盖内部7、17、97的大素数因子分解，正逆向分别用同一独立DFT及原样本验证。
direction：在同一实例上切换、复用逆向计划。二点频谱[0,2]恢复[2,-2]，该实例随后拒绝调用正变换。
*/

/** KissFFTCpp测试工程。
平台范围 Windows、Linux、macOS、Android、iOS、Wasm。
运行对应测试程序，返回0表示通过，非0表示失败。
Wasm 的两个位宽分别以 BIT=wasm32 和 BIT=wasm64 构建。启用 LIBRARY_TEST_HOST 后用 ctest 在 Node 执行。
浏览器通过 HTTP 打开构建目录的 LibraryTestHost.html?test=KissFFTCppTest。
微信将 KissFFTCppTest.js、KissFFTCppTest.wasm 和 LibraryTestHost.js 放入分包，调用
await require('./LibraryTestHost')(require('./KissFFTCppTest'), { wasmPath: '包内路径/KissFFTCppTest.wasm' })。
运行 wasm64 的宿主必须支持 Memory64。
*/

/** 性能用例
运行本工程时增加 --benchmark 参数，使用公开接口执行独立批次并验证结果。计量单位为纳秒/操作。
先自动预热并校准批次大小，再输出九轮数据、中位数、第一及第三四分位数。
*/

/** 当前实现已有性能数据（采集于2026-11-22）
输入为 1,024 点双精度单位脉冲。第一项在同一对象按输入长度重复 open 并作正变换，第二项复用两个已打开的正逆向计划完成一次往返。
Windows x64 Release，Intel Core i9-14900HX，VS2013 v120 /MT。两组各先预热一个进程，再顺序运行七个独立进程。每个进程自行预热、校准批量并采九批，用各批操作数归一化。单位为微秒/次，两列是在同一逻辑 CPU 6 上依次执行的独立批次，各列给出七个进程中位数的中位数 [最小值, 最大值]。
  open + forward    4.296 [4.227, 4.335]    4.310 [4.254, 4.353]
  forward + inverse    8.790 [8.692, 8.853]    8.815 [8.734, 8.939]
跨进程范围反映宿主调度与负载对该场景的影响。复测应使用同一输入、构建配置、宿主及 CPU 亲和性。
Wasm Release 使用 Emscripten 3.1.74。Node 24.0.0、Chrome 157 和微信开发者工具基础库 3.17.4 分宿主顺序采样。每格为一次宿主运行内九批的中位数 [Q1, Q3]，单位微秒/次。各列依次为open + forward、forward + inverse。
  Node wasm32    5.425 [5.334, 5.720]    10.767 [10.623, 10.924]
  Node wasm64    6.665 [6.576, 6.818]    12.544 [12.311, 12.599]
  Chrome wasm32    7.080 [6.812, 7.446]    13.037 [12.646, 14.600]
  Chrome wasm64    9.131 [9.131, 9.473]    16.699 [16.504, 17.529]
  微信开发者工具 wasm32    36.328 [35.938, 37.305]    88.281 [71.875, 104.883]
固定逻辑处理器 6，另用五个独立 Node 进程分别预热、校准并采九批。以下各列仍按上述接口顺序，单位微秒/次，报告进程中位数的中位数 [最小值, 最大值]。
  Node wasm32    5.187 [5.149, 5.490]    10.120 [10.024, 11.496]
  Node wasm64    6.747 [6.513, 7.153]    12.710 [12.461, 13.799]
Chrome 和微信的数据缺少独立宿主进程复测。微信开发者工具强制 wasm64 时实例化失败。Linux、macOS、Android、iOS、微信手机及原生 Windows x86 缺少同口径的性能数据。
相同点数和方向沿用已有计划。长度或方向改变时仍重建。正逆变换对照组复用各自的独立计划。重建第三方归档时配置 src/KissFFTCpp/scripts/kissfft-native，MSVC 通过 KISSFFT_CRT 选择 mt、md 或 mdd，按架构分别构建和安装。
*/

/** 当前功能组的稳定数字编号。 */
enum KissFFTCppTestId
{
    // 非法点数与未打开状态
    KissFFTCppTestInvalid = 1,
    // 2点的参考离散傅里叶变换
    KissFFTCppTestDft2 = 2,
    // 6点的参考离散傅里叶变换
    KissFFTCppTestDft6 = 3,
    // 16点的参考离散傅里叶变换
    KissFFTCppTestDft16 = 4,
    // 64点的参考离散傅里叶变换
    KissFFTCppTestDft64 = 5,
    // 120点的参考离散傅里叶变换
    KissFFTCppTestDft120 = 6,
    // 状态和变换方向
    KissFFTCppTestDirection = 7,
    // 含较大素数因子的偶数长度
    KissFFTCppTestDftPrime = 8,
};

/** 指定测试可用 --cases 1 或 --cases 1,2-3。--instant、--stress 和 --all 按当前平台选组。
1 invalid 非法点数与未打开状态（瞬时）。
2 dft-2 2点的参考离散傅里叶变换（瞬时）。
3 dft-6 6点的参考离散傅里叶变换（瞬时）。
4 dft-16 16点的参考离散傅里叶变换（瞬时）。
5 dft-64 64点的参考离散傅里叶变换（瞬时）。
6 dft-120 120点的参考离散傅里叶变换（瞬时）。
7 direction 状态和变换方向（瞬时）。
8 dft-prime 含7、17、97素数因子的参考DFT（瞬时）。
没有适用压力项时 --stress 返回 2；沿用 --case=英文名 和 --list。
*/

/** 当前版本功能验证（2026-11-22）
完整功能套件通过1254条断言，包含三个新的大素数因子点数。
Windows x86、x64 的 Debug、Release 四种功能测试均通过。公共 x64 SDK 的 Debug、Release 消费者分别通过。
Node 和 Chrome 的 Wasm32、Wasm64 均通过，Chrome 自动选择 64 位。微信开发者工具（基础库 3.17.4）自动选 32 位，强制 32 位通过；强制 64 位因宿主不支持 Memory64 在实例化时失败。微信手机未实测。
Linux x64 Release 已交叉编译、链接，未在 Linux 运行。macOS、Android、iOS 仅检查平台分支和构建源码。主类示例在 Windows x64 Release 与 Node 两个位宽运行成功。
点数 2、6、14、16、34、64、120 和 194 的频谱分别与独立离散傅里叶变换参考值比较，包含反变换的缩放约定。
数字选择、重复或非法范围及适用的分组与兼容入口累计 87 次，退出码符合预期。当前构建的测量结果和波动范围见性能章节。
*/