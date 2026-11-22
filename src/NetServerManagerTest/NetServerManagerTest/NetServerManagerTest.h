#pragma once

/** NetServerManagerTest 接口测试
无参数或 --all 执行全部用例；--cases 1,2 指定编号，--instant 和 --stress 按分类执行。
1 瞬时。通过正式可执行文件核对 --help 的退出码、程序名、协议和专属参数。
2 瞬时。旧模式拒绝；缺少配对私钥及越界端口返回明确的配置错误。
3 压力。用回环监听、临时端口和独占身份目录启动，验证 READY 在 STOPPED 之前、身份文件已生成且退出码为0，清理文件。
在原生桌面运行，将同配置的 NetServerManager1.1 可执行文件放在测试程序同目录，保证系统没有正在运行的同名网关实例。XUEYELINGSHUANG 指定仓库时，临时目录位于 src/temp；单独部署时请在专属临时工作目录运行。每次调用通过 PlatformTest 限时 30 秒，本机需有 cmake 或设置 PLATFORM_TEST_CMAKE 为其路径，完整批次建议限时 60 秒。该工程观察进程行为，700 ms 为运行期限，本次不测接口耗时。
2026-11-22 功能实测：ARMv7 hard-float 和 AArch64 的 Release 可执行文件均在 QEMU 的 Linux 6.1 内核中逐项通过。Windows x64 的帮助、参数拒绝、回环启停也均通过。
*/