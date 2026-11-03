#pragma once
#include <cstddef>

/** 同模块跨类使用的局部辅助，不保存可变业务状态
*/
class NetInterfaceCertificateHelper
{
public:
    /** 适配Mbed的size_t熵请求，禁止溢出或伪随机回退
    @param [in] context 本次借用的随机配置
    @param [out] output 密码学随机输出
    @param [in] size 字节数
    @return 0成功，否则失败
    */
    static int certificateEntropyBytes(void* context, unsigned char* output, size_t size);
};