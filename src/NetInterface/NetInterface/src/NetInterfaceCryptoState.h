#pragma once
#include "NetInterface.h"
#include "mbedtls/sha256.h"

// 私有流式摘要上下文，原生/Wasm同源；不向调用方泄漏第三方类型。
struct NetInterfaceHashOpaque
{
public:
    // 拥有的mbedtls增量SHA256状态，析构时释放并擦除
    mbedtls_sha256_context m_context;
    // 已提交的消息总字节数，用于阻止SHA256长度溢出
    uint64_t m_size;
    // 最近算法状态，负数错误持续保留
    int32_t m_status;
    // 已完成最终摘要，后续不能继续更新或再次终结
    bool m_finished;

public:
    /** 初始化SHA256算法
    */
    NetInterfaceHashOpaque();

    /** 擦除上下文
    */
    ~NetInterfaceHashOpaque();
};