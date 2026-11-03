#pragma once
#include "NetInterface.h"
#include "NetInterfaceTls.h"

// 固定槽位只保存拥有的帧缓冲和队列链接；由连接统一回收，不单独分配节点。
struct NetInterfaceConnectionWrite
{
    // 本槽独占的编码帧；空槽为nullptr。
    uint8_t* m_data;
    // 实际编码字节数，计入对应队列预算。
    uint32_t m_size;
    // 已分配容量；超过4KiB的缓冲完成后立即释放。
    uint32_t m_capacity;
    // 所在队列或空闲链的下一槽；80表示链尾。
    uint32_t m_next;
    // 0为普通队列，1为控制队列。
    uint32_t m_queue;
};

// 单执行者拥有的TLS及NIF2帧驱动；不建线程或socket，不处理业务身份。
// 两种有界队列共用80个固定槽位；完整编码后提交，半帧不被控制消息打断。
class NetInterfaceAPI NetInterfaceConnection
{
public:
    /** 创建统一字节通道，TLS配置在构造时复制；不创建线程或socket
    @param [in] options TLS证书、域名、强随机与握手时限
    */
    explicit NetInterfaceConnection(const NetInterfaceTlsOptions& options);

    /** 本地可信转发可额外容纳32字节来源上下文；普通网络沿用单参数构造
    @param [in] options TLS配置
    @param [in] payloadLimit 1至1048608字节；不扩大发送队列总预算
    */
    NetInterfaceConnection(const NetInterfaceTlsOptions& options, uint32_t payloadLimit);

    /** 清除拥有的待发明文，并由成员析构释放TLS及消息
    */
    ~NetInterfaceConnection();

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

    /** 消费完整帧；余下明文由下次progress继续处理
    @return NI_OK或错误
    */
    int32_t consume();

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
    @return 消费字节数；解析错误存入m_error
    */
    uint32_t decodePending();

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

    /** 擦除并回收一个已编码缓冲，最多保留一块4KiB以内的小缓冲
    @param [in,out] write 槽内容；返回后不再拥有字节，空闲链由调用者维护
    */
    void recycle(NetInterfaceConnectionWrite& write);

    /** 退还活动帧预算，将槽位交还空闲链
    */
    void completeActive();

    /** 释放所有待发帧和缓存，重置空闲链；失败和析构均不分配
    */
    void clearWrites();

public:
    /** 设置公开RPC压缩白名单；只在单执行者内调用，不改变已排队帧
    @param [in] types count为0时可空，每个类型必须非零，本次调用复制
    @param [in] count 0至64；重复类型允许
    @param [in] negotiated 已完成压缩协商才允许收发压缩帧
    @return NI_OK或NI_INVALID_ARGUMENT；失败不改变此前白名单
    */
    int32_t configureCompression(const uint32_t* types, uint32_t count, bool negotiated);

private:
    // 固定槽位总数，也是各链的结束标记。
    static const uint32_t kWriteSlots = 80;
    // 独占TLS会话；不拥有外部网络。
    NetInterfaceTls m_tls;
    // 独占一条已接收帧，借用由consume结束。
    NetInterface m_decoder;
    // 帧层持续错误，0表示尚未失败。
    int32_t m_error;
    // 已接收认证关闭并验证帧完整性。
    bool m_closed;
    // 本轮解密明文及尚未解析的后缀。
    uint8_t m_plain[16384];
    // 明文有效字节数。
    uint32_t m_plainSize;
    // 明文已消费偏移。
    uint32_t m_plainOffset;
    // 正在提交TLS的槽位；kWriteSlots表示空。
    uint32_t m_active;
    // 活动帧已经交给TLS的前缀长度。
    uint32_t m_activeOffset;
    // 空闲槽位链头；不为排队节点分配内存。
    uint32_t m_free;
    // 普通和控制队列链头。
    uint32_t m_heads[2];
    // 普通和控制队列链尾。
    uint32_t m_tails[2];
    // 两种预算均包含尚未交完的活动帧。
    uint32_t m_queueBytes[2];
    // 两种条目数均包含活动帧。
    uint32_t m_queueCounts[2];
    // 本连接独占的80个固定元数据槽。
    NetInterfaceConnectionWrite m_writes[kWriteSlots];
    // 单块已擦除的小消息复用缓冲；不计作已排队帧。
    uint8_t* m_spare;
    // 复用容量不超过4096，无缓存时为0。
    uint32_t m_spareCapacity;
    // 固定压缩白名单，无动态容器或跨模块分配。
    uint32_t m_compressionTypes[64];
    // 已协商白名单项数；未协商时为0。
    uint32_t m_compressionCount;
};