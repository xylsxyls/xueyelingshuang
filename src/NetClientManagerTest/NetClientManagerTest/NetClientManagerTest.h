#pragma once

/** NetClientManagerTest 接口测试
无参数或 --all 执行全部用例；--cases 1,2 指定编号，--instant 和 --stress 按分类执行。
1 瞬时。通过正式可执行文件核对 --help 的退出码、程序名、协议和专属参数。
2 瞬时。旧模式拒绝；非法端口和 v3 混用 CA 选项返回参数错误。
3 压力。通过任意空闲的本机网关租约，以回环地址、独占缓存和 700 ms 运行期限以受监督模式启动，按顺序观察本实例 READY、STOPPED 并清理。
在原生桌面运行，将同配置的 NetClientManager1.1 可执行文件放在测试程序同目录，保证系统没有正在运行的同名网关实例。XUEYELINGSHUANG 指定仓库时，临时目录位于 src/temp；单独部署时请在专属临时工作目录运行。每次调用通过 PlatformTest 限时 30 秒，本机需有 cmake 或设置 PLATFORM_TEST_CMAKE 为其路径，完整批次建议限时 60 秒。该工程观察进程行为，700 ms 为运行期限，本次不测接口耗时。
2026-11-22 功能实测：ARMv7 hard-float 和 AArch64 的 Release 可执行文件均在 QEMU 的 Linux 6.1 内核中逐项通过。Windows x64 的帮助、参数拒绝、回环启停也均通过。 Linux 另外验证了 Windows 登录自启动选项返回参数错误。
*/