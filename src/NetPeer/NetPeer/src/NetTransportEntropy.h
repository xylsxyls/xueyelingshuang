#pragma once
#include <stdint.h>

/** 通过已经登记的字节宿主读取强随机数据；宿主负责各平台的熵来源
@param [in] context 忽略，匹配NetInterface回调
@param [out] output size非0时不可空
@param [in] size 单次最多8192字节
@return 0完整成功，-1失败；有效长度内宿主失败时擦除输出，非法范围不访问输出
*/
int32_t NetTransportEntropy(void* context, uint8_t* output, uint32_t size);