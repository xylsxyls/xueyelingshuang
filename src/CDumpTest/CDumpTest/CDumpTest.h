#pragma once

/** CDump 进程级转储测试
1 瞬时。Windows/Linux 首次注册成功，重复注册失败。Linux 先传超长路径，失败后仍可成功注册，随后检查 SIGABRT/SIGSEGV 处理器已安装。
2 Windows 压力。重启本测试的独立无窗口子进程，在独占目录安装处理器并触发测试专用 SEH 异常。父进程等待至多 20 秒，检查非正常退出、.dmp 的 MDMP 标记和非空 .log，清理本次目录和文件。
2 Linux 压力。独立子进程安装处理器并发出一次 SIGABRT。父进程在 20 秒内等到子进程正常返回，从本次目录的 core-file 日志核对信号号、回溯帧数和首帧，再清理目录。依赖 glibc 堆栈回溯和 /proc/self/exe，适用于 Linux x86 与 ARM。生产库在 Wasm 无本地进程转储后端。
使用 --cases 1、--instant 或 --stress 选择用例。真实转储涉及操作系统调试组件和磁盘，进程内注册只能执行一次，本次不单独计时。
2026-11-22 功能实测。Windows x64 和 QEMU Linux ARMv7 hard-float、AArch64 各通过用例 1、2，含各平台真实子进程转储。
*/