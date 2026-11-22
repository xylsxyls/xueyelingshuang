#pragma once
#include <stdint.h>

/** 同步从宿主的密码学随机源获取字节
@param [in] context 宿主上下文，是否可空由回调约定；TLS 借用至实例销毁，其他接口借用至调用返回
@param [out] output 本次调用期间可写的缓冲区
@param [in] size 须完整写入的字节数
@return 完整填充返回 0，失败返回非零；调用方丢弃部分输出
*/
typedef int32_t (*CEncodeDecodeEntropyFunction)(void* context, uint8_t* output, uint32_t size);

// 一次密码运算期间借用的字节片段，按数组顺序连接，不复制正文。
struct CEncodeDecodeBytes
{
    // 借用至调用返回；长度为0时允许空指针。
    const uint8_t* m_bytes;
    // 本片段字节数，全部片段合计不得超过调用接口上限。
    uint32_t m_size;
};

// 通用密码运算结果，与TLS连接状态分别处理；不传播第三方错误值。
enum CEncodeDecodeResult
{
    // 操作完整成功。
    CENCODE_OK = 0,
    // 空指针、长度或参数范围不合法。
    CENCODE_ARGUMENT = -1,
    // 本库无法分配独占对象。
    CENCODE_MEMORY = -2,
    // 已完成、算法失败或当前状态不允许操作。
    CENCODE_STATE = -3,
    // 累积长度超过算法允许的上限。
    CENCODE_LIMIT = -4,
    // 调用方输出容量不足，缓冲及算法状态保持不变。
    CENCODE_BUFFER = -5
};