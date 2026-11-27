#pragma once
#include <stdint.h>

// 宿主回调只携带原始字节/连接事件，不解释任何网关协议
typedef void (*NetHostConnected)(void* context, uint64_t peer);
// 输入借用到回调返回，单次1至16384字节；必须遵守pauseRead
typedef void (*NetHostData)(void* context, uint64_t peer, const uint8_t* data, uint32_t size);
// 本地写完成，可写预算释放不表示对端消费
typedef void (*NetHostWritable)(void* context, uint64_t peer);
// 本实例peer按分配顺序严格递增且不复用；每peer至多一次终态
typedef void (*NetHostClosed)(void* context, uint64_t peer, int32_t reason);
// 宿主密码学随机，0完整成功，其余失败；只借用输出到返回，不允许弱随机回退
typedef int32_t (*NetHostEntropy)(void* context, uint8_t* output, uint32_t size);

// 可选的执行等待宿主；全空表示调用方自行调度，填写时六个函数必须完整。
// create/exchange/begin/wait/destroy串行；只有wake可跨线程，外层必须保护借用期。
typedef struct NetHostExecutionFunctions
{
    // 创建无线程执行循环。成功返回 0 并交付实例。失败通常输出空。若已交付实例后返回错误或抛出异常，执行者会调用 destroy 一次。
    int32_t (*create)(void** instance);
    // 交换当前执行者中新建网络通道的循环归属，返回旧归属
    void* (*exchange)(void* instance);
    // 检查所有工作之前取得代次，通知不能在随后wait中丢失
    uint64_t (*begin)(void* instance);
    // 等待网络、wake或最长毫秒；期间已有进展立即返回，0成功
    int32_t (*wait)(void* instance, uint64_t observed, uint32_t timeoutMs);
    // 发布后通知，可合并但不得丢失；不得与destroy并发
    void (*wake)(void* instance);
    // 先释放所有通道、关闭外部唤醒者；0已释放，负数未释放
    int32_t (*destroy)(void* instance);
} NetHostExecutionFunctions;

// v1完整函数表，调用方拥有providerContext至所有实例destroy成功
// 所有方法和事件只允许同一个执行者；回调可同步触发但不直接调用上层业务
// 所有操作0成功、负数未接纳；write额外以1表示完全未接纳的背压
// destroy成功必须保证绝无后续事件，失败保留实例供宿主处理，不能释放回调上下文
typedef struct NetHostFunctions
{
    // 必须等于sizeof(NetHostFunctions)，禁止读取短结构
    uint32_t structSize;
    // 当前必须为1
    uint32_t version;
    // 宿主提供方上下文，借用至对应运输全部销毁
    void* context;
    // 创建独占实例；失败通常输出null，未收敛的部分实例仅交还shutdown/destroy清理所有权；listen/connect前不发事件，事件上下文不可转交其他执行者
    int32_t (*create)(void* providerContext, void* observerContext, NetHostConnected connected, NetHostData data,
        NetHostWritable writable, NetHostClosed closed, void** instance);
    // 可空表示不提供监听；数值IPv4/IPv6，port 0供隔离测试，maxPeers为非零预留提示，实际连接存储由宿主池配置控制；只启动一次
    int32_t (*listen)(void* instance, const char* address, uint32_t port, uint32_t maxPeers);
    // 可空表示不提供主动连接，但listen/connect至少一个非空；数值地址和1至65535端口，失败清0
    int32_t (*connect)(void* instance, const char* address, uint32_t port, uint64_t* peer);
    // 非阻塞推进，1仍活动、0收敛、负数失败；不得嵌套驱动
    int32_t (*poll)(void* instance);
    // 只借用至返回，size至多32768；0表示已实际调用网络库发送接口且完整接纳
    // 禁止仅进入NetPeer适配层待发队列就返回0；第三方库内部队列允许，1无副作用背压，负数未接纳
    int32_t (*write)(void* instance, uint64_t peer, const uint8_t* data, uint32_t size);
    // paused仅0/1；暂停后不再交付新输入，宿主自身缓冲也须有界
    int32_t (*pauseRead)(void* instance, uint64_t peer, int32_t paused);
    // 请求关闭，允许同步终态，不存在或已关闭peer无操作
    int32_t (*close)(void* instance, uint64_t peer, int32_t reason);
    // 请求停止并回收全部所属资源，不得再接纳新连接；可同步终态
    int32_t (*shutdown)(void* instance);
    // 外层执行者停止后释放，回调/poll内拒绝且不得释放，成功后绝无后续事件
    int32_t (*destroy)(void* instance);
    // 实际监听端口，未监听0
    uint32_t (*localPort)(void* instance);
    // 包含关闭中连接的资源数
    uint32_t (*peerCount)(void* instance);
    // 可空表示仅提供字节宿主；创建安全会话必须由TLS配置或此函数提供强随机
    NetHostEntropy entropy;
    // 可选的本地执行等待；不改变poll非阻塞语义，不用于JS主动驱动宿主
    NetHostExecutionFunctions execution;
} NetHostFunctions;