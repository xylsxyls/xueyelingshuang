#pragma once
#include "NetConnectionPoolConfig.h"
#include "CEncodeDecode/CEncodeDecodeAPI.h"
#include "NetInterfaceLinkTypes.h"
#include "NetInterfaceTunnelTypes.h"
#include "NetInterfaceTypes.h"

class NetInterfaceLinkState;
// 每条链路的业务输出与管理 TLS 独立配置、分配和释放
struct NetInterfaceAPI NetInterfaceLinkPools
{
public:
    // 空闲至发送管理 PING 的毫秒数，必须大于零
    uint32_t m_pingIntervalMs;
    // 发出 PING 后等待对应 PONG 的毫秒数，必须大于零
    uint32_t m_pongTimeoutMs;
    // EOF 后允许上层消费已接纳数据的毫秒数，必须大于零
    uint32_t m_drainTimeoutMs;
    // 每次progress最多处理的管理帧数，默认64，至少1
    uint32_t m_controlStepBudget;
    // 业务密文按大小分级的正文池
    NetBufferPoolConfig m_outputBodies;
    // 每条待输出业务记录占一个槽
    LockFreeHashMapPoolConfig m_outputRecords;
    // 管理 TLS 自己的控制、数据记录及两套正文池
    NetConnectionPoolConfig m_controlConnection;
    // 仅管理 TLS 的输入、输出密文字节预算，与业务 TLS 独立
    CEncodeDecodeTlsQueueLimits m_controlCiphertext;

public:
    /** 使用各组件的默认配置，业务记录初始128槽
    */
    NetInterfaceLinkPools();
};

// 管理TLS和NTW3组合，无socket/IPC/线程；业务TLS在各逻辑通道端点单独持有。
class NetInterfaceAPI NetInterfaceLink
{
public:
    /** 复制配置，始终使用编译内置根；创建结果由status读取
    @param [in] options v3配置、强随机及可选服务端专用回调
    */
    explicit NetInterfaceLink(const NetInterfaceLinkOptions& options);

    /** 用指定的管理连接和业务输出池创建链路，结果由 status 返回
    @param [in] options 身份、角色及宿主回调
    @param [in] pools 当前实例各物理池配置
    */
    NetInterfaceLink(const NetInterfaceLinkOptions& options, const NetInterfaceLinkPools& pools);

    /** 在拥有状态的本模块释放并擦除身份私钥副本
    */
    ~NetInterfaceLink();

    /** 首次progress/accept/peekOutput前设置业务密文输出的实际池；失败保留原配置
    @param [in] bodies 正文大小类及实际槽预算，复制配置不长期借用
    @param [in] records 实际记录池配置，不再另加固定条数或字节门槛
    @return NI_OK或参数/内存/已启动错误；生命周期owner串行调用
    */
    int32_t configureOutputPools(const NetBufferPoolConfig& bodies, const LockFreeHashMapPoolConfig& records);

    /** 接纳一条完整外层记录，无空间时无副作用拒绝
    @param [in] record 调用期间借用，接纳后由本实例持有副本
    @return NI_OK接纳、NI_LIMIT_EXCEEDED背压、其他负数持续错误
    */
    int32_t accept(const NetInterfaceTunnelRecord& record);

    /** 推进管理认证、固定总期限和心跳，不执行业务或宿主回调等待
    @param [in] now 单调毫秒，不得倒退；无输入也须推进
    @return NI_READY管理就绪、NI_NEED_MORE或持续错误
    */
    int32_t progress(uint64_t now);

    /** 借用下一条待发记录，管理与业务密文各自保持顺序
    首次progress前只提供外层HELLO，不隐式以零时刻启动TLS超时；发送方仍须主动推进
    @param [out] record 借用至consumeOutput/destroy
    @return NI_READY、NI_NEED_MORE或错误
    */
    int32_t peekOutput(NetInterfaceTunnelRecord& record);

    /** 下层已拥有整条记录后消费，背压时不得调用
    @return NI_OK或错误
    */
    int32_t consumeOutput();

    /** 管理就绪后排队业务密文或关闭，仅复制外层，不解业务TLS
    @param [in] record BYTES/CLOSE、非零通道号，宿主负责已授权的路由归属
    @return NI_OK、NI_LIMIT_EXCEEDED未接纳可重试或错误
    */
    int32_t sendRecord(const NetInterfaceTunnelRecord& record);

    /** 借用已收到的业务密文/关闭记录；不会因管理认证而授权任意业务通道
    @param [out] record 借用至consumeRecord
    @return NI_READY、NI_NEED_MORE或错误
    */
    int32_t peekRecord(NetInterfaceTunnelRecord& record) const;

    /** 宿主接纳或拒绝当前业务记录后消费
    @return NI_OK或错误
    */
    int32_t consumeRecord();

    /** 排队认证管理消息，不能伪造内部握手或心跳
    @param [in] frame REQUEST/RESPONSE/EVENT/CANCEL，产品号必须0，消息体至多16KiB
    @return NI_OK、NI_LIMIT_EXCEEDED或错误
    */
    int32_t sendControl(const NetInterfaceFrame& frame);

    /** 借用管理消息，宿主仍需按已认证角色校验每项操作
    @param [out] frame 借用至consumeControl
    @return NI_READY、NI_NEED_MORE或错误
    */
    int32_t peekControl(NetInterfaceFrame& frame) const;

    /** 消费当前管理消息
    @return NI_OK或错误
    */
    int32_t consumeControl();

    /** 获取已经验证的管理身份；CLIENT不代表注册用户
    @param [out] peer 字符串/声明借用至析构
    @return NI_READY或当前状态
    */
    int32_t peerInfo(NetInterfaceLinkPeer& peer) const;

    /** 借用服务首次注册的签发结果，私钥不会通过网络返回
    @param [out] bytes 公共叶证书借用至析构
    @param [out] size 不含NUL字节数
    @return NI_READY就绪、NI_NEED_MORE或错误
    */
    int32_t serviceGrant(const uint8_t*& bytes, uint32_t& size) const;

    /** TCP/IPC结束，未完成认证关闭时不得当作业务成功
    @return 负数终态
    */
    int32_t finishInput();

    /** 原始运输已结束后有界排空已认证输入；仅同一执行者调用
    第一次调用禁止接纳/输出新记录；随后仍按peek/consume交付完整管理帧和业务密文
    未完成认证立即失败；首次排空起五秒内必须消费完，不因重复调用延期
    @param [in] nowMs 单调毫秒，不得小于上次progress时间
    @return NI_READY仍需消费/推进；负数为唯一终态，裸运输结束仍为TRUNCATED而非业务成功
    */
    int32_t drainInput(uint64_t nowMs);

    /** 认证状态；READY仅是管理身份，不代表业务TLS/账号/VIP
    @return NI_READY、NI_NEED_MORE或持续错误
    */
    int32_t status() const;

public:
    /** 查询本层待发数据是否全部交给下层，含创建后尚未交付的外层 HELLO。只由协议执行者调用
    @return NI_OK 本层已排空，NI_READY 有待发记录，负数表示错误。此状态不表示远端收到
    */
    int32_t outputState() const;

private:
    /** 禁止复制持有活动协议状态的对象
    @param [in] other 不可复制的实例
    */
    NetInterfaceLink(const NetInterfaceLink& other);

    /** 禁止赋值持有活动协议状态的对象
    @param [in] other 不可赋值的实例
    @return 不可调用
    */
    NetInterfaceLink& operator=(const NetInterfaceLink& other);
    // 本模块分配的独占管理协议状态，析构时在本模块释放
    NetInterfaceLinkState* m_state;
};