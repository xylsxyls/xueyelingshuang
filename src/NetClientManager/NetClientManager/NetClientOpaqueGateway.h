#pragma once
#include "NetSender/NetSenderAPI.h"
#include "NetWork/NetWorkAPI.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

typedef NetGatewayAddress NetClientGatewayAddress;

class NetClientOpaqueGatewayPeer;

// NTW3业务流完全透传；另有独立CLIENT管理连接发现地址，不持有业务TLS密钥。
class NetClientOpaqueGateway : private NetSenderLocalRawListener
{
    friend class NetClientOpaqueGatewayPeer;
public:
    /** 创建未启用透明网关
    */
    NetClientOpaqueGateway();

    /** 同一执行者收敛TCP、IPC及旧ProcessWork回调
    */
    ~NetClientOpaqueGateway();

    /** 首次启动前替换实际连接对象池
    @param [in] config 物理槽配置，失败保持原池
    @return NI_OK或状态、内存、配置错误
    */
    int32_t configurePeerPool(const LockFreeHashMapPoolConfig& config);

    /** 注册本机收件箱并配置数值入口种子，不建立假就绪业务会话
    @param [in] addresses 非空数值地址候选列表，数量由实际存储决定，端点仍用内置CA验证身份
    @param [in] cachePath 本地目录缓存路径，空字符串禁用持久化
    @return NI_OK或错误
    */
    int32_t init(const std::vector<NetClientGatewayAddress>& addresses, const std::string& cachePath = std::string());

    /** 驱动本机/远端窗口，不重放失败连接中的任何记录
    @return 是否仍启用
    */
    bool poll();

    /** 停止所有所属资源
    */
    void shutdown();

private:
    /** 为本机新代次选择入口并创建独立TCP运输
    @param [in] peer 本机运输代次
    @param [in] pid 下层PID，仅寻址，不作为授权
    */
    virtual void onRawLocalReady(uint64_t peer, int32_t pid);

    /** 将本机完整透明记录提交给对应远端运输
    @param [in] peer 本机运输代次
    @param [in] record 回调期间借用，接纳后下层持有副本
    @return false表示远端尚未就绪或有界背压，true表示已消费或关闭该代
    */
    virtual bool onRawLocalRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 标记本机代次终止并关闭对应TCP，回调退出后由poll回收
    @param [in] peer 本机运输代次
    @param [in] reason 下层终态原因
    */
    virtual void onRawLocalClosed(uint64_t peer, int32_t reason);

    /** 失效本机代次并推进入口选择，新端点须重新建立TLS
    @param [in] local 本机代次
    @param [in] reason 原因
    */
    void remoteClosed(uint64_t local, int32_t reason);

    /** 禁止复制包含活动运输与回调地址的网关
    @param [in] other 不可复制的源对象
    */
    NetClientOpaqueGateway(const NetClientOpaqueGateway& other);

    /** 禁止赋值活动网关
    @param [in] other 不可赋值的源对象
    @return 不可调用
    */
    NetClientOpaqueGateway& operator=(const NetClientOpaqueGateway& other);

private:
    // ProcessWork回调收件箱，停止接收并收敛回调后才销毁
    NetSenderLocalInbox m_inbox;
    // 借用收件箱的本机透明记录运输，不解密业务TLS
    NetSenderLocal m_local;
    // 独占的可信入口发现与缓存管理，只由本执行者推进
    NetGatewayDiscovery m_discovery;
    // 实际连接对象池，覆盖关闭回调及远端运输析构
    std::unique_ptr<NetObjectPool<NetClientOpaqueGatewayPeer>> m_peerPool;
    // 本机代次到独占远端运输，每个业务端点独立TLS字节流
    std::map<uint64_t, NetObjectReference<NetClientOpaqueGatewayPeer>> m_peers;
    // 已登记本对象的ProcessWork回调，shutdown清理登记及随后取得的接收资源
    bool m_receiving;
    // 本机监听已成功启用，可由poll推进
    bool m_started;
    // 停止只提交一次，后续poll仅收敛目录缓存后台工作
    bool m_stopping;
};