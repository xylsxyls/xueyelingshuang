#pragma once
#include "NetInterfaceMacro.h"
#include "NetInterfaceTlsTypes.h"
#include <stdint.h>
#include <string>
#include <vector>

// 仅为候选地址提示，不构成服务器身份或业务授权
struct NetGatewayAddress
{
    // 数值IPv4/IPv6，不是证书身份
    std::string m_address;
    // 1至65535，输出失败时0
    uint32_t m_port;
};

// 发现器向会话交付的v1值，不转交字符串分配或释放权。
struct NetClientDiscoveryPoint
{
    // sizeof(NetClientDiscoveryPoint)。
    uint32_t m_structSize;
    // 当前为1。
    uint32_t m_version;
    // 数值IP，最多39字节及NUL。
    char m_address[40];
    // 1至65535。
    uint32_t m_port;
};

// 会话只借用发现器；实际字节传输和可选缓存持久化由宿主组合提供
class NetInterfaceAPI NetClientDiscovery
{
public:
    /** 不拥有使用本接口的会话，必须先停止会话再销毁发现器
    */
    virtual ~NetClientDiscovery();

    /** 接纳异步发现，地址/路径由实现有界复制，不解释业务载荷
    @param [in] seeds 1至64个数值入口
    @param [in] cachePath 可空，空表示仅内存；非空需要宿主持久化能力
    @param [in] tls 仅使用客户端强随机和期限，上下文借用至停止，不采用外部信任根
    @return OK接纳或负数；失败后仍允许调用shutdown收敛
    */
    virtual int32_t start(const std::vector<NetGatewayAddress>& seeds, const std::string& cachePath,
        const NetInterfaceTlsOptions& tls) = 0;

    /** 同一执行者非阻塞推进，不直接交付业务回调
    @return 是否仍有发现资源
    */
    virtual bool poll() = 0;

    /** 请求停止本发现代次；后续poll推进资源回收，不销毁借用者
    */
    virtual void shutdown() = 0;

    /** 是否已经收敛所有资源和回调
    @return true后可销毁，异步文件工作未结束时必须false
    */
    virtual bool stopped() const = 0;

    /** 选择尚未退避的候选，真实连接仍须TLS认证
    @param [out] endpoint 调用者预填v1 size/version；成功为完整值，失败内容不可使用
    @return 是否可尝试
    */
    virtual bool choose(NetClientDiscoveryPoint& endpoint) = 0;

    /** 标记一个既有候选本次失败，不丢弃其他备用
    @param [in] endpoint 由choose取得的地址
    */
    virtual void failed(const NetClientDiscoveryPoint& endpoint) = 0;

    /** 校验v1候选输出，先限制NUL再解释数值地址，不分配任何跨模块输出。
    @param [in] endpoint 借用固定值
    @return 元数据、地址和端口均合法
    */
    static bool validPoint(const NetClientDiscoveryPoint& endpoint);

    /** 取得数值地址的统一16字节键，IPv4与其映射IPv6使用同一键；端口由调用者另比
    @param [in] endpoint 借用v1候选，必须包含合法端口和有界NUL地址
    @param [out] key 非空且至少16字节，失败保持原值，不分配内存
    @return 候选合法且已完整复制键
    */
    static bool addressKey(const NetClientDiscoveryPoint& endpoint, uint8_t* key);

    /** 值数组解析入口，NI只填调用者自己的固定字节，不转交容器。
    @param [in] text 非空ASCII入口列表，最多4096字节、64项
    @param [out] points 预填size/version的v1数组，不得与text重叠；容量不足或失败保持原值
    @param [in] capacity 0至64，空数组及0可查询所需项数
    @param [out] count 必需项数；一般参数/解析错误为0；不得与points或text重叠，重叠时拒绝且全部保持原值
    @return OK、BUFFER_TOO_SMALL、INVALID_ARGUMENT或NO_MEMORY
    */
    static int32_t parseSeeds(const std::string& text, NetClientDiscoveryPoint* points, uint32_t capacity, uint32_t& count);

    /** 同模块/静态兼容纯解析器；跨DLL调用值数组重载，不转交STL释放权
    @param [in] text 非空ASCII入口列表，最多4096字节、64项
    @param [out] points 成功去重数值候选，非法输入失败时清空
    @return OK、INVALID_ARGUMENT或NO_MEMORY
    */
    static int32_t parseSeeds(const std::string& text, std::vector<NetGatewayAddress>& points);
};