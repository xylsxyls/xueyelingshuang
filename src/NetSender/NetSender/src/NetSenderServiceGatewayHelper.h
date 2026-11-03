#pragma once
#include <cstdint>

/** 同模块跨类使用的局部辅助，不保存可变业务状态
*/
class NetSenderServiceGatewayHelper
{
public:
    /** 服务业务TLS仅使用系统强随机
    @param [in] context 忽略
    @param [out] output 输出
    @param [in] size 字节数
    @return 0成功，否则失败
    */
    static int32_t serviceGatewayEntropy(void* context, uint8_t* output, uint32_t size);

    /** 本执行者统一单调毫秒
    @return 单调时间
    */
    static uint64_t serviceGatewayNow();
};