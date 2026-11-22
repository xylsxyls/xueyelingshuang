#pragma once
#if defined(__EMSCRIPTEN__)
#include "NetInterface/NetInterfaceAPI.h"
#include <emscripten/emscripten.h>

// Test-only exports; never part of a production SDK or business integration.
extern "C"
{

/** 握手共同确认位8后启用有界解压；仅白名单类型会自动尝试压缩，默认全部关闭。 */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceConnectionConfigureCompression(NetInterfaceConnectionHandle handle,
        const uint32_t* types, uint32_t count, int32_t negotiated);

/** 消费peek后的完整帧，允许下次progress读取后续帧
    @param [in] handle 有效连接
    @return NI_OK或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceConnectionConsume(NetInterfaceConnectionHandle handle);

/** 消费已由宿主复制或发送的密文
    @param [in] handle 有效连接
    @param [in] size 不超过peekOutput长度
    @return TLS状态或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceConnectionConsumeOutput(NetInterfaceConnectionHandle handle, uint32_t size);

/** 释放连接和拥有的队列
    @param [in] handle 实例或null，禁止重复释放
    */
EMSCRIPTEN_KEEPALIVE void netTestInterfaceConnectionDestroy(NetInterfaceConnectionHandle handle);

/** 提交密文，不接纳的后缀仍由宿主拥有
    @param [in] handle 有效连接
    @param [in] data size为0时可空
    @param [in] size 字节数
    @param [out] consumed 不可空，已复制长度
    @return TLS状态或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceConnectionFeed(NetInterfaceConnectionHandle handle, const uint8_t* data, uint32_t size,
                                                       uint32_t* consumed);

/** 标记TCP结束，后续progress检查TLS关闭及帧是否完整
    @param [in] handle 有效连接
    @return TLS状态或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceConnectionFinishInput(NetInterfaceConnectionHandle handle);

/** 查询持续状态与诊断
    @param [in] handle 有效连接
    @param [out] detail 可空，TLS诊断
    @return TLS状态或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceConnectionGetStatus(NetInterfaceConnectionHandle handle, int32_t* detail);

/** 借用一条完整、经过TLS验证的帧
    @param [in] handle 有效连接
    @param [out] fields 不可空，六项与Send相同
    @param [out] payload 不可空，借用至consume或destroy
    @return NI_READY、NI_NEED_MORE或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceConnectionPeek(NetInterfaceConnectionHandle handle, uint32_t* fields, const uint8_t** payload);

/** 借用连续密文输出
    @param [in] handle 有效连接
    @param [out] data 不可空，消费前有效
    @param [out] size 不可空，连续长度
    @return TLS状态或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceConnectionPeekOutput(NetInterfaceConnectionHandle handle, const uint8_t** data, uint32_t* size);

/** 有界驱动握手、加密和完整帧读取
    @param [in] handle 有效连接
    @param [in] timeLow 单调毫秒低32位
    @param [in] timeHigh 单调毫秒高32位
    @return TLS状态或持续错误；READY只表示TLS完成
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceConnectionProgress(NetInterfaceConnectionHandle handle, uint32_t timeLow, uint32_t timeHigh);

/** 排队完整帧；普通与控制使用独立的可配置物理池，池满时报告背压
    @param [in] handle 有效且TLS就绪的连接
    @param [in] fields 六项kind/product/request低位/高位/type/payloadSize
    @param [in] payload 字节，当fields[5]为0时可空
    @return NI_OK接纳、NI_LIMIT_EXCEEDED背压或其他错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceConnectionSend(NetInterfaceConnectionHandle handle, const uint32_t* fields, const uint8_t* payload);

/** 从已认证的管理OPEN回应读取窗口字段并应用，缺字段采用64KiB
    @param [in] handle 尚未使用的有效窗口句柄，不可空
    @param [in] input 仅在调用中借用的ProtoMessage字节，不可空；本函数不负责认证来源
    @param [in] size 输入1至16384字节
    @return NI_OK；参数、协议、窗口状态或内存不足返回对应负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceCreditConfigureMessage(NetInterfaceCreditHandle handle, const uint8_t* input, uint32_t size);

/** 管理层拥有归还消息后消费，不能在背压时调用
    @param [in] handle 窗口
    @param [in] size 已交付管理层的额度
    @return NI_OK或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceCreditConsume(NetInterfaceCreditHandle handle, uint32_t size);

/** 创建固定64KiB双向窗口，失败清空输出
    @param [out] handle 不可空
    @return NI_OK或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceCreditCreate(NetInterfaceCreditHandle* handle);

/** 在本模块释放窗口
    @param [in] handle 有效句柄或null
    */
EMSCRIPTEN_KEEPALIVE void netTestInterfaceCreditDestroy(NetInterfaceCreditHandle handle);

/** 查询可发送和待归还字节数，持续错误时两项为0
    @param [in] handle 本模块创建的有效窗口句柄，不可空
    @param [out] fields 不可空，至少2个uint32，依次为可发送和待归还字节数
    @return NI_OK或持续错误；参数非法时返回NI_INVALID_ARGUMENT且不写输出
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceCreditGet(NetInterfaceCreditHandle handle, uint32_t* fields);

/** 对应C++grant，拒绝凭空或重复扩大窗口
    @param [in] handle 窗口
    @param [in] size 对端实际归还字节数
    @return NI_OK或持续错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceCreditGrant(NetInterfaceCreditHandle handle, uint32_t size);

/** 对应C++received，只在拥有完整密文记录后计入
    @param [in] handle 窗口
    @param [in] size 完整记录密文字节
    @return NI_OK或持续错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceCreditReceived(NetInterfaceCreditHandle handle, uint32_t size);

/** 对应C++sent；LIMIT_EXCEEDED不扣额度
    @param [in] handle 窗口
    @param [in] size 已由下层拥有的1至16384字节密文
    @return NI_OK或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceCreditSent(NetInterfaceCreditHandle handle, uint32_t size);

/** 导出只含公开候选的NGC3缓存，不重新实现JS协议
    @param [in] handle 有效句柄
    @param [in] utcLow 当前UTC毫秒低32位
    @param [in] utcHigh 当前UTC毫秒高32位
    @param [out] output 输出，允许空以查询required
    @param [in] capacity 输出容量
    @param [out] required 所需字节数，不得与output重叠；一般参数失败为0，重叠拒绝且保持原值
    @return NI_OK、NI_BUFFER_TOO_SMALL或错误；空间不足不修改输出
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceDirectoryCache(NetInterfaceDirectoryHandle handle, uint32_t utcLow, uint32_t utcHigh,
        uint8_t* output, uint32_t capacity, uint32_t* required);

/** 查询已验证的完整候选数量
    @param [in] handle 有效句柄
    @param [out] count 成功1至64，失败0
    @return NI_OK或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceDirectoryCount(NetInterfaceDirectoryHandle handle, uint32_t* count);

/** 解析完整发现表，创建只读候选句柄；失败清空，不授予身份
    @param [in] data 非空protobuf
    @param [in] size 最大16384字节
    @param [out] handle 不可空且不得与data重叠；重叠拒绝且不修改输入
    @return NI_OK或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceDirectoryCreate(const uint8_t* data, uint32_t size, NetInterfaceDirectoryHandle* handle);

/** 校验NGC3缓存并创建句柄，UTC拆为低/高32位避免JS精度损失
    @param [in] data 非空缓存
    @param [in] size 最大16432字节
    @param [in] utcLow 当前UTC毫秒低32位
    @param [in] utcHigh 当前UTC毫秒高32位
    @param [out] handle 不得与data重叠；一般失败清空，重叠拒绝且不修改输入
    @return NI_OK、NI_TIMEOUT或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceDirectoryCreateCache(const uint8_t* data, uint32_t size, uint32_t utcLow,
        uint32_t utcHigh, NetInterfaceDirectoryHandle* handle);

/** 在创建模块销毁
    @param [in] handle 有效句柄或null
    */
EMSCRIPTEN_KEEPALIVE void netTestInterfaceDirectoryDestroy(NetInterfaceDirectoryHandle handle);

/** 复制一个公开节点，不暴露C++布局；输出缓冲不得重叠
    @param [in] handle 有效句柄
    @param [in] index 从0开始
    @param [out] name 节点ASCII名，包含NUL需38字节
    @param [in] nameCapacity 节点名缓冲容量
    @param [out] address 数值地址，包含NUL最多40字节
    @param [in] addressCapacity 地址缓冲容量
    @param [out] fields 四项uint32：端口、容量、boot低32、高32；一般失败清零，输出重叠时全部保持原值
    @return NI_OK、NI_BUFFER_TOO_SMALL或错误；失败不修改字符串
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceDirectoryNode(NetInterfaceDirectoryHandle handle, uint32_t index,
        char* name, uint32_t nameCapacity, char* address, uint32_t addressCapacity, uint32_t* fields);

/** 接纳DATA/ACK帧
    @param [in] handle 流
    @param [in] fields 六项kind/product/id低高/type/size
    @param [in] payload fields[5]非零时不可空
    @return 同Flow.accept
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceFileFlowAccept(NetInterfaceFileFlowHandle handle, const uint32_t* fields, const uint8_t* payload);

/** 取消本地流，宿主另外发文件CANCEL RPC并收敛文件工作
    @param [in] handle 流
    @param [in] reason 负数错误
    @return OK或INVALID_ARGUMENT
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceFileFlowCancel(NetInterfaceFileFlowHandle handle, int32_t reason);

/** 实际文件提交结果，禁止入队时就报成功
    @param [in] handle 流
    @param [in] result NI_OK已提交，负数失败
    @return 同Flow.committed
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceFileFlowCommitted(NetInterfaceFileFlowHandle handle, int32_t result);

/** 下层完整接纳后消费
    @param [in] handle 流
    @return 同Flow.consumeOutput
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceFileFlowConsume(NetInterfaceFileFlowHandle handle);

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
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceFileFlowCreateWire(const uint8_t* bytes, uint32_t size, uint32_t product, uint32_t sender,
                                                           uint32_t timeLow, uint32_t timeHigh, uint32_t timeoutMs,
                                                           NetInterfaceFileFlowHandle* handle);

/** 释放并擦除协议流，不操作宿主文件
    @param [in] handle 句柄或null
    */
EMSCRIPTEN_KEEPALIVE void netTestInterfaceFileFlowDestroy(NetInterfaceFileFlowHandle handle);

/** 借用输出；背压时不消费
    @param [in] handle 流
    @param [out] fields 六项kind/product/id低高/type/size
    @param [out] payload 借用至consume/cancel/destroy
    @return READY或其他结果，失败清输出
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceFileFlowPeek(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** payload);

/** 按顺序补恢复前缀
    @param [in] handle 流
    @param [in] offsetLow 前缀位置低位
    @param [in] offsetHigh 高位
    @param [in] bytes 文件前缀
    @param [in] size 不超过剩余量且最多16MiB
    @return 同Flow.prime
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceFileFlowPrime(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
                                                      const uint8_t* bytes, uint32_t size);

/** 接纳异步读取结果并复制
    @param [in] handle 流
    @param [in] offsetLow 工作偏移低位
    @param [in] offsetHigh 高位
    @param [in] bytes 完整块
    @param [in] size 精确长度
    @return 同Flow.produced
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceFileFlowProduced(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
                                                         const uint8_t* bytes, uint32_t size);

/** 驱动时限
    @param [in] handle 流
    @param [in] timeLow 单调时间低位
    @param [in] timeHigh 高位
    @return 同Flow.progress
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceFileFlowProgress(NetInterfaceFileFlowHandle handle, uint32_t timeLow, uint32_t timeHigh);

/** 申请宿主读取
    @param [in] handle 流
    @param [out] fields 三项offset低/高位和精确字节数
    @return READY读取参数，否则输出0
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceFileFlowRead(NetInterfaceFileFlowHandle handle, uint32_t* fields);

/** 可选接收端有界合并写，必须在第一块DATA之前设置，默认1
    @param [in] handle 收端流
    @param [in] chunks 不大于协商窗口，最多8；宿主支持最大chunks乘chunk字节单次写入
    @return 同Flow.setWriteBatchChunks；只减少宿主调用，不提前ACK，不改变线上协议
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceFileFlowSetWriteBatchChunks(NetInterfaceFileFlowHandle handle, uint32_t chunks);

/** 摘要通过后取一次提交任务
    @param [in] handle 流
    @return 同Flow.takeCommit
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceFileFlowTakeCommit(NetInterfaceFileFlowHandle handle);

/** 取一次持久写请求，宿主复制借用bytes后投递文件执行者
    @param [in] handle 流
    @param [out] fields 三项offset低/高位和长度
    @param [out] bytes 借用至written/cancel/destroy
    @return 同Flow.writeRequest，失败输出0/null
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceFileFlowWrite(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** bytes);

/** 回报对应实际持久写结果
    @param [in] handle 流
    @param [in] offsetLow 工作偏移低位
    @param [in] offsetHigh 高位
    @param [in] size 精确长度
    @param [in] result NI_OK持久化或负数
    @return 同Flow.written
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceFileFlowWritten(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
                                                        uint32_t size, int32_t result);

/** Wasm读取元数据，64位字段使用低/高32位
    @param [in] bytes 160字节状态体
    @param [in] size 长度
    @param [out] fields 12项：direction/state/chunk/window/total低高/offset低高/stream低高/expires低高
    @param [out] transfer 16字节
    @param [out] token 32字节秘密
    @param [out] hash 32字节整文件摘要
    @return OK或负数，失败输出全部清零
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceFileInfoFields(const uint8_t* bytes, uint32_t size, uint32_t* fields, uint8_t* transfer,
                                                       uint8_t* token, uint8_t* hash);

/** 接收经拆包的完整帧
    @param [in] handle 有效实例
    @param [in] fields 六项：kind/product/requestLow/requestHigh/type/payloadSize
    @param [in] payload 借用消息体，长度由fields指定
    @param [in] timeLow 单调毫秒低32位
    @param [in] timeHigh 单调毫秒高32位
    @return 握手READY/NEED_MORE、普通帧OK或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceHandshakeAccept(NetInterfaceHandshakeHandle handle, const uint32_t* fields, const uint8_t* payload,
                                                        uint32_t timeLow, uint32_t timeHigh);

/** 下层接纳后消费待发帧
    @param [in] handle 有效实例
    @return READY/NEED_MORE或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceHandshakeConsumeOutput(NetInterfaceHandshakeHandle handle);

/** 创建固定字段握手，供上层适配器使用有界数组传参
    @param [in] options 六项：server/platform/capabilities/maxRequests/maxChunk/timeoutMs
    @param [in] optionCount 必须为6
    @param [in] identities 固定32字节，本连接随机数16字节和设备标识16字节
    @param [in] identitySize 必须为32
    @param [in] timeLow 单调毫秒低32位
    @param [in] timeHigh 单调毫秒高32位
    @param [out] handle 必须非空，失败清空
    @return NEED_MORE或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceHandshakeCreate(const uint32_t* options, uint32_t optionCount, const uint8_t* identities,
                                                        uint32_t identitySize, uint32_t timeLow, uint32_t timeHigh,
                                                        NetInterfaceHandshakeHandle* handle);

/** 释放本实例
    @param [in] handle 实例或null；非null不得重复释放
    */
EMSCRIPTEN_KEEPALIVE void netTestInterfaceHandshakeDestroy(NetInterfaceHandshakeHandle handle);

/** 查看待发送握手，背压时不消费
    @param [in] handle 有效实例
    @param [out] fields 六项固定帧字段，未就绪时清空
    @param [out] payload 借用到consume/destroy，未就绪时清空
    @return READY有输出、NEED_MORE无输出或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceHandshakePeekOutput(NetInterfaceHandshakeHandle handle, uint32_t* fields, const uint8_t** payload);

/** 读取已协商信息；失败清空有效的输出地址
    @param [in] handle 有效实例
    @param [out] fields 平台/能力/在途上限/块大小四项
    @param [out] identities 对端随机数16字节及客户端设备标识16字节
    @return READY、NEED_MORE或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceHandshakePeerInfo(NetInterfaceHandshakeHandle handle, uint32_t* fields, uint8_t* identities);

/** 检查固定时限和时钟回退
    @param [in] handle 有效实例
    @param [in] timeLow 单调毫秒低32位
    @param [in] timeHigh 单调毫秒高32位
    @return READY、NEED_MORE或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceHandshakeProgress(NetInterfaceHandshakeHandle handle, uint32_t timeLow, uint32_t timeHigh);

/** 接纳布局无关的完整外层记录
    @param [in] handle 实例
    @param [in] fields 五项kind/product/channelLow/channelHigh/size
    @param [in] payload fields[4]非0时不可空，调用后不借用
    @return NI_OK、NI_LIMIT_EXCEEDED未接纳或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceLinkAccept(NetInterfaceLinkHandle handle, const uint32_t* fields, const uint8_t* payload);

/** 消费管理消息
    @param [in] handle 实例
    @return NI_OK或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceLinkConsumeControl(NetInterfaceLinkHandle handle);

/** 消费已送出记录
    @param [in] handle 实例
    @return NI_OK或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceLinkConsumeOutput(NetInterfaceLinkHandle handle);

/** 消费收到的业务记录
    @param [in] handle 实例
    @return NI_OK或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceLinkConsumeRecord(NetInterfaceLinkHandle handle);

/** 创建管理连接；调用方显式提供身份、运输及熵源，不提供JS工厂
    @param [in] options v3配置，不可空
    @param [out] handle 不可空，失败清零
    @return NI_NEED_MORE或错误，管理认证完成后才READY
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceLinkCreate(const NetInterfaceLinkOptions* options, NetInterfaceLinkHandle* handle);

/** 释放独占管理连接
    @param [in] handle 有效实例或null，不得重复释放
    */
EMSCRIPTEN_KEEPALIVE void netTestInterfaceLinkDestroy(NetInterfaceLinkHandle handle);

/** 借用完整管理消息
    @param [in] handle 实例
    @param [out] fields 六项帧字段
    @param [out] payload 借用至consumeControl
    @return NI_READY、NI_NEED_MORE或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceLinkPeekControl(NetInterfaceLinkHandle handle, uint32_t* fields, const uint8_t** payload);

/** 借用下一待发记录；下层拥有之后才consume
    @param [in] handle 实例
    @param [out] fields 五项路由字段，不可空
    @param [out] payload 不可空，借用至consume/destroy
    @return NI_READY、NI_NEED_MORE或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceLinkPeekOutput(NetInterfaceLinkHandle handle, uint32_t* fields, const uint8_t** payload);

/** 借用收到的业务密文
    @param [in] handle 实例
    @param [out] fields 五项字段
    @param [out] payload 借用至consumeRecord
    @return NI_READY、NI_NEED_MORE或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceLinkPeekRecord(NetInterfaceLinkHandle handle, uint32_t* fields, const uint8_t** payload);

/** 推进认证/心跳
    @param [in] handle 实例
    @param [in] low 单调毫秒低32位
    @param [in] high 单调毫秒高32位
    @return NI_READY、NI_NEED_MORE或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceLinkProgress(NetInterfaceLinkHandle handle, uint32_t low, uint32_t high);

/** 排队认证管理消息
    @param [in] handle 实例
    @param [in] fields 六项kind/product/requestLow/requestHigh/type/size
    @param [in] payload 借用至返回
    @return NI_OK、NI_LIMIT_EXCEEDED或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceLinkSendControl(NetInterfaceLinkHandle handle, const uint32_t* fields, const uint8_t* payload);

/** 排队布局无关的业务密文，不在管理层解密
    @param [in] handle 已就绪实例
    @param [in] fields 五项路由字段
    @param [in] payload 调用后不借用
    @return NI_OK、NI_LIMIT_EXCEEDED未接纳或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceLinkSendRecord(NetInterfaceLinkHandle handle, const uint32_t* fields, const uint8_t* payload);

/** 查询管理认证状态
    @param [in] handle 实例
    @return NI_READY、NI_NEED_MORE或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceLinkStatus(NetInterfaceLinkHandle handle);

/** 清空消息及累计修改预算
    @param [in] handle 有效实例
    @return NI_OK或NI_INVALID_ARGUMENT
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceMessageClear(NetInterfaceMessageHandle handle);

/** 创建空protobuf消息
    @param [out] handle 不可空；失败清空
    @return NI_OK或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceMessageCreate(NetInterfaceMessageHandle* handle);

/** 销毁消息
    @param [in] handle 有效实例或null，禁止重复释放
    */
EMSCRIPTEN_KEEPALIVE void netTestInterfaceMessageDestroy(NetInterfaceMessageHandle handle);

/** 序列化同源protobuf消息，不在C++与宿主间转移内存所有权
    @param [in] handle 有效实例
    @param [out] output 输出，capacity为0时可空
    @param [in] capacity 容量
    @param [out] required 不可空，所需字节数；不足空间时缓冲保持不变
    @return NI_OK或负数错误，最大1MiB
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceMessageEncode(NetInterfaceMessageHandle handle, uint8_t* output, uint32_t capacity,
                                                      uint32_t* required);

/** 查询状态码和错误文本
    @param [in] handle 有效实例
    @param [out] code 不可空
    @param [out] error 调用方缓冲，capacity为0时可空
    @param [in] capacity 缓冲容量
    @param [out] required 不可空，文本字节数，不添加NUL
    @return NI_OK或负数错误；不足空间时code及required仍有效
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceMessageGetCode(NetInterfaceMessageHandle handle, int32_t* code, uint8_t* error, uint32_t capacity,
                                                       uint32_t* required);

/** 查询单个map字段
    @param [in] handle 有效实例
    @param [in] group 容器键
    @param [in] key 字段键
    @param [out] type 不可空，Variant类型
    @param [out] output 字节缓冲，capacity为0时可空
    @param [in] capacity 字节容量
    @param [out] required 不可空，实际字节数；不足空间不写缓冲
    @return NI_OK、NI_NOT_FOUND或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceMessageGetValue(NetInterfaceMessageHandle handle, int32_t group, int32_t key, int32_t* type,
                                                        uint8_t* output, uint32_t capacity, uint32_t* required);

/** 有界解析，所有失败均先清除旧字段，禁止沿用旧账号等内容
    @param [in] handle 有效实例
    @param [in] input 完整protobuf，size为0时可空
    @param [in] size 最大1MiB；容器/值节点最大65536
    @return NI_OK或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceMessageParse(NetInterfaceMessageHandle handle, const uint8_t* input, uint32_t size);

/** 设置状态码，序列化不会清除它
    @param [in] handle 有效实例
    @param [in] code 业务码
    @param [in] error UTF8错误文本，size为0时可空
    @param [in] size 最多4096字节
    @return NI_OK或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceMessageSetCode(NetInterfaceMessageHandle handle, int32_t code, const uint8_t* error, uint32_t size);

/** 设置map字段，保留既有Variant编号及二进制格式，字符串可以含NUL
    @param [in] handle 有效实例，单消息累计修改成本最多1MiB，clear后恢复预算
    @param [in] group 容器键
    @param [in] key 字段键
    @param [in] type INIT0/INT32=1/UINT64=2/DOUBLE=3/STRING=4/BLOB=5
    @param [in] value 字节，size为0时可空；数字为现有小端4/8字节
    @param [in] size 字节数，固定宽度会校验
    @return NI_OK或负数错误；非法输入不改旧值
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceMessageSetValue(NetInterfaceMessageHandle handle, int32_t group, int32_t key, int32_t type,
                                                        const uint8_t* value, uint32_t size);

/** 登记宿主验证过的一代端点
    @param [in] handle 有效实例
    @param [in] low 端点号低32位
    @param [in] high 端点号高32位
    @param [in] service 0外部端点，1本地可信服务
    @param [in] product 专用产品号，外部端点为0
    @param [in] limit 非零兼容协商提示；实际请求存储由请求池配置决定
    @param [in] internalOnly 0普通服务，1只允许合法父请求访问
    @return OK或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceRequestsAddEndpoint(NetInterfaceRequestsHandle handle, uint32_t low, uint32_t high,
                                                            uint32_t service, uint32_t product, uint32_t limit, uint32_t internalOnly);

/** 登记完整请求，所有64位值用低/高uint32相邻表示
    @param [in] handle 有效实例
    @param [in] input 十三项：source低高、original低高、target低高、product、type、parent低高、now低高、timeoutMs
    @param [in] inputCount 必须13
    @param [in] autoId 0采用input原号；1由C++生成，input原号必须0
    @param [out] output 十六项：route/source/original/target/origin/parent/deadline各低高，product、type；失败清空
    @return OK、CANCELLED或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceRequestsBegin(NetInterfaceRequestsHandle handle, const uint32_t* input, uint32_t inputCount,
                                                      uint32_t autoId, uint32_t* output);

/** 取消来源请求或登记先到的取消控制
    @param [in] handle 有效实例
    @param [in] sourceLow 来源低32位
    @param [in] sourceHigh 来源高32位
    @param [in] originalLow 原请求低32位
    @param [in] originalHigh 原请求高32位
    @param [out] output 十六项请求记录，仅READY时填充
    @return READY、OK或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceRequestsCancel(NetInterfaceRequestsHandle handle, uint32_t sourceLow, uint32_t sourceHigh,
                                                       uint32_t originalLow, uint32_t originalHigh, uint32_t* output);

/** 目标完成请求，校验身份/元数据及当前期限
    @param [in] handle 有效实例
    @param [in] input 网关八项：target低高、route低高、product、type、now低高；客户端十项：source低高、original低高、target低高、product、type、now低高
    @param [in] inputCount 8按路由号完成，10按来源原请求号完成
    @param [out] output 十六项请求记录，READY/TIMEOUT/CANCELLED时填充，其他结果清空
    @return READY、TIMEOUT、CANCELLED或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceRequestsComplete(NetInterfaceRequestsHandle handle, const uint32_t* input, uint32_t inputCount,
                                                         uint32_t* output);

/** 查询在途数
    @param [in] handle 有效实例
    @return 0至4096，空句柄INVALID_ARGUMENT
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceRequestsCount(NetInterfaceRequestsHandle handle);

/** 创建等待表
    @param [in] capacity 1至4096
    @param [out] handle 不可空，失败清空
    @return OK或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceRequestsCreate(uint32_t capacity, NetInterfaceRequestsHandle* handle);

/** 释放等待表，不执行宿主回调
    @param [in] handle 实例或null，非null不得重复释放
    */
EMSCRIPTEN_KEEPALIVE void netTestInterfaceRequestsDestroy(NetInterfaceRequestsHandle handle);

/** 每次仅取一个需终止的请求，宿主控制本轮次数
    @param [in] handle 有效实例
    @param [in] operation 0到期、1端点关闭、2父请求结束
    @param [in] keyLow 单调时间/端点号/父路由号的低32位
    @param [in] keyHigh 相应高32位
    @param [out] output 十六项请求记录，仅READY时填充
    @return READY、NOT_FOUND或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceRequestsTake(NetInterfaceRequestsHandle handle, uint32_t operation, uint32_t keyLow,
                                                     uint32_t keyHigh, uint32_t* output);

/** 共享控制帧验证/回复，避免微信重复协议逻辑
    @param [in] fields kind/product/requestLow/requestHigh/type/payloadSize共6项
    @param [in] count 必须为6
    @param [out] reply 六项；仅READY时为PONG，其余清零，允许同输入
    @return NetInterfaceRpc::control结果或INVALID_ARGUMENT
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceRpcControl(const uint32_t* fields, uint32_t count, uint32_t* reply);

/** 固定字段RPC信封编码，不传递C++布局或JS浮点请求号
    @param [in] kind REQUEST或RESPONSE
    @param [in] value 剩余毫秒或传输状态
    @param [in] parentLow 内部父请求号低32位
    @param [in] parentHigh 内部父请求号高32位
    @param [in] data 借用业务字节
    @param [in] size 字节数
    @param [out] output 不重叠输出空间
    @param [in] capacity 输出容量
    @param [out] required 不可空，所需空间
    @return NetInterfaceRpc::encode结果
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceRpcEncode(uint32_t kind, uint32_t value, uint32_t parentLow, uint32_t parentHigh,
                                                  const uint8_t* data, uint32_t size, uint8_t* output, uint32_t capacity,
                                                  uint32_t* required);

/** 解析已拆包的RPC帧；不复制业务体
    @param [in] frameFields kind/product/requestLow/requestHigh/type/payloadSize共6项
    @param [in] count 必须为6
    @param [in] data frameFields声明长度的消息体
    @param [out] fields value/parentLow/parentHigh/bodyOffset/bodySize共5项；失败清零，允许与输入字段重叠
    @return OK或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceRpcParse(const uint32_t* frameFields, uint32_t count, const uint8_t* data, uint32_t* fields);

/** 消费当前完整记录
    @param [in] handle 实例
    @return NI_OK或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceTunnelConsume(NetInterfaceTunnelHandle handle);

/** 创建原生/Wasm共用的外层拆包器
    @param [out] handle 不可空，失败清零
    @return NI_OK或负数错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceTunnelCreate(NetInterfaceTunnelHandle* handle);

/** 在拥有缓冲的模块释放
    @param [in] handle 有效句柄或null，不得重复释放
    */
EMSCRIPTEN_KEEPALIVE void netTestInterfaceTunnelDestroy(NetInterfaceTunnelHandle handle);

/** 编码固定外层头，64位编号由两项uint32传递，禁止JS浮点合并
    @param [in] fields 五项kind/product/channelLow/channelHigh/size
    @param [in] payload fields[4]非0时不可空
    @param [out] output 调用者输出，不允许与payload重叠
    @param [in] capacity 输出空间
    @param [out] required 不可空，合法输入所需字节数
    @return NI_OK、NI_BUFFER_TOO_SMALL或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceTunnelEncode(const uint32_t* fields, const uint8_t* payload,
    uint8_t* output, uint32_t capacity, uint32_t* required);

/** 提交外层字节，只消费到一条完整记录
    @param [in] handle 实例
    @param [in] data size非零时不可空
    @param [in] size 字节数
    @param [out] consumed 不可空，实际接纳量
    @return NI_READY、NI_NEED_MORE或错误
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceTunnelFeed(NetInterfaceTunnelHandle handle, const uint8_t* data, uint32_t size, uint32_t* consumed);

/** 获取布局无关的路由头和借用密文
    @param [in] handle 实例
    @param [out] fields 五项kind/product/channelLow/channelHigh/size，不可空
    @param [out] payload 不可空，借用至consume/destroy
    @return NI_READY或当前状态，非READY输出清零
    */
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceTunnelPeek(NetInterfaceTunnelHandle handle, uint32_t* fields, const uint8_t** payload);

/** 从测试宿主的真实强随机源创建TLS客户端，协议仍由NetInterface实现
@param [in] authority,authoritySize 测试CA及包含PEM结尾NUL的长度
@param [in] hostname 测试期望域名
@param [in] timeoutMs 握手期限，毫秒
@param [out] handle 真实TLS句柄，失败为空
@param [out] detail 底层错误
@return 真实协议状态或错误码
*/
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceWasmConnectionCreate(const uint8_t* authority,
    uint32_t authoritySize, const char* hostname, uint32_t timeoutMs,
    NetInterfaceConnectionHandle* handle, int32_t* detail);

/** 以生产固定CA创建测试业务会话，不向正式SDK增加C入口
@param [in] product 服务编号
@param [in] timeoutMs 握手期限，毫秒
@param [out] handle 真实业务TLS句柄
@param [out] detail 底层错误
@return 真实协议状态或错误码
*/
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceWasmServiceConnectionCreate(uint32_t product,
    uint32_t timeoutMs, NetInterfaceConnectionHandle* handle, int32_t* detail);

/** 为独立JS协议互操作测试创建CLIENT角色的管理链路
@param [in] reserved,reservedSize 必须为空及0，不接受调用方替换生产信任根
@param [in] timeoutMs 握手期限，毫秒
@param [out] handle 真实管理TLS链路句柄
@return 真实协议状态或错误码
*/
EMSCRIPTEN_KEEPALIVE int32_t netTestInterfaceWasmLinkCreateClient(const uint8_t* reserved,
    uint32_t reservedSize, uint32_t timeoutMs, NetInterfaceLinkHandle* handle);
}
#endif