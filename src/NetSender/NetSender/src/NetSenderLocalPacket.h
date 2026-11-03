#pragma once
#include "NetSenderMacro.h"
#include <stdint.h>

// ProcessWork单块至少容纳16384字节密文、56字节NIP2头和12字节IPC内部头。
static const int32_t kNetSenderLocalReceiveBytes = 16452;

// 只标识本地TLS字节通道的代次和传输额度，不代表账号或服务授权。
enum NetSenderLocalPacketKind
{
    // 请求建立本地运输代次，不能据此获得业务授权
    NET_LOCAL_OPEN = 1,
    // 携带一块已加密或透明转发的有序字节
    NET_LOCAL_DATA = 2,
    // 累计释放本方向运输窗口，不表示远端业务处理成功
    NET_LOCAL_ACK = 3,
    // 终止本地运输代次，不复用该代次的旧数据
    NET_LOCAL_END = 4
};

// NIP2本地运输包的借用视图，编解码不会转移输入所有权
struct NetSenderLocalPacketView
{
    // OPEN、DATA、ACK或END本地运输包类型，不代表业务消息类型
    uint32_t m_kind;
    // DATA序号或ACK累计序号；OPEN和END按对应格式要求填写
    uint64_t m_packet;
    // 发送方公开的16字节随机代次，不作为密钥
    uint8_t m_source[16];
    // 接收方公开的16字节随机代次，OPEN首次寻址使用全零
    uint8_t m_target[16];
    // 借用的密文字节首地址，解析结果只活到原输入缓冲失效
    const uint8_t* m_data;
    // 密文字节数，DATA最多16384，其他控制类型不携带正文
    uint32_t m_size;
};

// IPC只运输同一个NetInterfaceConnection的TLS密文，不能复制业务协议或加密实现。
class NetSenderAPI NetSenderLocalPacket
{
public:
    /** 编码NIP2固定56字节头和至多16KiB密文，不分配输出内存
    @param [in] packet 本地源/目标随机代次，OPEN目标全零，其他目标非零
    @param [out] output 与密文不重叠的调用者空间
    @param [in] capacity 容量
    @param [out] required 所需空间，错误参数为0
    @return NetInterface结果码
    */
    static int32_t encode(const NetSenderLocalPacketView& packet, uint8_t* output,
        uint32_t capacity, uint32_t& required);

    /** 解析完整ProcessWork消息，不接纳拼接尾部或声明长度不符
    @param [in] bytes 完整本地包
    @param [in] size 消息体字节数
    @param [out] packet 成功借用密文，失败清零
    @return OK或PROTOCOL_ERROR
    */
    static int32_t parse(const uint8_t* bytes, uint32_t size, NetSenderLocalPacketView& packet);

    /** 比较公开随机代次，不用于比较密钥或口令
    @param [in] left 16字节
    @param [in] right 16字节
    @return 是否相同，空参数false
    */
    static bool sameGeneration(const uint8_t* left, const uint8_t* right);
};

// 每方向固定4个未确认16KiB包；ACK只释放字节额度，不能作为业务成功回执。
class NetSenderAPI NetSenderLocalWindow
{
public:
    /** 清零本连接独占计数
    */
    NetSenderLocalWindow();

    /** 查看待发布编号，不消耗额度
    @return 窗口满或耗尽为0，否则不复用的下一编号
    */
    uint64_t next() const;

    /** 仅在IPC已接纳后提交编号，背压不得调用
    @param [in] packet next给出的编号
    @return OK或INVALID_STATE
    */
    int32_t published(uint64_t packet);

    /** 接收真实本地通道ACK，拒绝未发布编号；重复确认无副作用
    @param [in] packet 累计确认编号
    @return OK或PROTOCOL_ERROR
    */
    int32_t acknowledge(uint64_t packet);

    /** 整个密文片已被TLS实例拥有后记录接收，禁止跳号
    @param [in] packet 完整密文片编号
    @return OK新接收，NOT_FOUND重复，PROTOCOL_ERROR跳号/零号
    */
    int32_t received(uint64_t packet);

    /** 已完整接收的累计编号
    @return 0尚未收到
    */
    uint64_t receipt() const;

    /** 当前未确认包数
    @return 0至4
    */
    uint32_t outstanding() const;

private:
    // 本方向最后已提交IPC的数据包序号
    uint64_t m_sent;
    // 对端累计确认的最高序号，不能超过m_sent
    uint64_t m_acknowledged;
    // 已完整交给本端协议输入的连续序号，重复包不重复消费
    uint64_t m_received;
};