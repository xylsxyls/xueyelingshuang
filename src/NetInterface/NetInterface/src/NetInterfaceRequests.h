#pragma once
#include "NetInterfaceTypes.h"

class NetInterfaceRequestsState;

// 仅包含本实例的可信路由元数据；不从网络采用source/target/parent。
struct NetInterfaceRequest
{
    // 网关本轮生命周期内不复用的内部请求号
    uint64_t m_route;
    // 验证后的来源端点和来源的原请求号
    uint64_t m_source;
    // 来源连接内的原请求号，不与网关内部路由号混用
    uint64_t m_original;
    // 验证后的目标端点；回复只允许来自此端点
    uint64_t m_target;
    // 根客户端端点及父请求；父为0表示外部请求
    uint64_t m_origin;
    // 内部调用的父路由号，外部请求必须为0，期限不能超过父请求
    uint64_t m_parent;
    // 固定单调截止时间；内部请求不能晚于其父请求
    uint64_t m_deadline;
    // 原产品和业务类型，回复不能改写
    uint32_t m_product;
    // 原业务消息类型，目标回复必须保持一致
    uint32_t m_type;
};

// 单执行者、有界元数据表，不拥有消息体、回调或线程。端点号对应一代连接/服务实例。
class NetInterfaceAPI NetInterfaceRequests
{
    public:
    /** 创建等待表；容量非法时后续入口返回INVALID_ARGUMENT
    @param [in] capacity 全局在途上限，1至4096
    */
    explicit NetInterfaceRequests(uint32_t capacity = 4096);

    /** 释放本模块资源，不执行外部回调
    */
    ~NetInterfaceRequests();

    /** 登记已经通过宿主验证的端点，不接纳客户端自报PID/代次
    @param [in] endpoint 必须递增且不复用的非零号，最多1024个活跃端点
    @param [in] service 是否本地可信服务端点，外部连接为false
    @param [in] product 专用服务产品号，0表示通用服务；外部连接必须为0
    @param [in] limit 本端点入/出请求分别的上限，1至128
    @param [in] internalOnly 仅可信服务允许设置，必须经合法父请求访问
    @return OK或负数错误
    */
    int32_t addEndpoint(uint64_t endpoint, bool service, uint32_t product, uint32_t limit, bool internalOnly = false);

    /** 在相关请求全部取走后移除端点；此号永不再次登记
    @param [in] endpoint 既有端点号
    @return OK、NOT_FOUND或仍有在途的INVALID_STATE
    */
    int32_t removeEndpoint(uint64_t endpoint);

    /** 应用握手完成后收紧预登记端点额度，不允许低于已有在途数
    @param [in] endpoint 既有代次
    @param [in] limit 协商后的1至128
    @return OK、NOT_FOUND或INVALID_ARGUMENT
    */
    int32_t setEndpointLimit(uint64_t endpoint, uint32_t limit);

    /** 只读取得发给指定服务的根请求上下文，不改变等待表、额度或期限
    @param [in] target 已验证的服务端点
    @param [in] route 父路由号
    @param [in] product 原产品号
    @param [out] request 可信根元数据；找不到或归属不符时清零
    @return READY或NOT_FOUND；允许读取已到期限但尚未清理的根以回传失败
    */
    int32_t parentContext(uint64_t target, uint64_t route, uint32_t product, NetInterfaceRequest& request) const;

    /** 接纳一个完整请求；来源原请求号必须严格递增
    容量拒绝也消耗原请求号，重试业务使用新号；不能把本方法用于尚未收到完整帧的探测。
    @param [in] source 可信来源端点
    @param [in] original 来源原请求号，非零
    @param [in] target 可信目标服务端点
    @param [in] product 非零产品号
    @param [in] type 业务类型
    @param [in] parent 内部Account查询的父路由号，外部为0；当前最多两级
    @param [in] nowMs 同一宿主单调时间
    @param [in] timeoutMs 1至60000毫秒
    @param [out] request 成功时填充，失败清零
    @return OK、CANCELLED（取消先于请求到达，不执行）或负数错误
    */
    int32_t begin(uint64_t source, uint64_t original, uint64_t target, uint32_t product, uint32_t type, uint64_t parent, uint64_t nowMs,
        uint32_t timeoutMs, NetInterfaceRequest& request);

    /** 客户端/内部服务发起操作时由C++生成下一原请求号，不在JS维护64位计数
    @param [in] source 可信来源端点
    @param [in] target 可信目标服务
    @param [in] product 非零产品号
    @param [in] type 业务类型
    @param [in] parent 父路由号，外部操作为0
    @param [in] nowMs 单调时间
    @param [in] timeoutMs 1至60000毫秒
    @param [out] request 生成的完整元数据，失败清空
    @return 与begin相同；编号耗尽时LIMIT_EXCEEDED，不回绕
    */
    int32_t beginNext(uint64_t source, uint64_t target, uint32_t product, uint32_t type, uint64_t parent, uint64_t nowMs,
        uint32_t timeoutMs, NetInterfaceRequest& request);

    /** 从真实目标完成请求，只允许一次；调用方先保证有容量存放待投递结果
    @param [in] target 已验证回复来源
    @param [in] route 网关内部号
    @param [in] product 回复产品，必须与请求一致
    @param [in] type 回复类型，必须与请求一致
    @param [in] nowMs 同一宿主单调时间；到期回复不得先于清理轮次被接受
    @param [out] request 原来源及原请求号，失败清空
    @return READY成功取出；TIMEOUT/CANCELLED取出失效元数据；NOT_FOUND迟到/重复；其他错误清输出
    */
    int32_t complete(uint64_t target, uint64_t route, uint32_t product, uint32_t type, uint64_t nowMs, NetInterfaceRequest& request);

    /** 客户端按网络回显的原请求号完成，仍由C++核对真实目标
    @param [in] source 原本地来源端点
    @param [in] original 网络回显的原请求号
    @param [in] target 已验证回复来源
    @param [in] product 回复产品
    @param [in] type 回复类型
    @param [in] nowMs 单调时间
    @param [out] request 请求记录或清零
    @return 与complete相同
    */
    int32_t completeOriginal(uint64_t source, uint64_t original, uint64_t target, uint32_t product, uint32_t type, uint64_t nowMs,
        NetInterfaceRequest& request);

    /** 按真实来源取消；允许控制帧先于尚未发出的普通请求到达
    @param [in] source 已验证取消来源
    @param [in] original 此来源的原请求号
    @param [out] request 已在途时取出原请求，否则清空
    @return READY取消在途；OK记录未来取消；NOT_FOUND已结束；LIMIT_EXCEEDED超取消容量
    */
    int32_t cancel(uint64_t source, uint64_t original, NetInterfaceRequest& request);

    /** 按最早截止取出一个到期请求，同期限按路由号；调用方每轮限制总处理数量
    @param [in] nowMs 同一宿主单调时间
    @param [out] request 到期对象或清零
    @return READY取出、NOT_FOUND没有到期或时钟错误
    */
    int32_t takeExpired(uint64_t nowMs, NetInterfaceRequest& request);

    /** 一次取出一个直接内部查询，用于父请求完成、取消或失效后的清理
    @param [in] parent 既有或刚移除的父路由号
    @param [out] request 子请求或清零
    @return READY取出或NOT_FOUND
    */
    int32_t takeChild(uint64_t parent, NetInterfaceRequest& request);

    /** 一次取出端点相关的请求，包含根客户端关闭后的内部查询
    @param [in] endpoint 关闭的端点代次
    @param [out] request 相关请求或清零
    @return READY取出或NOT_FOUND；全部取完后才removeEndpoint
    */
    int32_t takeEndpoint(uint64_t endpoint, NetInterfaceRequest& request);

    /** 在途数量
    @return 不超过构造容量
    */
    uint32_t count() const;

    private:
    /** 校验时钟并记录回退错误
    @param [in] nowMs 本轮单调时间
    @return OK或持续错误
    */
    int32_t advance(uint64_t nowMs);

    /** 从各索引及额度中移除一个已存在的请求
    @param [in] route 已存在内部号
    @param [out] request 被取走的元数据
    @return READY或NOT_FOUND
    */
    int32_t take(uint64_t route, NetInterfaceRequest& request);

    /** 禁止复制所有权
    @param [in] other 不可复制的实例
    */
    NetInterfaceRequests(const NetInterfaceRequests& other);

    /** 禁止复制所有权
    @param [in] other 不可复制的实例
    @return 无实现
    */
    NetInterfaceRequests& operator=(const NetInterfaceRequests& other);

    private:
    // 由本DLL创建及释放的独占状态
    NetInterfaceRequestsState* m_state;
};