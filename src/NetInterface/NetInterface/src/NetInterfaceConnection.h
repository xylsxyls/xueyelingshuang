#pragma once
#include "NetInterface.h"
#include "NetInterfaceTls.h"

#include "NetConnectionPoolConfig.h"
#include <memory>
#include <vector>

class NetConnectionQueue;

// 单执行者拥有的TLS及NIF2帧驱动；不建线程或socket，不处理业务身份。
// 数据和控制分别占用可增长的记录池及正文池；完整编码后提交，半帧不被打断。
class NetInterfaceAPI NetInterfaceConnection
{
public:
    /** 创建统一字节通道，TLS配置在构造时复制；不创建线程或socket
    @param [in] options TLS证书、域名、强随机与握手时限
    */
    explicit NetInterfaceConnection(const NetInterfaceTlsOptions& options);

    /** 本地可信转发可额外容纳32字节来源上下文；普通网络沿用单参数构造
    @param [in] options TLS配置
    @param [in] payloadLimit 1至1048608字节；协议合法性限制，与物理池容量无关
    */
    NetInterfaceConnection(const NetInterfaceTlsOptions& options, uint32_t payloadLimit);

    /** 清除拥有的待发明文，并由成员析构释放TLS及消息
    */
    ~NetInterfaceConnection();

    /** 在首次feed/progress/send之前设置物理池；单执行者独占，失败保留原配置
    @param [in] config 实际记录和正文池的预分配、增长及容量配置
    @return NI_OK或状态、参数、内存错误
    */
    int32_t configurePools(const NetConnectionPoolConfig& config);

    /** 提交网络密文，未接纳后缀仍由宿主保管
    @param [in] data size为0时可空
    @param [in] size 字节数
    @param [out] consumed 实际接纳字节数
    @return TLS状态或负数持续错误
    */
    int32_t feed(const uint8_t* data, uint32_t size, uint32_t& consumed);

    /** 有界推进握手、帧编码及解码，不调用宿主业务回调
    @param [in] monotonicMs 宿主单调毫秒，必须持续检查握手期限
    @return TLS状态或帧/TLS持续错误；READY仅表示TLS已验证
    */
    int32_t progress(uint64_t monotonicMs);

    /** 排队完整帧，内部复制；控制帧有独立容量和优先级但不打断半个帧
    @param [in] frame 消息体不超过创建时限额，输入只在调用期间借用
    @return NI_OK接纳；NI_LIMIT_EXCEEDED表示背压，其他负数为错误
    */
    int32_t send(const NetInterfaceFrame& frame);

    /** 查看经过TLS验证且完整的一条帧，未消费时不继续读明文
    @param [out] frame 借用至consume或析构
    @return NI_READY、有持续错误或NI_NEED_MORE
    */
    int32_t peek(NetInterfaceFrame& frame) const;

    /** 只读借用完整帧及正文，不复制逐帧元数据；由本连接执行者串行使用
    @param [out] frame READY时借用至consume或析构，其余状态为nullptr
    @return NI_READY、NI_NEED_MORE或持续错误；借用期间不得消费或销毁连接
    */
    int32_t peek(const NetInterfaceFrame*& frame) const;

    /** 消费完整帧；余下明文由下次progress继续处理
    @return NI_OK或错误
    */
    int32_t consume();

    /** 仅解析本连接已认证的明文后缀，不推进TLS或宿主I/O；须由同一owner在consume之后调用
    @return NI_READY取得下一帧、NI_NEED_MORE需progress，或负数解析错误
    @note 批量接收仍须定期progress及输出推进，保持控制帧、截止和其他连接的公平性
    */
    int32_t decodeBuffered();

    /** 查看连续待发密文，宿主复制给网络后才能consumeOutput
    @param [out] data 借用字节
    @param [out] size 字节数
    @return TLS状态或错误
    */
    int32_t peekOutput(const uint8_t*& data, uint32_t& size) const;

    /** 消费宿主已拥有或已发送的密文
    @param [in] size 不超过peekOutput连续长度
    @return TLS状态或错误
    */
    int32_t consumeOutput(uint32_t size);

    /** 标记TCP EOF；后续progress验证TLS关闭及剩余帧完整性
    @return TLS状态或错误
    */
    int32_t finishInput();

    /** 当前TLS状态或持续帧错误
    @return NetInterfaceTlsStatus或NetInterfaceResult负数
    */
    int32_t status() const;

    /** 最近TLS底层错误，0为无底层错误
    @return Mbed TLS诊断码
    */
    int32_t detail() const;

public:
    /** 查询本层待发数据是否全部交给下层；只由协议执行者调用
    @return OK全部交付，READY仍有本层待发数据，负数失败；不表示远端收到
    */
    int32_t outputState() const;

private:
    /** 禁止复制连接所有权
    @param [in] other 不可复制实例
    */
    NetInterfaceConnection(const NetInterfaceConnection& other);

    /** 禁止赋值连接所有权
    @param [in] other 不可复制实例
    @return 不可调用
    */
    NetInterfaceConnection& operator=(const NetInterfaceConnection& other);

    /** 消费当前已解密的明文前缀，到完整帧时停止
    @param [out] consumed 本次消费字节数，当前帧尚未消费或没有明文时为0
    @return 本次解析状态；错误同时保存到m_error，不重复查询解码器
    */
    int32_t decodePending(uint32_t& consumed);

    /** 记录持续错误，后续操作不能恢复当前连接
    @param [in] error 负数错误
    @return error
    */
    int32_t fail(int32_t error);

    /** 擦除明文；不读未初始化的容量尾部
    @param [out] bytes size为0时可空，其他情况须有size字节可写内存
    @param [in] size 字节数
    */
    static void wipe(uint8_t* bytes, uint32_t size);

    /** 归还已完整交给TLS的活动帧及正文租约
    */
    void completeActive();

    /** 归还所有待发帧；保留池内存供复用，失败和析构均不分配
    */
    void clearWrites();

public:
    /** 设置公开RPC压缩白名单；只在单执行者内调用，不改变已排队帧
    @param [in] types count为0时可空，每个类型必须非零，本次调用复制
    @param [in] count 非负项数；重复类型允许
    @param [in] negotiated 已完成压缩协商才允许收发压缩帧
    @return NI_OK或NI_INVALID_ARGUMENT；失败不改变此前白名单
    */
    int32_t configureCompression(const uint32_t* types, uint32_t count, bool negotiated);

    /** 协商完成后由owner启用紧凑帧，不重编码已排队的帧
    @param [in] enabled 对端已经明确同意紧凑格式
    */
    void configureCompact(bool enabled);

private:
    // 当前新发帧是否使用协商后的紧凑头
    bool m_compact;
    // 同方向已接纳的最近紧凑DATA元数据，失败不推进，不保存正文指针
    NetInterfaceFrame m_previousCompactData;
    // 独占TLS会话；不拥有外部网络。
    NetInterfaceTls m_tls;
    // 独占一条已接收帧，借用由consume结束。
    NetInterface m_decoder;
    // 帧层持续错误，0表示尚未失败。
    int32_t m_error;
    // 已接收认证关闭并验证帧完整性。
    bool m_closed;
    // TLS读取等待宿主I/O；收到输入、输出容量归还或EOF后再尝试，不跨owner共享。
    bool m_readBlocked;
    // 本轮解密明文及尚未解析的后缀。
    uint8_t m_plain[16384];
    // 明文有效字节数。
    uint32_t m_plainSize;
    // 明文已消费偏移。
    uint32_t m_plainOffset;
    // 实际物理池和两级FIFO，所有权覆盖部分发送和失败回收。
    std::unique_ptr<NetConnectionQueue> m_queue;
    // 活动帧已经交给TLS的前缀长度。
    uint32_t m_activeOffset;
    // 首次驱动后冻结池配置，避免替换仍被TLS借用的帧存储。
    bool m_started;
    // 本库拥有排序去重后的白名单，查询二分查找；调用方不持有容器内存。
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::vector<uint32_t> m_compressionTypes;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};