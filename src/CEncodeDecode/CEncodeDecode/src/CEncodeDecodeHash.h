#pragma once
#include "CEncodeDecodeMbedConfig.h"
#include "CEncodeDecodeTypes.h"
#include <mbedtls/sha256.h>

// 仅本库实现可见的SHA256对象；调用方通过CEncodeDecode取得和释放，不暴露第三方ABI。
class CEncodeDecodeHash
{
public:
    /** 初始化一个独占算法上下文，失败状态由主类返回
    */
    CEncodeDecodeHash();

    /** 擦除并释放算法上下文
    */
    ~CEncodeDecodeHash();

public:
    // 独占增量状态，无共享密钥或进程级锁。
    mbedtls_sha256_context m_context;
    // 已提交字节数，限制SHA256位计数溢出。
    uint64_t m_size;
    // 持续算法错误，参数错误不写入此字段。
    int32_t m_status;
    // 完成后禁止追加和重复完成。
    bool m_finished;

private:
    /** 禁止复制第三方拥有状态
    @param [in] other 不允许复制的对象
    */
    CEncodeDecodeHash(const CEncodeDecodeHash& other);

    /** 禁止赋值导致重复释放
    @param [in] other 不允许复制的对象
    @return 不提供实现
    */
    CEncodeDecodeHash& operator=(const CEncodeDecodeHash& other);
};