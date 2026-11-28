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

/** 分派一个 JS 文件帧，保留已验证的远端失败与本地错误之间的区别
@param [in] handle 当前文件流
@param [in] fields 六项 kind/product/id 低高/type/size
@param [in] payload 调用时借用的正文
@return NI_READY 表示已接收对端失败 ACK，其原因由文件流保存；其余同 fileFlowAccept
*/
extern "C" NET_PEER_C_EXPORT int32_t netPeerWasmFileFlowAccept(NetInterfaceFileFlowHandle handle,
    const uint32_t* fields, const uint8_t* payload);
#endif