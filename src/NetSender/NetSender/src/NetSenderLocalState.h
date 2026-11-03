#pragma once
#include "NetSenderLocal.h"
#include "NetSenderLocalPacket.h"
#include "NetSenderIpcChannel.h"
#include <deque>
#include <map>
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include <string>
#include <vector>
#include <set>
#include <array>

// 从回调队列移交给唯一协议owner的消息；字节独占，失败通知不分配正文。
struct NetSenderLocalInput
{
public:
    // 实际ProcessWork发送PID，仅用于寻址，不作为业务授权。
    int32_t m_pid;
    // 收件箱登记的本地通道号；尚未识别的OPEN为0，绝不从网络包采用。
    uint64_t m_peer;
    // 正常输入为NI_OK；负数表示仅此通道的唯一接纳失败通知。
    int32_t m_reason;
    // 验证后的NIP2种类，用于归还独立数据/控制槽位。
    uint32_t m_kind;
    // 本模块独占字节，回调复制一次，随后只转移所有权。
    std::vector<uint8_t> m_bytes;

public:
    /** 创建空输入，不分配正文
    */
    NetSenderLocalInput();

    /** 转移完整元数据及正文，原对象清空
    @param [in,out] other 被转移对象
    */
    NetSenderLocalInput(NetSenderLocalInput&& other);

    /** 释放旧正文并转移新输入，支持自移动
    @param [in,out] other 被转移对象
    @return 本对象
    */
    NetSenderLocalInput& operator=(NetSenderLocalInput&& other);

private:
    /** 禁止复制正文所有权
    @param [in] other 原对象
    */
    NetSenderLocalInput(const NetSenderLocalInput& other);

    /** 禁止复制正文所有权
    @param [in] other 原对象
    @return 不可调用
    */
    NetSenderLocalInput& operator=(const NetSenderLocalInput& other);
};

// 单通道FIFO，正常数据4项、控制16项；未识别OPEN单独使用64项暂存入口。
class NetSenderLocalInboxLane
{
public:
    /** 建立空通道，不分配正文
    @param [in] pid 绑定的实际发送PID；OPEN入口为0
    @param [in] peer 本地owner不复用的通道号；OPEN入口为0
    @param [in] remote 已知16字节远端代次，客户端尚未知时为空
    */
    NetSenderLocalInboxLane(int32_t pid, uint64_t peer, const uint8_t* remote = nullptr);

public:
    // 登记时固定，回调不能改写。
    int32_t m_pid;
    // owner分配的本地号，错误通知只影响此号。
    uint64_t m_peer;
    // 服务端登记的远端代次；未识别或客户端尚未知时全零。
    std::array<uint8_t, 16> m_remote;
    // 仍待交付的密文项数，不包含控制包。
    uint32_t m_dataCount;
    // 独立控制槽位，不能被四包数据窗口占用。
    uint32_t m_controlCount;
    // 第一项接纳错误；后续输入丢弃直到owner移除此通道。
    int32_t m_reason;
    // 是否已连入全箱就绪链。
    bool m_queued;
    // 就绪链前项，非拥有；所有访问在收件箱锁内。
    NetSenderLocalInboxLane* m_previous;
    // 就绪链后项，非拥有。
    NetSenderLocalInboxLane* m_next;
    // 本通道严格按原回调交付顺序，ACK/END不越过此前DATA。
    std::deque<NetSenderLocalInput> m_input;
};

// 回调与协议owner之间的有界同步状态；就绪通道轮流交付，不扫描空通道。
class NetSenderLocalInboxState
{
public:
    /** 创建停用的状态和独立OPEN入口
    */
    NetSenderLocalInboxState();

    /** 空通道首次就绪时无分配追加，已有位置不变
    @param [in,out] lane 本状态拥有的通道，锁内调用
    */
    void queue(NetSenderLocalInboxLane& lane);

    /** 从就绪链摘除，不删除对象
    @param [in,out] lane 本状态拥有的通道，锁内调用
    */
    void unqueue(NetSenderLocalInboxLane& lane);

    /** 只终止一个已登记通道；失败通知使用预留元数据，不依赖再次分配
    @param [in,out] lane 已登记通道，不能是OPEN入口
    @param [in] reason 负数错误，保留首次原因
    */
    void fail(NetSenderLocalInboxLane& lane, int32_t reason);

    /** 登记目标后将同源的待处理OPEN移到同一FIFO，防止后续DATA越过握手尾片
    @param [in,out] lane 已登记且已知远端代次的通道，锁内调用
    */
    void adoptOpen(NetSenderLocalInboxLane& lane);

    /** 释放全部队列及通道索引，不触碰ProcessWork
    */
    void clear();

public:
    // 只保护收件箱元数据与字节所有权，不在锁内调用业务或ProcessWork。
    ReadWriteMutex m_mutex;
    // 随机目标到稳定独占通道，最多64；同时绑定PID减少无关输入影响。
    std::map<std::array<uint8_t, 16>, std::unique_ptr<NetSenderLocalInboxLane>> m_generations;
    // 未识别OPEN独立限额，洪泛只能影响新连接，不能终止已有通道。
    NetSenderLocalInboxLane m_open;
    // 就绪链首项，take为常数时间并轮转仍有输入的通道。
    NetSenderLocalInboxLane* m_first;
    // 就绪链末项。
    NetSenderLocalInboxLane* m_last;
    // start到stop期间接纳输入。
    bool m_active;
    // 只有服务端接纳目标全零的OPEN。
    bool m_acceptOpen;
};

// 一片密文的独占存储及消费位置，只能移动不能复制
struct NetSenderLocalChunk
{
public:
    // 本条NIP2序号，待发送记录初始为0
    uint64_t m_packet;
    // 当前已消费字节数，包含内部信封偏移
    uint32_t m_offset;
    // 本层独占字节存储或队列累计字节数，按所属类型使用
    std::vector<uint8_t> m_bytes;

public:
    /** 创建空的独占缓冲，兼容v120不隐式生成移动操作的行为
    */
    NetSenderLocalChunk();

    /** 转移字节缓冲，不复制正文
    @param [in,out] other 转移后标量清零，字节所有权归本对象
    */
    NetSenderLocalChunk(NetSenderLocalChunk&& other);

    /** 释放旧缓冲并转移新缓冲；支持自移动
    @param [in,out] other 被转移的独占对象
    @return 本对象
    */
    NetSenderLocalChunk& operator=(NetSenderLocalChunk&& other);
private:
    /** 禁止复制独占缓冲
    @param [in] other 不可复制对象
    */
    NetSenderLocalChunk(const NetSenderLocalChunk& other);

    /** 禁止复制赋值
    @param [in] other 不可复制对象
    @return 不可调用
    */
    NetSenderLocalChunk& operator=(const NetSenderLocalChunk& other);
};

// 一个本地通道的输入生命周期；反向接收器消失不等于本方向已交付EOF
enum NetSenderLocalPhase
{
    // 允许发送；仍可能处于TLS握手或等待首条数据
    NET_LOCAL_ACTIVE,
    // 反向接收器已失效，禁止再发送，有限等待本方向已提交的尾包和END
    NET_LOCAL_DRAINING,
    // 已收到本代有序END，只交付此前输入，不再补发ACK或心跳
    NET_LOCAL_ENDED,
    // 唯一终态已经确定，等待owner移除资源并通知监听者
    NET_LOCAL_CLOSED
};

// 单执行者拥有的NIP2通道，两种协议模式共用一套生命周期和额度
class NetSenderLocalPeer
{
public:
    /** 显式初始化全部标量和随机代次，不依赖隐式值初始化
    */
    NetSenderLocalPeer();

    /** 单向进入排空阶段，不因重复失效或重复END延长首次五秒总期限
    @param [in] now 单调毫秒，与所属poll同一时钟
    @param [in] end true表示已收到本代有序END；false仅表示反向传输已失效
    */
    void drain(uint64_t now, bool end);

public:
    // 实际ProcessWork发送PID，仅用于寻址，不作为业务授权
    int32_t m_pid;
    // 唯一终态原因，只在关闭时确定
    int32_t m_reason;
    // 是否已经向所属监听者报告就绪
    bool m_ready;
    // 唯一关闭阶段，替代可同时矛盾的closing/ended组合
    NetSenderLocalPhase m_phase;
    // 是否已绑定对端非零随机代次
    bool m_remoteKnown;
    // 本端16字节随机代次，绑定本实例输入
    uint8_t m_local[16];
    // 对端16字节随机代次，绑定本实例输出
    uint8_t m_remote[16];
    // 已消费但尚未交给反向IPC的累计确认号
    uint64_t m_ackPending;
    // 当前最早未确认批次开始或实际ACK推进的毫秒
    uint64_t m_ackTime;
    // 最后消费业务帧的单调毫秒
    uint64_t m_activity;
    // 首次进入DRAINING或ENDED的毫秒，不随新消息重置
    uint64_t m_drainSince;
    // 最近一次心跳提交的单调毫秒
    uint64_t m_pingTime;
    // 待回心跳号，0表示没有在途心跳
    uint64_t m_pingId;
    // 本代独占的四包发送及累计接收窗口
    NetSenderLocalWindow m_window;
    // 只绑定本代ProcessWork接收器的反向发送通道
    NetSenderIpcChannel m_sender;
    // TLS模式的独占协议连接，Raw模式为空
    std::unique_ptr<NetInterfaceConnection> m_connection;
    // 顺序拥有的最多四个完整IPC密文片
    std::deque<NetSenderLocalChunk> m_input;
    // Raw模式的独占NTW3解码器，TLS模式为空
    std::unique_ptr<NetInterfaceTunnel> m_rawDecoder;
    // Raw待发记录，正文已经复制，按入队顺序分片
    std::deque<NetSenderLocalChunk> m_rawOutput;
    // Raw队列中尚未交付IPC的字节总量
    uint32_t m_rawOutputBytes;
    // Raw监听者首次拒绝当前完整记录的毫秒，0表示未背压
    uint64_t m_rawBlockedSince;
};

// 本机通道协调器，借用收件箱和监听者，不拥有ProcessWork生命周期或线程
class NetSenderLocalState
{
public:
    /** 绑定收件箱和当前PID，尚未启用
    @param [in] inbox 借用对象
    */
    explicit NetSenderLocalState(NetSenderLocalInbox& inbox);

    /** 擦除复制的私钥，外层先shutdown收敛回调
    */
    ~NetSenderLocalState();

    /** 复制TLS配置并启用收件箱
    @param [in] tls 配置
    @param [in] listener 监听者
    @param [in] server 模式
    @param [in] maximum 通道上限
    @return OK或负数
    */
    int32_t configure(const NetInterfaceTlsOptions& tls, NetSenderLocalListener* listener, bool server, uint32_t maximum);

    /** 启用透明字节路径，与原TLS配置互斥
    @param [in] entropy 强随机
    @param [in] context 随机上下文
    @param [in] listener 原始记录监听
    @param [in] server 是否监听
    @param [in] maximum 本机代次数
    @return NI_OK或错误
    */
    int32_t configureRaw(NetInterfaceEntropyFunction entropy, void* context, NetSenderLocalRawListener* listener, bool server, uint32_t maximum);

    /** 推进原窗口、确认和原始记录，不解密
    @param [in] id 代次
    @param [in,out] peer 独占状态
    */
    void pumpRaw(uint64_t id, NetSenderLocalPeer& peer);

    /** 创建一个本地TLS代次及有界发送器
    @param [in] pid 对端PID
    @param [in] remote 服务器接收OPEN时的对端16字节；客户端可空
    @param [out] peer 新内部号
    @return OK或负数
    */
    int32_t createPeer(int32_t pid, const uint8_t* remote, uint64_t& peer);

    /** 验证本地信封并投递到正确代次，不采用PID作为授权
    @param [in,out] input 收件箱弹出的完整包；接纳数据后转移缓冲所有权，不在返回后借用
    */
    void accept(NetSenderLocalInput& input);

    /** 推进单个共享TLS连接及字节确认
    @param [in] id 本轮号
    @param [in,out] peer 独占状态
    */
    void pump(uint64_t id, NetSenderLocalPeer& peer);

    /** TLS路径只推进协议与真实输入，关闭和排空期限由共同pump维护
    @param [in] id 本轮号
    @param [in,out] peer 独占状态
    */
    void pumpTls(uint64_t id, NetSenderLocalPeer& peer);

    /** 根据未提交原因决定排空或关闭，不重发已提交字节
    @param [in] id 本轮号
    @param [in,out] peer 独占状态
    @param [in] result NetSenderIpc返回值，BUSY保持原顺序和状态
    */
    void sendFailure(uint64_t id, NetSenderLocalPeer& peer, int32_t result);

    /** 编码并尝试发布一个本地包，不阻塞
    @param [in,out] peer 通道
    @param [in] kind 包种类
    @param [in] number 包/ACK号
    @param [in] data 密文或空
    @param [in] size 密文字节
    @return NetSenderIpc结果
    */
    int32_t transmit(NetSenderLocalPeer& peer, uint32_t kind, uint64_t number, const uint8_t* data, uint32_t size);

    /** 标记关闭，不在回调中立即删除
    @param [in] id 本轮号
    @param [in] reason 原因
    */
    void close(uint64_t id, int32_t reason);

    /** 删除关闭对象后回报唯一终态
    */
    void collect();

    /** 单执行者有界推进
    @return 是否仍启用
    */
    bool poll();

    /** 停止，不重入正在执行的回调
    */
    void shutdown();

    /** 单调毫秒
    @return 当前时间
    */
    static uint64_t now();

public:
    // 借用到全部本地传输停止，不负责收件箱释放。
    NetSenderLocalInbox* m_inbox;
    // 借用TLS监听者，须活到shutdown完成，Raw模式为空
    NetSenderLocalListener* m_listener;
    // 借用Raw监听者，须活到shutdown完成，TLS模式为空
    NetSenderLocalRawListener* m_rawListener;
    // 本实例固定TLS参数，指针指向下列自有副本或已约定的宿主上下文
    NetInterfaceTlsOptions m_options;
    // 复制的受信任CA字节
    std::vector<uint8_t> m_authority;
    // 复制的本地服务证书字节
    std::vector<uint8_t> m_certificate;
    // 复制的本地服务私钥，析构时先擦除
    std::vector<uint8_t> m_privateKey;
    // 复制的TLS验证域名
    std::string m_hostname;
    // owner独占的活跃通道表，最多64条
    std::map<uint64_t, std::unique_ptr<NetSenderLocalPeer>> m_peers;
    // 实际ProcessWork发送PID，仅用于寻址，不作为业务授权
    int32_t m_pid;
    // 本次初始化验证过的并发通道上限
    uint32_t m_maximum;
    // 当前一秒窗口已接纳的OPEN数量
    uint32_t m_acceptCount;
    // 当前OPEN限速窗口起点毫秒
    uint64_t m_acceptWindow;
    // 已分配的最大本地通道号，不回绕复用
    uint64_t m_lastPeer;
    // 已分配的最大心跳号，不回绕复用
    uint64_t m_lastPing;
    // 本轮poll固定的单调毫秒快照
    uint64_t m_now;
    // 上一次有界进程存活扫描的毫秒
    uint64_t m_processCheckAt;
    // 配置成功后启用；同一实例运行期不允许重新配置
    bool m_started;
    // owner已开始停止，拒绝新输入和发送
    bool m_stopping;
    // owner执行中的重入保护，不用于跨线程同步
    bool m_polling;
};