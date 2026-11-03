#pragma once
#include "NetInterface/NetInterfaceAPI.h"
#include <memory>
#include <vector>

// 本服务实例独占的消息；正文只在消费或最终销毁时擦除。
class NetSenderServiceManagedFrame
{
public:
    /** 创建空槽，不分配业务正文
    */
    NetSenderServiceManagedFrame();

    /** 擦除仍由本槽持有的业务正文
    */
    ~NetSenderServiceManagedFrame();

    /** 恢复借用视图并扣除排队期限，分配失败不修改原消息
    @param [in] now 单调毫秒，不得早于接纳时间
    @param [out] frame 借用本槽或scratch的视图
    @param [out] scratch 调用者负责最终擦除的请求临时体
    @return READY、TIMEOUT、NO_MEMORY或协议错误
    */
    int32_t view(uint64_t now, NetInterfaceFrame& frame, std::vector<uint8_t>& scratch) const;

public:
    // 接纳时复制的帧头，payload不指向调用者
    NetInterfaceFrame m_frame;
    // 路由器给出的真实来源，不能由业务正文覆盖
    NetInterfaceOrigin m_origin;
    // 独占业务正文；借用结束前不移动或清空
    std::unique_ptr<uint8_t[]> m_bytes;
    // 实际分配大小，不参与待发字节计数；复用仅保留小缓冲
    uint32_t m_capacity;
    // 目标端点，由路由层解释
    uint64_t m_endpoint;
    // 首次进入本库队列的时刻，背压重试不刷新
    uint64_t m_created;
    // 首次接收背压的时刻，0表示尚未背压
    uint64_t m_blocked;
    // 可选的直接目标物理通道，0时按端点映射
    uint64_t m_channel;
    // 队列或空闲链中的后继；256表示末尾
    uint32_t m_next;
    // 队列中的前驱；256表示队首或未排队
    uint32_t m_previous;
    // 0至2为排队，3至5为对应队列的借用，6为空闲
    uint32_t m_lane;
};

// 三路队列共享256槽和8MiB；借用项仍属于本拥有者并继续占用额度。
class NetSenderServiceQueue
{
public:
    /** 建立固定空闲链及三条空队列
    */
    NetSenderServiceQueue();

    /** 最终释放所有槽及复用缓冲；调用者必须先结束借用
    */
    ~NetSenderServiceQueue();

    /** 原子接纳消息；匹配尚未交付的请求取消时直接删除该请求
    @param [in] lane 0业务回调、1客户端输出、2内部依赖
    @param [in] endpoint 目标端点
    @param [in] frame 完整帧，调用期间借用正文
    @param [in] origin 已验证来源
    @param [in] now 首次接纳的单调时刻
    @return OK、LIMIT_EXCEEDED、INVALID_ARGUMENT或NO_MEMORY
    */
    int32_t push(uint32_t lane, uint64_t endpoint, const NetInterfaceFrame& frame,
        const NetInterfaceOrigin& origin, uint64_t now);

    /** 查询共享预算，包含当前回调借用的消息
    @param [in] count 新增条数
    @param [in] bytes 新增正文字节数
    @return 是否能完整接纳
    */
    bool room(uint32_t count, uint64_t bytes) const;

    /** 查看指定队列第一项
    @param [in] lane 0至2
    @return 槽号或256
    */
    uint32_t first(uint32_t lane) const;

    /** 查看指定队列最后一项
    @param [in] lane 0至2
    @return 槽号或256
    */
    uint32_t last(uint32_t lane) const;

    /** 取得当前拥有项，不增加所有权
    @param [in] index 槽号
    @return 有效槽或null，借用规则由borrow/restore/release维护
    */
    NetSenderServiceManagedFrame* get(uint32_t index);

    /** 只读取得当前拥有项，不能延长其借用期
    @param [in] index 槽号
    @return 有效槽或null
    */
    const NetSenderServiceManagedFrame* get(uint32_t index) const;

    /** 从队列取下并借用，条数和字节预算保持占用
    @param [in] index 当前排队槽
    @return 当前消息或null
    */
    NetSenderServiceManagedFrame* borrow(uint32_t index);

    /** 背压后无分配归还原队列头，不改变共享预算
    @param [in] index 当前借用槽
    @return 是否归还成功
    */
    bool restore(uint32_t index);

    /** 擦除并释放排队或借用项，归还精确额度
    @param [in] index 当前拥有槽；无效槽忽略
    */
    void release(uint32_t index);

    /** 清理指定队列；回调借用保持到其拥有者返回
    @param [in] lane 0至2
    */
    void clear(uint32_t lane);

    /** 清理三路排队项，仍不使当前回调的借用失效
    */
    void clear();

    /** 查询一条队列中的等待数，不包括已取出的借用
    @param [in] lane 0至2
    @return 等待数
    */
    uint32_t size(uint32_t lane) const;

    /** 查询全部拥有项，包括借用
    @return 条数
    */
    uint32_t size() const;

    /** 查询全部拥有正文，包括借用
    @return 正文字节数
    */
    uint64_t bytes() const;

private:
    /** 只解开排队链，不释放正文和共享额度
    @param [in] index 已在某队列中的槽
    */
    void detach(uint32_t index);

    // 固定拥有槽；只有FREE可被新接纳使用
    NetSenderServiceManagedFrame m_items[256];
    // 三条队首，256表示空
    uint32_t m_heads[3];
    // 三条队尾，256表示空
    uint32_t m_tails[3];
    // 三条排队计数，不含借用
    uint32_t m_sizes[3];
    // 空闲链首
    uint32_t m_free;
    // 当前全部拥有项数，包含借用
    uint32_t m_count;
    // 当前全部正文大小，包含借用
    uint64_t m_bytes;
    // 最多保留一份已擦除、容量不超过4KiB的小缓冲
    std::unique_ptr<uint8_t[]> m_spare;
    // 复用缓冲容量，无缓冲时为0
    uint32_t m_spareCapacity;

    /** 禁止复制独占消息拥有者
    @param [in] other 不允许复制的源对象
    */
    NetSenderServiceQueue(const NetSenderServiceQueue& other);

    /** 禁止覆盖仍拥有消息的队列
    @param [in] other 不允许赋值的源对象
    @return 未实现
    */
    NetSenderServiceQueue& operator=(const NetSenderServiceQueue& other);
};

// 栈上借用守卫，任何退出或异常均释放当前消息；归队成功后解除释放责任。
class NetSenderServiceQueueLoan
{
public:
    /** 借用已由queue.borrow取下的项
    @param [in,out] queue 原拥有者
    @param [in] index 当前借用槽
    */
    NetSenderServiceQueueLoan(NetSenderServiceQueue& queue, uint32_t index);

    /** 释放尚未归队的借用项
    */
    ~NetSenderServiceQueueLoan();

    /** 无分配归队并解除本守卫责任
    @return 是否成功
    */
    bool restore();

    /** 当前视图已经不用时提前归还额度，避免生成终态时仍占用原请求槽
    */
    void release();

private:
    // 借用至析构，队列必须覆盖本守卫
    NetSenderServiceQueue& m_queue;
    // 256表示责任已经交还队列
    uint32_t m_index;
    /** 禁止复制借用释放责任
    @param [in] other 不允许复制的守卫
    */
    NetSenderServiceQueueLoan(const NetSenderServiceQueueLoan& other);

    /** 禁止覆盖借用释放责任
    @param [in] other 不允许赋值的守卫
    @return 未实现
    */
    NetSenderServiceQueueLoan& operator=(const NetSenderServiceQueueLoan& other);
};

// 一次投递的明文暂存；异常和提前返回也必须擦除，不把业务明文遗留在释放堆块。
class NetSenderServiceQueueScratch
{
public:
    /** 擦除所有临时正文
    */
    ~NetSenderServiceQueueScratch();

public:
    // RPC剩余期限编码
    std::vector<uint8_t> m_request;
    // 账号推送编码
    std::vector<uint8_t> m_event;
    // 服务间来源信封编码
    std::vector<uint8_t> m_wrapped;
};