#pragma once
#include "NetChannelMacro.h"
#include "NetChannelTypes.h"

// 单执行者NTW3记录编解码器；独占最多16KiB正文存储，借用到consume或析构
// 不拥有网络、线程、TLS、业务授权或路由状态；每次只接纳一条完整记录
class NetChannelAPI NetChannel
{
public:
    /** 创建未接收任何字节的解码器，不分配正文存储
    */
    NetChannel();

    /** 在创建模块释放独占存储，调用者须先结束全部借用
    */
    ~NetChannel();

    /** 单执行者增量接纳一条记录，完整记录未消费时不读取新输入
    @param [in] data 至少size字节的有效输入，size为0时可空
    @param [in] size 本次输入字节数，未接纳的后缀仍归调用者
    @param [out] consumed 本次完整复制的字节数，参数拒绝时为0
    @return NC_READY、NC_NEED_MORE或错误；协议、截断及分配错误持续到析构
    */
    int32_t feed(const uint8_t* data, uint32_t size, uint32_t& consumed);

    /** 单执行者借用完整记录；不复制正文、不改变状态
    @param [out] record READY时借用到consume或析构，其余状态清零
    @return 当前READY、NEED_MORE或持续错误
    */
    int32_t peek(NetChannelRecord& record) const;

    /** 单执行者结束当前借用并准备下一条记录，保留有界存储供复用
    @return NC_OK、NC_INVALID_STATE或持续错误
    */
    int32_t consume();

    /** 单执行者报告输入结束，未完成的固定头或正文形成持续错误
    @return 边界完整为NC_OK，待消费记录为NC_READY，否则返回持续错误
    */
    int32_t finish();

    /** 独立编码一条记录；可由多线程使用互不重叠的输出并发调用
    @param [in] record 合法字段及借用正文，正文不得与实际输出区重叠
    @param [out] output 至少capacity字节的输出，查询长度时可空
    @param [in] capacity 可写字节数，0表示只查询所需长度
    @param [out] required 所需字节数，非法输入时为0；不得与输入输出别名
    @return NC_OK、NC_BUFFER_TOO_SMALL或NC_INVALID_ARGUMENT；失败不写输出
    */
    static int32_t encode(const NetChannelRecord& record, uint8_t* output, uint32_t capacity, uint32_t& required);

private:
    /** 验证独立于正文内容的字段与记录类型约束
    @param [in] record 待验证视图，正文地址由调用入口另行检查
    @return 字段满足固定线格式时为true
    */
    static bool valid(const NetChannelRecord& record);

    /** 读取固定宽度大端整数，不访问宽度之外的内存
    @param [in] bytes 至少width字节的已验证输入
    @param [in] width 1至8字节，仅由本类固定偏移调用
    @return 无符号整数，不按本机端序解释
    */
    static uint64_t readNumber(const uint8_t* bytes, uint32_t width);

    /** 写入固定宽度大端整数
    @param [out] bytes 至少width字节的已验证输出
    @param [in] value 对应字段的无符号值
    @param [in] width 1至8字节，仅由本类固定偏移调用
    */
    static void writeNumber(uint8_t* bytes, uint64_t value, uint32_t width);

    /** 在完整固定头上校验字段，再准备有限正文存储
    @return NC_NEED_MORE或持续错误；失败不会接受任何正文
    */
    int32_t acceptHeader();

    /** 禁止复制独占存储与正在借用的记录
    @param [in] other 原解码器
    */
    NetChannel(const NetChannel& other);

    /** 禁止复制赋值独占存储
    @param [in] other 原解码器
    @return 不可调用
    */
    NetChannel& operator=(const NetChannel& other);

private:
    // 固定头仅在全部24字节收到后解释，无需预先清零
    uint8_t m_header[NET_CHANNEL_HEADER_SIZE];
    // 已接纳固定头字节数，范围0至24
    uint32_t m_headerBytes;
    // 本条已复制正文数，不超过m_record.m_size
    uint32_t m_bodyBytes;
    // 独占未初始化字节存储，只有已复制区间对外可读
    uint8_t* m_storage;
    // 存储容量，不超过NET_CHANNEL_PAYLOAD_LIMIT
    uint32_t m_capacity;
    // NEED_MORE、READY或第一个持续错误
    int32_t m_state;
    // 已验证字段及完整正文借用视图
    NetChannelRecord m_record;
};