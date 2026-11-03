#pragma once
#include <stdint.h>

/** 读取本机密码学随机源；Linux/Android熵未就绪直接失败，不无限阻塞
@param [in] context 忽略，匹配NetInterface回调
@param [out] output size非0时不可空
@param [in] size 单次最多8192字节
@return 0完整成功，-1失败；失败输出擦除，不做弱随机回退
*/
int32_t NetTransportEntropy(void* context, uint8_t* output, uint32_t size);