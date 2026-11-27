#pragma once
#include "NetSession.h"
#include "NetObjectPool.h"
#include <unordered_map>
#include <vector>
#include <memory>

// 在单执行者会话调用外部代码期间阻止重入，离开当前作用域即恢复原状态。
class NetSessionCallbackScope
{
public:
    /** 标记回调期间的借用
    @param [in,out] flag 所属会话的回调标志，寿命覆盖本作用域
    */
    explicit NetSessionCallbackScope(bool& flag);

    /** 恢复进入时的标志，保留外层回调的状态
    */
    ~NetSessionCallbackScope();

private:
    /** 禁止复制一份恢复责任
    @param [in] other 原作用域
    */
    NetSessionCallbackScope(const NetSessionCallbackScope& other) = delete;

    /** 禁止转移恢复责任
    @param [in] other 原作用域
    @return 当前作用域
    */
    NetSessionCallbackScope& operator=(const NetSessionCallbackScope& other) = delete;

private:
    // 所属会话的回调标志，在析构前有效。
    bool& m_flag;
    // 进入回调前的标志，用于嵌套恢复。
    bool m_previous;
};

// 客户端单一在途记录；元数据不再复制到另一份路由等待表。
struct NetSessionPending
{
    // m_original 为业务预留号；m_route 为已接纳的线路号，未发送时为0。
    NetInterfaceRequest m_request;
    // 最小截止堆中的位置，摘除时不扫描全部请求。
    size_t m_heapIndex;
};

// 一个会话的协议状态；仅唯一执行者访问，不提供线程或物理运输。
class NetSessionState
{
public:
    /** 初始化物理请求池和单执行者索引，不设固定128条窗口
    */
    NetSessionState();

    /** 擦除发送缓冲区的最后一份明文
    */
    ~NetSessionState();

    /** 复制调用方的客户端 HELLO 参数，生成随机数后校验完整握手
    @param [in] offered 本次连接声明的能力及上限
    @param [in] tls 提供随机源和上下文
    @param [out] prepared 成功时为本次验证过的配置
    @return NI_OK、NI_INVALID_ARGUMENT 或 NI_INVALID_STATE
    */
    static int32_t prepareHandshake(const NetInterfaceHandshakeOptions& offered, const NetInterfaceTlsOptions& tls,
        NetInterfaceHandshakeOptions& prepared);

    /** 清空旧代次的请求及高水位，不执行上层回调
    @param [in] limit 本代次已验证的非零对端声明
    */
    void resetRequests(uint32_t limit);

    /** 原子预留一条客户端请求，只分配一个拥有节点
    @param [in] original 0自动编号，否则必须严格高于本代次高水位
    @param [in] product 非零产品
    @param [in] type 命令
    @param [in] deadline 已校验的原始总截止
    @param [out] requestId 成功为非零原号，失败0
    @return OK、PROTOCOL_ERROR、LIMIT_EXCEEDED或NO_MEMORY
    */
    int32_t reserve(uint64_t original, uint32_t product, uint32_t type, uint64_t deadline, uint64_t& requestId);

    /** 从唯一拥有表及截止堆同时摘除，不分配内存
    @param [in] original 原号
    @param [out] pending 成功为完整记录，失败不写
    @return 是否存在
    */
    bool take(uint64_t original, NetSessionPending& pending);

    /** 获取最早到期原号，空堆和未到期均返回0
    @param [in] nowMs 当前单调时间
    @return 到期原号，不移除
    */
    uint64_t expired(uint64_t nowMs) const;

    /** 擦除全部暂存明文，容量超过本会话的非负预算时立即释放
    */
    void clearPayload();

private:
    /** 按截止及原号稳定排序，不使用节点地址作为次序
    @param [in] left 有效在途项
    @param [in] right 有效在途项
    @return left应先到期
    */
    static bool earlier(const NetSessionPending* left, const NetSessionPending* right);

    /** 交换两个堆位置并同步节点反向位置
    @param [in] left 第一个节点在当前截止堆中的有效下标
    @param [in] right 第二个节点在当前截止堆中的有效下标
    */
    void exchange(size_t left, size_t right);

    /** 向根调整一项
    @param [in] index 有效位置
    */
    void rise(size_t index);

    /** 向叶调整一项
    @param [in] index 有效位置
    */
    void sink(size_t index);

public:
    // 本代实际借用通道，停止时清空。
    NetClientChannel* m_channel;
    // 启动前绑定的借用通道，不拥有。
    NetClientChannel* m_external;
    // 本代应用握手，TLS仍由通道负责。
    std::unique_ptr<NetInterfaceHandshake> m_handshake;
    // 请求记录的物理池，先于索引声明，保证晚于全部引用销毁
    std::unique_ptr<NetObjectPool<NetSessionPending>> m_records;
    // 单执行者无序索引，每个节点对应一个已取得的池槽，不另设请求条数窗口
    std::unordered_map<uint64_t, NetObjectReference<NetSessionPending>> m_pending;
    // 只为线路号与业务号不同的已发送请求保存反向索引。
    std::unique_ptr<std::unordered_map<uint64_t, uint64_t>> m_remapped;
    // 只借用池槽的截止堆，容量随实际记录增长，关闭时复用预热容量
    std::vector<NetSessionPending*> m_expiry;
    // 本代请求原号高水位；容量拒绝仍消耗原号。
    uint64_t m_highestOriginal;
    // 本代运输已接纳的最后一个请求号，背压不推进它。
    uint64_t m_highestWire;
    // 握手协商后的请求上限。
    uint32_t m_maxRequests;
    // 本会话独立的心跳及总无输入期限
    NetHeartbeatConfig m_heartbeat;
    // 每次RPC发送后可保留的容量，-1保留，关闭时全部释放
    int64_t m_payloadRetentionBytes;
    // 每次poll可结算的到期请求数
    uint32_t m_requestExpiryBudget;
    // 本次构造的RPC明文，离开发送调用前擦除。
    std::vector<uint8_t> m_payload;
    // 启动前配置的公开无秘密消息类型，动态存储且仅由协议执行者访问。
    std::vector<uint32_t> m_compressionTypes;
    // 本代固定握手参数。
    NetInterfaceHandshakeOptions m_options;
    // 业务观察者借用到本代关闭。
    NetSessionListener* m_listener;
    // 本通道的物理代次号。
    uint64_t m_peer;
    // 本会话接纳代次，单调递增。
    uint64_t m_generation;
    // 最近认证帧的接收时刻。
    uint64_t m_lastReceive;
    // 最近心跳编号，不复用。
    uint64_t m_ping;
    // 尚待响应心跳的发送时刻，0表示无在途心跳。
    uint64_t m_pingSent;
    // 已协商业务块上限。
    uint32_t m_maxChunk;
    // 已协商能力位。
    uint32_t m_capabilities;
    // 尚未交付本代关闭。
    bool m_active;
    // 本代握手完成且未请求关闭。
    bool m_ready;
    // 唯一poll执行者保护。
    bool m_polling;
    // 允许嵌套业务调用，但禁止重连/释放的回调保护。
    bool m_callback;
    // 正在结算本代关闭，区别于连接准备及普通业务回调。
    bool m_notifyingClose;
    // 已记录首次本地关闭意图。
    bool m_localClose;
    // 运输的 close 抛出异常，等待退出借用回调后停止该通道。
    bool m_closeFailed;
    // 0/CANCELLED为主动停止；其它值为连接故障。
    int32_t m_closeReason;
};