#pragma once
#include "NetSenderServiceProxy.h"
#include <memory>
#include <vector>

// 单个根请求的唯一阶段，取消不撤销远端已经发生的业务副作用。
enum NetSenderProxyPhase
{
    // 槽位未被占用
    NET_SERVICE_PROXY_FREE,
    // 根请求已接纳，等待按子请求号顺序交给服务
    NET_SERVICE_PROXY_QUEUED,
    // 子请求已交付，等待唯一响应
    NET_SERVICE_PROXY_WAITING,
    // 响应已复制，等待交回根请求
    NET_SERVICE_PROXY_REPLY,
    // 根已取消，最多保留五秒交付子取消
    NET_SERVICE_PROXY_CANCEL
};

// 实际池槽的正文归本对象独占；只由宿主协议执行者访问。
class NetSenderProxyCall
{
public:
    /** 创建未占用槽位，不分配正文
    */
    NetSenderProxyCall();

    /** 擦除并释放尚未交付的正文
    */
    ~NetSenderProxyCall();

    /** 在空正文上事务性复制RPC业务体；16字节头在prepare中填写
    @param [in,out] bodies 实际正文池，存活至本调用清理
    @param [in] rpc 已由NetInterfaceRpc验证的输入，借用到返回
    @return false分配失败，正文和计数尚未改变
    */
    bool assign(NetBufferPool& bodies, const NetInterfaceRpcView& rpc);

    /** 擦除正文及内联头，保留请求关联
    */
    void release();

    /** 擦除正文后恢复未占用状态
    */
    void clear();

    /** 验证同一来源、产品及命令，不能仅用请求号授权取消或响应
    @param [in] frame 已验证的取消或响应
    @param [in] origin 网关附加的可信来源
    @return 是否精确匹配本根上下文
    */
    bool matches(const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin) const;

public:
    // 网关附加的来源；不是客户端自报授权
    NetInterfaceOrigin m_origin;
    // 当前槽位阶段
    NetSenderProxyPhase m_phase;
    // 根及子请求号；子号在发送现场分配，背压后舍弃未交付号，不复用
    uint64_t m_root;
    // 已分配的子请求号，0表示尚未为本次交付尝试分配
    uint64_t m_child;
    // 原始单调总期限，重试不延长
    uint64_t m_deadline;
    // 子取消最终交付期限；0为尚未取消
    uint64_t m_cancelUntil;
    // 精确产品和命令
    uint32_t m_product;
    // 根请求的业务命令号，响应和取消必须精确匹配
    uint32_t m_type;
    // 响应RPC状态
    uint32_t m_status;
    // 当前拥有的业务体字节，不含本记录的16字节RPC头
    uint32_t m_size;
    // 活跃FIFO前后槽位，UINT32_MAX为空
    uint32_t m_previous;
    // 活跃FIFO的后一槽位，UINT32_MAX表示尾部
    uint32_t m_next;
    // 零正文时使用内联头，不分配堆内存
    uint8_t m_header[16];
    // 非空正文只复制一次；连续存储16字节头和正文，poll不再分配整包
    NetBufferLease m_wire;

private:
    /** 禁止复制独占正文
    @param [in] other 不可复制实例
    */
    NetSenderProxyCall(const NetSenderProxyCall& other);

    /** 禁止复制独占正文
    @param [in] other 不可复制实例
    @return 不可调用
    */
    NetSenderProxyCall& operator=(const NetSenderProxyCall& other);
};

// 可复用稳定编号；目录扩容不移动实际调用及其正文
struct NetSenderProxySlot
{
    // 真正占用记录池的租约，空闲目录项不占记录池
    NetObjectReference<NetSenderProxyCall> m_value;
    // 空闲编号链后继
    uint32_t m_next;

    /** 构造空闲编号
    */
    NetSenderProxySlot();
};

// 单执行者维护FIFO，实际记录及正文池限制存储，不设置代理请求条数门槛。
class NetSenderServiceProxyState
{
public:
    /** 借用服务，生命周期覆盖全部代理操作
    @param [in] service 宿主服务对象
    */
    explicit NetSenderServiceProxyState(NetSenderService& service);

    /** 首次接纳前替换真实存储池，配置失败保留旧池
    @param [in] bodies 正文分级存储配置
    @param [in] records 调用记录存储配置
    @return NI_OK或状态、配置、分配错误
    */
    int32_t configurePools(const NetBufferPoolConfig& bodies, const LockFreeHashMapPoolConfig& records);

    /** 申请记录和正文，失败不改变队列及空闲编号链
    @param [in] rpc 已校验业务输入
    @return 尚未入队的稳定编号，UINT32_MAX为分配失败
    */
    uint32_t acquire(const NetInterfaceRpcView& rpc);

    /** 取出仍由状态持有的记录；调用者保证编号有效
    @param [in] index 已占用的编号
    @return 可修改实际记录
    */
    NetSenderProxyCall& call(uint32_t index);

    /** 只读访问已占用记录
    @param [in] index 已占用的编号
    @return 借用实际记录
    */
    const NetSenderProxyCall& call(uint32_t index) const;

    /** 擦除全部未完成请求，回到空且有效的代次
    */
    void reset();

    /** 擦除本代全部拥有正文并标为无效，宿主在回调外关闭代次
    */
    void fail();

    /** 在活跃槽位中查询根或子请求
    @param [in] id 请求号
    @param [in] root true查根号，false查子号
    @return 稳定编号，UINT32_MAX未找到
    */
    uint32_t find(uint64_t id, bool root) const;

    /** 将已准备槽位追加至FIFO并计入正文预算
    @param [in] index 空槽位，关联及正文已经完整构造
    */
    void append(uint32_t index);

    /** 从FIFO摘除后归还预算，最后擦除正文和关联
    @param [in] index 活跃槽位
    */
    void erase(uint32_t index);

    /** 原地准备本轮输出，处理原始期限及有限取消宽限
    @param [in] index 活跃槽位
    @param [in] now 单调毫秒
    @param [out] frame 借用正文至finish或reset
    @return true有输出；false等待或已回收
    */
    bool prepare(uint32_t index, uint64_t now, NetInterfaceFrame& frame);

    /** 根据真实服务发送结果提交阶段；失败不越过受阻子请求
    @param [in] index prepare返回的槽位
    @param [in] result 服务发送结果
    @return true本轮后续未发送子请求必须停留，响应及取消仍可推进
    */
    bool finish(uint32_t index, int32_t result);

public:
    // 非拥有服务指针，只在同一协议执行者调用
    NetSenderService* m_service;
    // 正文池在调用记录之后销毁
    std::unique_ptr<NetBufferPool> m_bodies;
    // 真实调用池，默认初始128槽，最大约8MiB实际记录存储
    std::unique_ptr<NetObjectPool<NetSenderProxyCall>> m_records;
    // 可增长的编号目录，只保存引用，扩容不移动调用记录
    std::vector<NetSenderProxySlot> m_calls;
    // 空闲编号链，UINT32_MAX表示需要增长目录
    uint32_t m_free;
    // 首次成功接纳后不得替换实际池
    bool m_used;
    // 活跃FIFO首尾，UINT32_MAX为空
    uint32_t m_first;
    // 活跃FIFO尾槽，UINT32_MAX表示没有活跃请求
    uint32_t m_last;
    // 已接纳记录数，仅作统计，不用于限制入队
    uint32_t m_count;
    // 当前独占正文总字节数，仅作统计，erase归还实际存储
    uint64_t m_bytes;
    // 协议或服务是否仍有效；reset开始新代次
    bool m_valid;
    // 同执行者poll重入守卫，不是跨线程锁
    bool m_polling;
};