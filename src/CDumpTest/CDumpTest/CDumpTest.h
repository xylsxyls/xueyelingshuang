#pragma once

/** CDumpTest 功能测试。用与目标位宽、配置一致的 CDump 和 PlatformTest SDK 构建
1 注册（瞬时）。Windows 和 glibc Linux（含 ARM）先用内嵌 NUL、过长路径和文件充当目录，
预期均失败且允许重试。默认目录首次注册成功，第二次指定目录返回 false。Linux 独立查询两个信号的处置。
macOS、Android、iOS、Wasm 没有当前库的原生转储后端，默认和指定目录均应返回 false。
2 转储（压力类，单个子进程一次）。原生后端在新进程中注册独占临时目录，目录参数无尾分隔符。
Windows 在有本地 SEH 保护的子进程交付异常；预期过滤器成功返回、子进程正常退出、恰有一个 .dmp 和一个 .log。
Windows 用例 2、3 在注册后切换到独立的空目录，再尝试将转储目录改为当前目录。预期重复注册返回 false，新目录始终为空。
核对 MDMP 签名、版本、异常流的线程 ID 与异常码，并将日志中的首帧地址与转储中该线程的指令指针逐值比较。日志应记录相同异常码和写盘成功标志。
Linux 送达一次 SIGABRT，预期恰有一份日志。核对信号、1～128 个连续编号的非零栈地址，以及首帧所属的可执行内存映射。在 20 秒内完成并清理临时目录。
3 工作线程（Windows 压力类，独立子进程一次）。在含空格及非 BMP 字符的工作目录中，以带尾分隔符的相对路径注册，然后切换工作目录。
工作线程交付受控异常。预期文件仍在注册时选定的位置，异常流记录真正的工作线程，文本与转储同名并记录同一异常。
无后端目标只登记 1。每次启动是一个新批次，可用 --cases 1、--instant、--stress 或 --all。
Windows 先运行 version_release.sh 32 或 64，再用同一 SDK 构建 CDumpTest.sln 的 Release Win32 或 x64，
执行 CDumpTest.exe --all。Debug 选择 version_debug.sh 和同位宽的 Debug 工程，运行 CDumpTestd.exe。
Linux、macOS 和移动端用内层 CMakeLists 配置对应 SDK_ROOT、BIT 和工具链，按 SDK 配置构建后运行 CTest。
Wasm 先用 version_release.sh wasm32 或 wasm64 准备 SDK，用 Emscripten 工具链配置内层 CMakeLists，
BIT 与 SDK 位宽一致，SDK_ROOT 指向该 SDK，CMAKE_CROSSCOMPILING_EMULATOR 指向支持该位宽的 Node，再构建并运行 CTest。
2026-11-23 功能实测。Windows VS2013 Release 的 Win32 静态库、x64 静态库、x64 DLL 和 Debug 的 x64 静态库均通过 1、2、3。
在独立的测试程序副本中，故意写往新工作目录时用例 2 报错，把文本首帧改为零时用例 3 报错。
Emscripten 3.1.74、Node 24.0.0 的 Release Wasm32 和 Memory64 各通过 1。两种 Wasm 选择 2 均返回参数错误 2。
CDump.cpp 的 main 示例在 Windows x64 静态库配置和两种 Wasm 位宽下均编译并运行通过。
Linux、Linux ARM、macOS、Android、iOS 仅核对源码及工程配置，本轮未在目标系统编译或执行。
性能：本次不做性能测试。注册每进程一次，无需吞吐基准；单次注册延迟和故障转储耗时均缺少性能测试数据。
子进程的 20 秒是挂起保护；CTest 的整例耗时含启动、文件检查与清理，不能用作接口耗时。
*/
enum CDumpTestId
{
    // 检查参数、重试和重复注册
    kRegisterOnce = 1,
    // 检查独立进程生成的诊断文件
    kDumpFiles = 2,
    // 检查工作线程身份及相对目录的生命周期
    kWorkerDump = 3
};