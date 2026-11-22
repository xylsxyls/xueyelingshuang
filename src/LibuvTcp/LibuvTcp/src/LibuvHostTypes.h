#pragma once
#include <stdint.h>

// 所有回调由所属执行者的poll/shutdown交付；context借用至destroy成功
typedef void (*LibuvHostConnected)(void* context, uint64_t peer);
// 字节只借用到回调返回，每次不超过配置的读缓冲大小，默认16384字节
typedef void (*LibuvHostData)(void* context, uint64_t peer, const uint8_t* data, uint32_t size);
// 在所属循环提示该连接可以重试，共享池容量也可由其他连接释放
typedef void (*LibuvHostWritable)(void* context, uint64_t peer);
// peer本实例内不复用，终态最多一次，reason是底层错误或主动原因
typedef void (*LibuvHostClosed)(void* context, uint64_t peer, int32_t reason);