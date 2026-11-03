#pragma once
#include "NetSenderC.h"

#if defined(__EMSCRIPTEN__)
/** 保留既有Wasm导出名，统一转交NetSender的备用网关配置
@param [in] handle 本模块C接口创建的有效客户端，回调内拒绝
@param [in] seeds 可空清除，调用时复制，最多4096字节
@param [in] cachePath 可空禁用，调用时复制，最多4096字节
@return NI_OK或参数、生命周期错误
*/
extern "C" NET_PEER_C_EXPORT int32_t netPeerWasmConfigureGateways(NetSenderClientHandle handle,
    const char* seeds, const char* cachePath);
#endif