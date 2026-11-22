#pragma once
#include "NetConnectionPoolConfig.h"
#include "NetBufferPool.h"

// 单执行者链中的编码帧；所有权转交后仍占用记录池和正文池
struct NetConnectionWrite
{
public:
    // 所属记录池，覆盖本记录生命周期
    NetObjectPool<NetConnectionWrite>* m_pool;
    // 本记录的不透明池槽，最终释放归还一次
    void* m_storage;
    // 同一优先级的后继，不拥有，队列收敛时统一释放
    NetConnectionWrite* m_next;
    // 实际编码缓冲租约
    NetBufferLease m_buffer;
    // 缓冲中应擦除的有效字节数；编码失败时包括可能写过的前缀
    uint32_t m_size;

public:
    /** 创建未接入任何链的空记录
    */
    NetConnectionWrite();

    /** 擦除已使用的明文，成员租约随后归还物理缓冲
    */
    ~NetConnectionWrite();
};

// 协议owner独占的两级FIFO；池负责存储，队列不重复设置条数/字节接纳窗口
class NetConnectionQueue
{
public:
    /** 初始化独立数据和控制池
    @param [in] config 实际组件池配置，构造时复制
    */
    explicit NetConnectionQueue(const NetConnectionPoolConfig& config);

    /** 收敛全部链和当前部分发送帧后再析构池
    */
    ~NetConnectionQueue();

    /** 查询实际物理池初始化结果
    @return NI_OK或参数/内存/平台错误
    */
    int32_t status() const;

    /** 备齐记录和缓冲，失败不改变链
    @param [in] control 是否使用独立控制资源
    @param [in] bytes 需要的编码空间
    @param [out] output 空引用；成功后调用方完成编码再append
    @return NI_OK或明确未接纳错误
    */
    int32_t allocate(bool control, uint32_t bytes, NetObjectReference<NetConnectionWrite>& output);

    /** 接管已编码帧，接入相应FIFO尾部，不分配内存
    @param [in] control 与分配时相同的队列
    @param [in] write 已从引用转交的独占记录
    */
    void append(bool control, NetConnectionWrite* write);

    /** 借用当前帧，空闲时从控制队列优先且有轮转公平性地取一帧；部分发送不被插队
    @return 借用至complete或clear，可能为空
    */
    NetConnectionWrite* active();

    /** 完成当前帧并归还其物理资源
    */
    void complete();

    /** 关闭/失败时释放全部明文，容量留在池中复用
    */
    void clear();

    /** 是否仍拥有尚未交给TLS的完整帧或部分帧
    @return 是否为空
    */
    bool empty() const;

private:
    /** 按实际缓冲容量选择复用链，不改变物理池的额度
    @param [in] bytes 所需或实际容量，已验证不超过最大正文类
    @return 缓冲类下标0至4
    */
    static uint32_t bufferClass(uint32_t bytes);

private:
    // 独占空闲记录仍占实际池槽；每级每容量类一条链，不与其他线程共享
    NetConnectionWrite* m_cached[2][5];
    // 正文池先于记录池声明，保证晚于缓冲租约析构
    NetBufferPool m_dataBytes;
    // 控制正文独立预算
    NetBufferPool m_controlBytes;
    // 普通帧记录
    NetObjectPool<NetConnectionWrite> m_dataRecords;
    // 控制帧记录
    NetObjectPool<NetConnectionWrite> m_controlRecords;
    // 已从队列摘下的当前帧，仍拥有池槽
    NetConnectionWrite* m_active;
    // 连续控制帧调度次数；32帧后让普通帧取得一次机会，不限制接纳数量
    uint32_t m_controlRun;
    // 两级FIFO首项
    NetConnectionWrite* m_heads[2];
    // 两级FIFO尾项
    NetConnectionWrite* m_tails[2];
};