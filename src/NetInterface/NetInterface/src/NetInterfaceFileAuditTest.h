#pragma once
#include "NetInterface.h"

// 仅测试程序引用；生产库不编译实现，不创建线程或文件。
class NetInterfaceFileAuditTest
{
public:
    /** 运行编码、状态和独立文件内容模型；单执行者调用
    @return 全部通过为true，否则报告首个失败位置
    */
    static bool run();

    /** 生成固定身份及有界元数据，摘要由用例独立提供
    @param [in] size 整文件长度
    @param [in] chunk 协商块长度
    @param [in] window 未确认窗口
    @param [in] resume 已持久化前缀
    @return 测试元数据
    */
    static NetInterfaceFileInfo info(uint64_t size, uint32_t chunk, uint32_t window, uint64_t resume);

    /** 借用用例字节构成完整帧，不编码或修改内容
    @param [in] info 固定流身份
    @param [in] kind 帧类别
    @param [in] bytes 借用正文
    @param [in] size 正文长度
    @return 固定产品7的帧
    */
    static NetInterfaceFrame frame(const NetInterfaceFileInfo& info, uint16_t kind, const uint8_t* bytes, uint32_t size);

    /** 核对独立字节预期、截断、别名和长度边界
    @return 是否通过
    */
    static bool codec();

    /** 用独立持久化字节模型核对累计ACK和最终提交
    @param [in] total 整文件长度
    @param [in] chunk 协商块长度
    @param [in] window 未确认窗口
    @param [in] batch 合并落盘块数
    @param [in] resume 已有持久前缀
    @return 是否通过
    */
    static bool transfer(uint32_t total, uint32_t chunk, uint32_t window, uint32_t batch, uint32_t resume);

    /** 核对冲突END、未持久化重复块和单调超时
    @return 是否通过
    */
    static bool boundaries();

    /** 核对C包装的空参数、别名、失败输出与失败ACK
    @return 是否通过
    */
    static bool cApi();

#if defined(_WIN32) && defined(_DEBUG)
    /** 独立main中检查构造及首批分配失败、热路径分配和擦除
    @return 是否通过
    */
    static bool memory();

    /** 隔离进程的CRT故障注入与释放观察，不分配内存
    @param [in] operation CRT操作
    @param [in] bytes 释放/调整的原区域
    @param [in] size 请求长度
    @param [in] block CRT块类型
    @param [in] request CRT请求号
    @param [in] file CRT源码位置
    @param [in] line CRT源码行号
    @return 1继续，0拒绝当前分配
    */
    static int __cdecl memoryHook(int operation, void* bytes, size_t size, int block, long request, const unsigned char* file, int line);

    // 剩余允许分配次数；负数表示仅观察。
    static int32_t m_rejectAfter;
    // 钩子内记录分配和未擦除测试正文的释放次数。
    static uint32_t m_allocations;
    // 释放钩子发现仍包含测试明文标记的内存块次数，期望为0
    static uint32_t m_exposed;
#endif

public:
    // 仅本次测试执行者累加，不用于生产状态。
    static uint32_t m_checks;
};