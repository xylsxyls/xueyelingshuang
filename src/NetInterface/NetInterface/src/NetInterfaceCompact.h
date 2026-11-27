#pragma once
#include "NetInterfaceTypes.h"

// 紧凑帧头；完整头使用规范变长整数，连续DATA可借用调用者持有的已提交元数据
class NetInterfaceCompact
{
public:
    /** 编码完整紧凑帧，失败不写输出；不压缩正文
    @param [in] frame 元数据及正文借用到返回
    @param [out] output 可写区域，可以复用帧元数据的存储，须与正文分开，容量为0时可空
    @param [in] capacity 输出容量，字节
    @param [out] required 所需字节数；与frame、previous、正文或输出重叠时拒绝并保留原值，其他参数错误置0
    @param [in] previous 同方向已接纳的最近紧凑DATA元数据，可空；仅相同产品/类型和正32位编号增量时复用；短正文增量1用单字节头
    @return NI_OK、NI_BUFFER_TOO_SMALL或负数参数错误
    */
    static int32_t encode(const NetInterfaceFrame& frame, uint8_t* output, uint32_t capacity, uint32_t& required,
        const NetInterfaceFrame* previous = nullptr);

    /** 只解析帧头，完整合法后才输出字段；不分配正文
    @param [in] input 连续输入，size非0时不可空
    @param [in] size 可读字节数，可包含后续正文
    @param [out] frame 完整头元数据，payload为null；未完整时清零
    @param [out] headerSize 完整头长度；未完整时为0
    @param [in] previous 同方向最近完整紧凑DATA元数据，可空；增量头没有上下文时拒绝
    @return NI_READY、NI_NEED_MORE或NI_PROTOCOL_ERROR
    */
    static int32_t header(const uint8_t* input, uint32_t size, NetInterfaceFrame& frame, uint32_t& headerSize,
        const NetInterfaceFrame* previous = nullptr);

private:
    /** 写规范无符号32位变长整数
    @param [out] output 至少5字节可写空间
    @param [in] value 待编码整数
    @return 实际1至5字节长度
    */
    static uint32_t write(uint8_t* output, uint32_t value);

    /** 解析规范无符号32位整数，拒绝溢出及非最短编码
    @param [in] input 借用输入
    @param [in] size 输入长度
    @param [in,out] offset 当前偏移，调用者提供不大于size的值
    @param [out] value 仅NI_READY时有效
    @return NI_READY、NI_NEED_MORE或NI_PROTOCOL_ERROR
    */
    static int32_t read(const uint8_t* input, uint32_t size, uint32_t& offset, uint32_t& value);
};