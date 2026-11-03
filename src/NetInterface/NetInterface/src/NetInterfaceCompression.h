#pragma once
#include "NetInterfaceTypes.h"

/** NIF2应用压缩扩展；TLS本身不压缩，仅显式白名单的公开RPC可以启用。
*/
class NetInterfaceAPI NetInterfaceCompression
{
public:
    /** 小于1KiB或不足8%收益时保留原帧，永不压缩HELLO/控制/文件数据
    @param [in] frame 待编码RPC帧，函数不保存正文借用
    @param [out] output 调用方缓冲；可为空以查询所需容量
    @param [in] capacity output可写字节数，output为空时为0
    @param [out] required 实际需要的总帧长度，失败按编码结果填写
    @return NI_OK编码完成，NI_BUFFER_TOO_SMALL需更大输出，或其他NI错误
    */
    static int32_t encode(const NetInterfaceFrame& frame, uint8_t* output, uint32_t capacity, uint32_t& required);

    /** 先校验声明的原长，再有界解压；输出仅由同模块delete[]释放
    @param [in] input 本次同步调用期间借用的NZC1压缩载荷
    @param [in] size 输入总字节数，至少9
    @param [in] limit 允许的最大解压字节数，先检查再分配
    @param [out] output 成功分配的字节数组，须由同模块delete[]释放；失败置空
    @param [out] length 成功解压长度；失败为0
    @return NI_OK成功，或协议、长度、内存错误；失败不交付部分输出
    */
    static int32_t decode(const uint8_t* input, uint32_t size, uint32_t limit, uint8_t*& output, uint32_t& length);
};