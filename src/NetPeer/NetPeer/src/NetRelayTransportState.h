#pragma once
#include "NetRelayTransport.h"
#include "NetHostSocket.h"
#include <map>
#include <memory>
#include <vector>

// 中继实例单次启动；停止意图先封住入口，再在最外层poll回收
enum NetRelayTransportPhase
{
    // 尚未启动，可校验参数
    NET_RELAY_IDLE,
    // 已交给宿主启动，不允许递归启动
    NET_RELAY_STARTING,
    // 可推进连接和发送
    NET_RELAY_ACTIVE,
    // 已拒绝新工作，等待当前回调退出
    NET_RELAY_STOPPING,
    // 所有peer和通知已结算，实例不可复用
    NET_RELAY_STOPPED
};

// TCP代次的上层交付状态；原始EOF另存，因为EOF前的完整记录仍须交付
enum NetRelayPeerPhase
{
    // 主动连接尚未就绪，受五秒期限限制
    NET_RELAY_CONNECTING,
    // TCP就绪，尚未通知上层
    NET_RELAY_CONNECTED,
    // 已通知上层，可交付完整记录
    NET_RELAY_ESTABLISHED,
    // 首个终态已确定，只等原始资源关闭
    NET_RELAY_CLOSING
};

// 固定队列中的唯一缓冲所有者；析构清理有效字节，不保留密文副本
class NetRelayTransportWrite
{
public:
    /** 创建不分配缓冲的空槽
    */
    NetRelayTransportWrite();

    /** 清理仍在队列中的有效字节并释放缓冲
    */
    ~NetRelayTransportWrite();

public:
    // 独占整条NTW3记录；空槽必须为空
    std::unique_ptr<uint8_t[]> m_bytes;
    // 有效字节数，只有接纳成功后才写入
    uint32_t m_size;
    // 已分配容量，最多16408字节
    uint32_t m_capacity;
    // 使用控制额度还是业务额度
    bool m_control;
};

// 单个TCP代次：输入解码、输出FIFO和额度由同一执行者独占
class NetRelayTransportPeer
{
public:
    /** 创建未连接、未报告的独占代次
    @param [in] now 单调毫秒
    */
    explicit NetRelayTransportPeer(uint64_t now);

    /** 清理输入和仍未交付的输出，不发回调
    */
    ~NetRelayTransportPeer();

    /** 验证并复制完整记录，失败不改变队列和额度
    @param [in] record 借用到返回，不允许引用本队列存储
    @return OK已接纳、LIMIT_EXCEEDED未接纳或校验/分配错误
    */
    int32_t enqueue(const NetInterfaceTunnelRecord& record);

    /** 下层完整接纳队首后归还额度并清理缓冲，最多保留一份空缓冲
    */
    void pop();

    /** 清理已消费输入，保持最多16KiB的容量供本代次复用
    */
    void clearInput();

public:
    // 单条NTW3解码状态，借用由peek/consume界定
    NetInterfaceTunnel m_decoder;
    // 暂停接收后的唯一原始块，最多16384字节
    std::vector<uint8_t> m_input;
    // 32条业务与32条控制共用FIFO，控制不得越过已接纳业务
    NetRelayTransportWrite m_output[64];
    // 只缓存一份已清理的输出缓冲，避免64个空槽各自保留大缓冲
    std::unique_ptr<uint8_t[]> m_recycled;
    // 空缓冲容量；所有有效数据已清理
    uint32_t m_recycledCapacity;
    // 输出FIFO队首，队尾由两类数量之和派生
    uint32_t m_outputHead;
    // 当前原始块已交给解码器的字节数
    uint32_t m_inputOffset;
    // 已接纳业务记录总字节，最多4*16408
    uint32_t m_dataBytes;
    // 已接纳控制记录总字节，最多2*16408
    uint32_t m_controlBytes;
    // 已接纳业务条数，最多32
    uint32_t m_dataCount;
    // 已接纳控制条数，最多32
    uint32_t m_controlCount;
    // 连接开始时间，不因重复事件延期
    uint64_t m_started;
    // 上层拒绝接收同一记录的起点，0表示未背压
    uint64_t m_blockedSince;
    // 单一上层交付阶段
    NetRelayPeerPhase m_phase;
    // 是否已请求暂停底层读取
    bool m_paused;
    // 原始EOF已到；未消费的完整记录仍交付
    bool m_closed;
    // 首个终态；正常EOF可在检查残包时变成TRUNCATED
    int32_t m_reason;
};

// 把原始宿主事件串行交给每个TCP代次，不持有业务密钥或路由状态
class NetRelayTransportState : public NetHostSocketListener
{
public:
    /** 创建独占网络状态
    */
    NetRelayTransportState();

    /** 获得单调毫秒
    @return 时间
    */
    static uint64_t now();

    /** 推进一个连接，输入和输出各最多64步，失败只关闭本连接
    @param [in] id 代次
    @param [in,out] peer 独占连接
    */
    void pump(uint64_t id, NetRelayTransportPeer& peer);

    /** 标记并关闭，一次原因保持不变
    @param [in] id 代次
    @param [in] reason 原因
    */
    void close(uint64_t id, int32_t reason);

    /** 保存原始TCP建立事件，通知上层留到pump
    @param [in] peer 当前代次
    */
    virtual void onConnected(uint64_t peer);

    /** 复制唯一有界块后暂停；错误只关闭当前代次
    @param [in] peer 当前代次
    @param [in] data 借用到返回
    @param [in] size 1至16384字节
    */
    virtual void onData(uint64_t peer, const uint8_t* data, uint32_t size);

    /** 下次pump自然重试队首，不在回调里递归发送
    @param [in] peer 当前代次
    */
    virtual void onWritable(uint64_t peer);

    /** 接收规范化的原始传输终态，保留EOF前的完整输入
    @param [in] peer 当前连接
    @param [in] reason NI关闭原因
    @param [in] detail 原始宿主诊断，本层不参与协议判断
    */
    virtual void onClosed(uint64_t peer, int32_t reason, int32_t detail);

public:
    // 独占宿主适配器，先停止再释放
    NetHostSocket m_channel;
    // 借用上层监听者直到shutdown返回
    NetRelayTransportListener* m_listener;
    // 每个TCP代次的唯一所有者
    std::map<uint64_t, std::unique_ptr<NetRelayTransportPeer>> m_peers;
    // 单次启动/停止阶段
    NetRelayTransportPhase m_phase;
    // 成功启动了监听，允许空连接时继续poll
    bool m_server;
    // 防止递归驱动，并推迟回调内shutdown的销毁
    bool m_polling;
};