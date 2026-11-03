#pragma once
#include "NetSenderHostC.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 同源构造文件控制请求，Wasm不手写二进制头
@param [in] fields 5项：command/chunk/window/total低高
@param [in] transfer 16字节非零幂等号或已有文件号
@param [in] token 32字节；OPEN全零，QUERY/CANCEL服务秘密
@param [in] hash 32字节；仅上传OPEN使用
@param [in] key 有界不透明逻辑键，OPEN必需
@param [in] keySize 0至512
@param [in] authorization 不透明业务授权，可空
@param [in] authorizationSize 0至2048
@param [out] output 可空探测
@param [in] capacity 容量
@param [out] required 不可空
@return NetInterfaceResult
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileControlEncode(const uint32_t* fields, const uint8_t* transfer, const uint8_t* token,
    const uint8_t* hash, const uint8_t* key, uint32_t keySize,
    const uint8_t* authorization, uint32_t authorizationSize, uint8_t* output,
    uint32_t capacity, uint32_t* required);

/** 解析文件成功响应
@param [in] bytes 160字节状态体
@param [in] size 精确长度
@param [out] info 不可空，失败清零
@return OK或负数
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileParseInfo(const uint8_t* bytes, uint32_t size, NetInterfaceFileInfo* info);

/** Wasm读取元数据，64位字段使用低/高32位
@param [in] bytes 160字节状态体
@param [in] size 长度
@param [out] fields 12项：direction/state/chunk/window/total低高/offset低高/stream低高/expires低高
@param [out] transfer 16字节
@param [out] token 32字节秘密
@param [out] hash 32字节整文件摘要
@return OK或负数，失败输出全部清零
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileInfoFields(const uint8_t* bytes, uint32_t size, uint32_t* fields, uint8_t* transfer,
    uint8_t* token, uint8_t* hash);

/** 创建独立纯协议流，不创建socket/文件/线程
@param [in] info 已授权并映射的元数据
@param [in] product 非零产品
@param [in] sender 1发送DATA，0接收DATA
@param [in] timeLow 单调毫秒低位
@param [in] timeHigh 单调毫秒高位
@param [in] timeoutMs 无进度时限1000至60000
@param [out] handle 不可空，失败清零
@return OK或负数
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowCreate(const NetInterfaceFileInfo* info, uint32_t product, uint32_t sender,
    uint32_t timeLow, uint32_t timeHigh, uint32_t timeoutMs,
    NetInterfaceFileFlowHandle* handle);

/** Wasm直接从状态体创建，宿主不维护C结构布局
@param [in] bytes 文件状态体
@param [in] size 固定160字节
@param [in] product 产品
@param [in] sender 1发端，0收端
@param [in] timeLow 单调时间低位
@param [in] timeHigh 高位
@param [in] timeoutMs 无进度时限
@param [out] handle 不可空，失败清零
@return OK或负数
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowCreateWire(const uint8_t* bytes, uint32_t size, uint32_t product, uint32_t sender,
    uint32_t timeLow, uint32_t timeHigh, uint32_t timeoutMs,
    NetInterfaceFileFlowHandle* handle);

/** 释放并擦除协议流，不操作宿主文件
@param [in] handle 句柄或null
*/
NET_PEER_C_EXPORT NetPeerAPI void netPeerFileFlowDestroy(NetInterfaceFileFlowHandle handle);

/** 可选接收端有界合并写，必须在第一块DATA之前设置，默认1
@param [in] handle 收端流
@param [in] chunks 不大于协商窗口，最多8；宿主支持最大chunks乘chunk字节单次写入
@return 同Flow.setWriteBatchChunks；只减少宿主调用，不提前ACK，不改变线上协议
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowSetWriteBatchChunks(NetInterfaceFileFlowHandle handle, uint32_t chunks);

/** 按顺序补恢复前缀
@param [in] handle 流
@param [in] offsetLow 前缀位置低位
@param [in] offsetHigh 高位
@param [in] bytes 文件前缀
@param [in] size 不超过剩余量且最多16MiB
@return 同Flow.prime
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowPrime(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
    const uint8_t* bytes, uint32_t size);

/** 驱动时限
@param [in] handle 流
@param [in] timeLow 单调时间低位
@param [in] timeHigh 高位
@return 同Flow.progress
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowProgress(NetInterfaceFileFlowHandle handle, uint32_t timeLow, uint32_t timeHigh);

/** 申请宿主读取
@param [in] handle 流
@param [out] fields 三项offset低/高位和精确字节数
@return READY读取参数，否则输出0
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowRead(NetInterfaceFileFlowHandle handle, uint32_t* fields);

/** 接纳异步读取结果并复制
@param [in] handle 流
@param [in] offsetLow 工作偏移低位
@param [in] offsetHigh 高位
@param [in] bytes 完整块
@param [in] size 精确长度
@return 同Flow.produced
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowProduced(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
    const uint8_t* bytes, uint32_t size);

/** 接纳DATA/ACK帧
@param [in] handle 流
@param [in] fields 六项kind/product/id低高/type/size
@param [in] payload fields[5]非零时不可空
@return 同Flow.accept
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowAccept(NetInterfaceFileFlowHandle handle, const uint32_t* fields, const uint8_t* payload);

/** 取一次持久写请求，宿主复制借用bytes后投递文件执行者
@param [in] handle 流
@param [out] fields 三项offset低/高位和长度
@param [out] bytes 借用至written/cancel/destroy
@return 同Flow.writeRequest，失败输出0/null
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowWrite(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** bytes);

/** 回报对应实际持久写结果
@param [in] handle 流
@param [in] offsetLow 工作偏移低位
@param [in] offsetHigh 高位
@param [in] size 精确长度
@param [in] result NI_OK持久化或负数
@return 同Flow.written
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowWritten(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
    uint32_t size, int32_t result);

/** 摘要通过后取一次提交任务
@param [in] handle 流
@return 同Flow.takeCommit
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowTakeCommit(NetInterfaceFileFlowHandle handle);

/** 实际文件提交结果，禁止入队时就报成功
@param [in] handle 流
@param [in] result NI_OK已提交，负数失败
@return 同Flow.committed
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowCommitted(NetInterfaceFileFlowHandle handle, int32_t result);

/** 借用输出；背压时不消费
@param [in] handle 流
@param [out] fields 六项kind/product/id低高/type/size
@param [out] payload 借用至consume/cancel/destroy
@return READY或其他结果，失败清输出
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowPeek(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** payload);

/** 下层完整接纳后消费
@param [in] handle 流
@return 同Flow.consumeOutput
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowConsume(NetInterfaceFileFlowHandle handle);

/** 取消本地流，宿主另外发文件CANCEL RPC并收敛文件工作
@param [in] handle 流
@param [in] reason 负数错误
@return OK或INVALID_ARGUMENT
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowCancel(NetInterfaceFileFlowHandle handle, int32_t reason);

/** 当前显示快照
@param [in] handle 流
@param [out] info 固定C布局，失败清零
@param [out] result 不可空
@return OK或INVALID_ARGUMENT
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowSnapshot(NetInterfaceFileFlowHandle handle, NetInterfaceFileInfo* info, int32_t* result);

/** Wasm从相同160字节状态读取进度，不依赖C结构体布局
@param [in] handle 流
@param [out] bytes 状态体，可空探测
@param [in] capacity 至少160
@param [out] result 流结果，不可空
@return 编码结果；秘密只供恢复，不向展示/日志输出
*/
NET_PEER_C_EXPORT NetPeerAPI int32_t netPeerFileFlowSnapshotWire(NetInterfaceFileFlowHandle handle, uint8_t* bytes, uint32_t capacity,
    int32_t* result);

#ifdef __cplusplus
}
#endif