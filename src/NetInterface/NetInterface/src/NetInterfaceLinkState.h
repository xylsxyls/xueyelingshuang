#pragma once
#include "NetInterfaceLink.h"
#include "NetInterfaceConnection.h"
#include <memory>
#include <string>
#include <vector>

// 单个输出槽独占一条完整业务密文，存储大小与记录载荷完全相同。
class NetInterfaceLinkWrite
{
public:
    /** 创建没有正文和记录的空槽
    */
    NetInterfaceLinkWrite();

    /** 擦除并释放尚未消费的密文正文
    */
    ~NetInterfaceLinkWrite();

public:
    // 已复制元数据，m_data不保存外部地址
    NetInterfaceTunnelRecord m_record;
    // 独占精确m_record.m_size字节，消费后交给状态的一份复用缓冲
    std::unique_ptr<uint8_t[]> m_bytes;
};

// 认证阶段由单个状态唯一拥有，不由多个已发送标记拼出隐式组合。
enum NetInterfaceLinkPhase
{
    // 尚未完成底层TLS；首次显式progress才建立总期限
    NET_LINK_TLS,
    // TLS客户端等待固定32字节新挑战
    NET_LINK_CHALLENGE,
    // TLS服务端已发挑战，等待唯一角色证明
    NET_LINK_PROOF,
    // 客户端已发双随机数证明，等待可信节点签名接受
    NET_LINK_ACCEPTANCE,
    // 管理身份验证成功；不等同于账号或业务授权
    NET_LINK_READY,
    // 原始运输结束，禁止新工作，只消费有限已接纳输入
    NET_LINK_DRAINING,
    // 保存首个负数错误，所有后续操作不复活
    NET_LINK_FAILED
};

/** 管理TLS认证和有界记录队列的唯一所有者，由同一执行者推进并擦除秘密
*/
class NetInterfaceLinkState
{
public:
    /** 创建空状态
    */
    NetInterfaceLinkState();

    /** 擦除身份私钥与随机挑战
    */
    ~NetInterfaceLinkState();

    /** 复制并校验固定根管理配置
    @param [in] options 配置，回调上下文借用至析构
    @return NI_NEED_MORE或错误
    */
    int32_t configure(const NetInterfaceLinkOptions& options);

    /** 保存第一个持续失败
    @param [in] error 负数错误
    @return 持续错误
    */
    int32_t fail(int32_t error);

    /** 由唯一认证阶段计算公开状态，不保存第二套就绪标记
    @return READY、NEED_MORE或首个持续错误
    */
    int32_t status() const;

    /** 编码客户端角色证明，绑定双侧随机数和全部声明
    @return NI_OK或错误
    */
    int32_t prove();

    /** 服务端验证角色、私钥/凭证持有证明后返回签名接受消息
    @param [in] frame 完整证明
    @return NI_OK或错误
    */
    int32_t verifyProof(const NetInterfaceFrame& frame);

    /** 客户端验证签名接受消息及服务首次注册证书
    @param [in] frame 完整接受消息
    @return NI_OK或错误
    */
    int32_t verifyAcceptance(const NetInterfaceFrame& frame);

    /** 构造一帧管理握手，不修改调用者数据
    @param [in] kind 帧类型
    @param [in] type 控制操作
    @param [in] body 声明字节
    @return NI_OK或错误
    */
    int32_t handshakeFrame(uint16_t kind, uint32_t type, const std::vector<uint8_t>& body);

    /** 推进状态机
    @param [in] now 单调毫秒
    @return NI_READY、NI_NEED_MORE或错误
    */
    int32_t progress(uint64_t now);

    /** 完整复制后才提交一条FIFO记录；失败不改变队首、数量或字节额度
    @param [in] record 已通过外层格式及认证状态校验的业务记录
    @return NI_OK、NI_LIMIT_EXCEEDED或NI_NO_MEMORY
    */
    int32_t queueRecord(const NetInterfaceTunnelRecord& record);

    /** 消费当前非空输出队首并保留一份清零的同尺寸复用缓冲
    */
    void consumeQueuedRecord();

public:
    // 复制的配置值；宿主随机/服务回调上下文借用至本实例析构
    NetInterfaceLinkOptions m_options;
    // 独占管理TLS及NIF2编解码，绝不承载业务明文
    std::unique_ptr<NetInterfaceConnection> m_control;
    // 本端已经校验的精确身份名称
    std::string m_name;
    // 本端公开叶证书副本，CLIENT为空
    std::string m_certificate;
    // 本端私钥副本，析构时安全擦除
    std::string m_privateKey;
    // 本端有界声明副本，参加证明签名或服务MAC
    std::vector<uint8_t> m_claims;
    // 通过证明验证后提交的对端精确名称
    std::string m_peerName;
    // 验证通过的对端公开叶证书，匿名客户端为空
    std::string m_peerCertificate;
    // 认证阶段完整复制的对端声明，不表示业务授权
    std::vector<uint8_t> m_peerClaims;
    // SERVICE客户端验证并持有的首次签发叶证书
    std::vector<uint8_t> m_grant;
    // 仅READY之后可读取的对端管理角色
    uint32_t m_peerRole;
    // 验证端的新挑战，证明和接受都必须精确绑定
    uint8_t m_nonce[32];
    // 证明端的新随机数，拒绝旧接受消息跨连接重放
    uint8_t m_clientNonce[32];
    // 最多一条已接纳的管理TLS外层正文，等待全部喂入
    std::vector<uint8_t> m_input;
    // 上述正文已经交给TLS的字节数
    uint32_t m_inputOffset;
    // 当前已接纳业务密文记录的元数据，数据指向自有正文
    NetInterfaceTunnelRecord m_received;
    // 当前业务密文正文，消费前不覆盖
    std::vector<uint8_t> m_receivedBytes;
    // 业务记录是否等待上层显式消费
    bool m_recordReady;
    // 固定32槽FIFO，由本状态单执行者拥有；总载荷上限64KiB
    NetInterfaceLinkWrite m_output[32];
    // 当前队首索引，消费后按32回绕
    uint32_t m_outputHead;
    // 已提交且未消费的槽数
    uint32_t m_outputCount;
    // 所有已提交正文的精确字节数
    uint32_t m_outputBytes;
    // 最多一份16KiB的已清零正文，仅在新载荷尺寸相等时复用
    std::unique_ptr<uint8_t[]> m_spare;
    // 复用缓冲的精确容量，空指针时为0
    uint32_t m_spareSize;
    // 当前待发借用的元数据快照，消费前不切换队首
    NetInterfaceTunnelRecord m_peeked;
    // 是否已有尚未消费的输出借用
    bool m_outputPeeked;
    // 认证/排空/失败的唯一阶段，外层HELLO发送接收与它独立
    NetInterfaceLinkPhase m_phase;
    // FAILED时唯一有效的首个负数结果
    int32_t m_error;
    // 首次显式progress是否发生，允许合法零时刻
    bool m_clockStarted;
    // 首次显式推进时间，管理握手总期限不刷新
    uint64_t m_started;
    // 最近一次合法单调时间，供无时钟参数的发送输出推进使用
    uint64_t m_lastTime;
    // 最近管理活动时刻，用于心跳空闲间隔
    uint64_t m_activity;
    // 未答心跳发送时刻，0表示没有未答心跳
    uint64_t m_pingTime;
    // 本端心跳编号高水位，不允许溢出或旧PONG复用
    uint64_t m_pingId;
    // 外层固定协商已接纳，重复HELLO持续拒绝
    bool m_receivedHello;
    // 外层HELLO已经被宿主完整接纳，仅消费时提交
    bool m_sentHello;
    // 首次排空的单调时刻；由DRAINING阶段区分合法0时刻
    uint64_t m_drainStarted;
    // TLS输入已结束但仍有已接纳业务记录时保留的终态
    int32_t m_drainResult;
};