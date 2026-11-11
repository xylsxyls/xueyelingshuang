#pragma once
#include "ProcessWorkMemory.h"
#include <atomic>
#include <stdint.h>
#include <stddef.h>

/** 一个固定小包槽；CAS取得后才允许读写正文，不移动正在使用的槽
*/
struct ProcessWorkPacketSlot
{
public:
    // 0空闲，1由一个申请者或已提交消息拥有
    std::atomic<uint32_t> m_used;
    // 包含调用方自己的计费前缀；整槽64字节，避免相邻owner位共享缓存行
    char m_data[60];

public:
    /** 初始化空闲槽并触及正文页
    */
    ProcessWorkPacketSlot();
};

/** 发布后地址及next不再改变的新增块；外部静止后才释放，不存在空闲链ABA
*/
struct ProcessWorkPacketBlock
{
public:
    // 扩容块拥有的独立槽数组
    ProcessWorkPacketSlot* m_slots;
    // 实际槽数
    size_t m_count;
    // 前一批已发布块，配置/销毁以外不释放
    ProcessWorkPacketBlock* m_next;

public:
    /** 建立槽数组，失败时m_slots为空
    @param [in] count 槽数，调用方已经检查乘法范围
    */
    explicit ProcessWorkPacketBlock(size_t count);

    /** 静止期释放数组，不递归释放next
    */
    ~ProcessWorkPacketBlock();
};

/** 本地小包的可增长备用池；多线程申请、归还，生命周期操作要求外部静止。
    只保存稳定槽，不使用带ABA风险的裸指针空闲栈；未命中由上层走原分配路径。
*/
class ProcessWorkPacketPool
{
public:
    /** 建立不分配内存的空池
    */
    ProcessWorkPacketPool();

    /** 外部已经归还全部小包后销毁池
    */
    ~ProcessWorkPacketPool();

    /** 静止期替换容量，0关闭；分配失败不改变原池
    @param [in] capacity 槽数，实际内存为每槽64字节
    @return true已完成，false无法分配
    */
    bool configure(uint32_t capacity);

    /** 静止设置初始预留、最大缓存槽数和外部增长机制
    @param [in] config 池及系统资源保护参数，保存策略强引用
    @return true已替换，false无效或准备失败
    */
    bool configure(const ProcessWorkMemoryConfig& config);

    /** 尝试借用一个小包槽，不等待某个持有者
    @param [in] bytes 包含上层前缀的字节数，至多60
    @return 稳定正文地址或nullptr；返回空不表示上层消息容量不足
    */
    char* acquire(size_t bytes);

    /** 归还本池拥有的借用；不读取其他分配器的正文
    @param [in] allocation 上层保存的原始分配地址
    @return true已归还本池，false不属于本池
    */
    bool release(char* allocation);

    /** 观察已经发布的实际小包槽数量
    @return 不包括尚未完成的增长预留
    */
    uint64_t capacity() const;

private:
    /** 禁止复制内存所有权
    @param [in] other 不可复制来源
    */
    ProcessWorkPacketPool(const ProcessWorkPacketPool& other);

    /** 禁止覆盖内存所有权
    @param [in] other 不可复制来源
    @return 不可调用
    */
    ProcessWorkPacketPool& operator=(const ProcessWorkPacketPool& other);

private:
    /** 检查一段稳定数组并独占取得空闲槽，不等待被占用槽
    @param [in,out] slots 稳定槽数组
    @param [in] count 数组长度
    @return 正文地址或nullptr
    */
    char* take(ProcessWorkPacketSlot* slots, size_t count);

    /** 为本次申请增长一批槽，成功后直接保留其中一个给申请者
    @return 新槽正文地址或nullptr，失败不计为成功增长
    */
    char* grow();

    /** 对数组内的精确正文起点执行归还
    @param [in,out] slots 稳定槽数组
    @param [in] count 数组长度
    @param [in] allocation 待归还的原始地址
    @return true属于该数组且已归还
    */
    bool returnSlot(ProcessWorkPacketSlot* slots, size_t count, char* allocation);

    /** 外部静止且全部借用已收回后销毁全部存储
    */
    void clear();

private:
    // 仅静止配置时替换，所有申请、归还期间地址稳定
    ProcessWorkPacketSlot* m_slots;
    // 与m_slots同生命周期的固定槽数
    uint32_t m_capacity;
    // 运行中只追加的稳定块链
    std::atomic<ProcessWorkPacketBlock*> m_blocks;
    // 已实际发布的槽数
    std::atomic<uint64_t> m_total;
    // 已保留和仍在申请的槽数，CAS预留防止并发突破有限上限
    std::atomic<uint64_t> m_reserved;
    // 成功扩容次数，不含初始化
    std::atomic<uint64_t> m_grows;
    // 初始配置和增长机制，运行期间只读
    ProcessWorkMemoryConfig m_config;
};