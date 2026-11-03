#pragma once
#include <stdint.h>

// 所有回调由所属执行者的poll/shutdown交付；context借用至destroy成功
typedef void (*LibuvHostConnected)(void* context, uint64_t peer);
// 字节只借用到回调返回，每次1至16384字节
typedef void (*LibuvHostData)(void* context, uint64_t peer, const uint8_t* data, uint32_t size);
// 本地写完成释放发送预算，不表示对端已经消费
typedef void (*LibuvHostWritable)(void* context, uint64_t peer);
// peer本实例内不复用，终态最多一次，reason是底层错误或主动原因
typedef void (*LibuvHostClosed)(void* context, uint64_t peer, int32_t reason);