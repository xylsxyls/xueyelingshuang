#pragma once
#include "NetInterfaceRequests.h"
#include <map>
#include "NetObjectPool.h"
#include <unordered_map>
#include <utility>
#include <vector>

struct NetInterfaceRequestEntry;

// 同一请求在三个端点集合中的职责，数组索引不从网络输入取得。
enum NetInterfaceRequestRole
{
    // 请求来源，用于来源并发额度和原号查找。
    NI_REQUEST_SOURCE = 0,
    // 真实目标服务，用于目标并发额度和断线清理。
    NI_REQUEST_TARGET = 1,
    // 根客户端，包括内部子请求，用于整条业务链的断线清理。
    NI_REQUEST_ORIGIN = 2,
    // 固定集合数量，不是合法角色。
    NI_REQUEST_ROLE_COUNT = 3
};

// 可信端点拥有索引入口，不拥有请求节点；有相关请求时不允许移除端点。
struct NetInterfaceRequestEndpoint
{
public:
    // 握手能力元数据；实际存储由池控制，不按此值拒绝请求。
    uint32_t m_limit;
    // 专用产品或通用服务0。
    uint32_t m_product;
    // 宿主验证的服务角色。
    bool m_service;
    // 仅允许合法父请求访问。
    bool m_internalOnly;
    // 已处理来源原号的高水位，容量拒绝同样推进。
    uint64_t m_highestOriginal;
    // 各角色仍在途的实际数量。
    size_t m_count[NI_REQUEST_ROLE_COUNT];
    // 各角色最早接纳的请求，非拥有指针。
    NetInterfaceRequestEntry* m_first[NI_REQUEST_ROLE_COUNT];
    // 各角色最后接纳的请求，用于无分配追加。
    NetInterfaceRequestEntry* m_last[NI_REQUEST_ROLE_COUNT];
    // 本端点发起的原请求号到权威节点；单执行者无序索引，不重复存储source键
    std::unordered_map<uint64_t, NetInterfaceRequestEntry*> m_originals;
    // 尚未到达的未来取消，不按时间丢弃而重新放行业务。
    std::map<uint64_t, NetObjectReference<uint64_t>> m_futureCancelled;

public:
    /** 建立空索引和未授权角色
    */
    NetInterfaceRequestEndpoint();
};

// 物理池持有请求元数据；索引持有租约或借用其稳定地址。
struct NetInterfaceRequestEntry
{
public:
    // 完整可信身份和固定截止，不从响应覆盖。
    NetInterfaceRequest m_request;
    // 三个角色所属端点，始终活到本请求移除。
    NetInterfaceRequestEndpoint* m_endpoint[NI_REQUEST_ROLE_COUNT];
    // 每角色的前一请求，借用到摘除。
    NetInterfaceRequestEntry* m_previous[NI_REQUEST_ROLE_COUNT];
    // 每角色的后一请求。
    NetInterfaceRequestEntry* m_next[NI_REQUEST_ROLE_COUNT];
    // 同父请求的前一子请求；父节点本身允许先结束。
    NetInterfaceRequestEntry* m_previousChild;
    // 同父请求的后一子请求。
    NetInterfaceRequestEntry* m_nextChild;
    // 截止最小堆中的准确位置，移除时不扫描堆。
    size_t m_heapIndex;

public:
    /** 建立尚未提交任何索引的请求
    @param [in] request 已验证可信元数据
    */
    explicit NetInterfaceRequestEntry(const NetInterfaceRequest& request);
};

// 父请求消失后仍保留子链入口，直到最后一个子请求收敛。
struct NetInterfaceRequestChildren
{
public:
    // 最早接纳的子请求，非拥有。
    NetInterfaceRequestEntry* m_first;
    // 最后接纳的子请求，非拥有。
    NetInterfaceRequestEntry* m_last;

public:
    /** 创建空子链
    */
    NetInterfaceRequestChildren();
};

// 单执行者维护的有界请求表；登记先准备全部分配，再无分配提交索引。
class NetInterfaceRequestsState
{
public:
    /** 初始化实际物理池和单执行者索引
    @param [in] pools 请求、端点及提前取消的物理配置
    */
    explicit NetInterfaceRequestsState(const NetRequestTablePools& pools);

    /** 原子登记已通过验证的完整元数据，不在失败后扣额度
    @param [in] request 含唯一非零route和存在的三个端点
    @param [in] source 已验证来源
    @param [in] target 已验证目标
    @param [in] origin 已验证根端点
    @return OK或NO_MEMORY；失败不保留半条请求
    */
    int32_t insert(const NetInterfaceRequest& request, NetInterfaceRequestEndpoint* source,
        NetInterfaceRequestEndpoint* target, NetInterfaceRequestEndpoint* origin);

    /** 从全部索引和额度中取走一次，不分配内存
    @param [in] entry 本表中有效节点，不可为空
    @param [out] request 原元数据
    @return READY
    */
    int32_t take(NetInterfaceRequestEntry* entry, NetInterfaceRequest& request);

    /** 修复新增或减小位置后的截止堆，同时维护反向位置
    @param [in] index 有效堆下标
    */
    void rise(size_t index);

    /** 修复移除节点后由末项填补的截止堆
    @param [in] index 有效堆下标
    */
    void sink(size_t index);

    /** 比较两个截止节点，期限相同按路由号稳定取出
    @param [in] left 非空有效节点
    @param [in] right 非空有效节点
    @return left是否应先处理
    */
    static bool earlier(const NetInterfaceRequestEntry* left, const NetInterfaceRequestEntry* right);

private:
    /** 禁止复制包含借用索引的独占状态
    @param [in] other 原状态
    */
    NetInterfaceRequestsState(const NetInterfaceRequestsState& other);

    /** 禁止复制索引所有权
    @param [in] other 原状态
    @return 不可调用
    */
    NetInterfaceRequestsState& operator=(const NetInterfaceRequestsState& other);

public:
    // 实际池先于索引声明，确保晚于全部租约析构。
    NetObjectPool<NetInterfaceRequestEntry> m_requestPool;
    // 端点池在请求索引清空前始终有效。
    NetObjectPool<NetInterfaceRequestEndpoint> m_endpointPool;
    // 取消池在端点索引清空后析构。
    NetObjectPool<uint64_t> m_cancelPool;
    // 单执行者无序请求索引；实际对象地址由池保证稳定。
    std::unordered_map<uint64_t, NetObjectReference<NetInterfaceRequestEntry>> m_requests;
    // 活跃可信端点，各持有一个实际池槽。
    std::unordered_map<uint64_t, NetObjectReference<NetInterfaceRequestEndpoint>> m_endpoints;
    // 最早截止位于首项；每个在途请求对应一项，空间在本实例复用。
    std::vector<NetInterfaceRequestEntry*> m_expiry;
    // 仅有实际子请求的父路由索引，不因父结束丢掉子请求。
    std::unordered_map<uint64_t, NetInterfaceRequestChildren> m_children;
    // 内部请求号不回绕、不复用。
    uint64_t m_lastRoute;
    // 端点号不回绕、不复用。
    uint64_t m_lastEndpoint;
    // 最近单调时间，回退后保持错误。
    uint64_t m_lastMs;
    // 参数或时钟造成的持续状态。
    int32_t m_status;
};