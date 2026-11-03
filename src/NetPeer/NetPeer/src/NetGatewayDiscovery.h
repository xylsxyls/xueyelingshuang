#pragma once
#include "NetLinkTransport.h"
#include "NetGatewayDirectoryStore.h"
#include "NetInterface/NetInterfaceAPI.h"

struct NetGatewayDiscoveryState;

// 所有NetPeer宿主共用的管理发现连接；同一执行者串行调用，禁止并发和回调重入
class NetPeerAPI NetGatewayDiscovery : public NetClientDiscovery, private NetLinkTransportListener
{
public:
    /** 创建静止状态，不创建线程或连接
    */
    NetGatewayDiscovery();

    /** 调用者先shutdown并poll至stopped，且停止借用本发现器的会话
    */
    ~NetGatewayDiscovery();

    /** 启动有界发现，宿主在网络执行者初始化；缓存路径空时仅内存缓存
    @param [in] seeds 1至64个数值入口
    @param [in] cachePath 可空UTF8路径，非空需要已配置文件后端，磁盘操作仅在后端
    @param [in] tls 仅采用客户端熵/期限；空熵使用显式字节宿主，0期限默认5000毫秒
    @return NI_OK或错误，不代表已认证远端；同一执行者调用，失败可shutdown收敛
    */
    int32_t start(const std::vector<NetGatewayAddress>& seeds, const std::string& cachePath = std::string(),
        const NetInterfaceTlsOptions& tls = NetInterfaceTlsOptions());

    /** 每次推进有界管理及缓存工作，不等待网络或文件；同一执行者禁止重入
    @return 是否仍有发现资源
    */
    bool poll();

    /** 同一执行者停止管理连接和缓存，随后继续poll至stopped
    */
    void shutdown();

    /** 查询管理运输及可选缓存后端是否已停止
    @return true后才允许销毁，停止后继续poll推进缓存回收
    */
    virtual bool stopped() const;

    /** 选择尚未退避的地址提示；真实连接仍须管理认证
    @param [out] endpoint 调用者预填v1 size/version；失败内容不可使用
    @return 是否有可尝试候选
    */
    bool choose(NetClientDiscoveryPoint& endpoint);

    /** 记录一次连接失败，不清空其余备用地址
    @param [in] endpoint 失败地址，必须属于既有候选；等价IP写法共用退避
    */
    void failed(const NetClientDiscoveryPoint& endpoint);

    /** 已认证发现列表中的成员数，仅用于状态观察
    @return 当前完整发现表大小；启动缓存不算本轮认证证据
    */
    uint32_t discovered() const;

    /** 同模块/静态兼容解析入口；跨DLL使用NI固定值parseSeeds，不转交STL释放权
    @param [in] text 非空ASCII列表，最多4096字节
    @param [out] points 成功去重候选；失败清空
    @return NI_OK、NI_INVALID_ARGUMENT或NI_NO_MEMORY；允许text引用points中的字符串
    */
    static int32_t parseSeeds(const std::string& text, std::vector<NetGatewayAddress>& points);
private:
    /** 验证当前管理代次的节点角色后允许请求目录
    @param [in] peer 管理代次
    @param [in] identity 已验证的链路角色
    */
    virtual void onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity);

    /** 仅接受当前唯一目录请求的响应，完整校验后替换候选
    @param [in] peer 管理代次
    @param [in] frame 借用至返回的帧
    @return 已消费帧，包括拒绝并关闭非法来源
    */
    virtual bool onLinkControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 发现连接禁止传送业务密文，收到即关闭
    @param [in] peer 管理代次
    @param [in] record 被拒绝的记录
    @return 已消费
    */
    virtual bool onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 标记当前运输待收敛，运行中才记录退避
    @param [in] peer 管理代次
    @param [in] reason 关闭原因
    */
    virtual void onLinkClosed(uint64_t peer, int32_t reason);

    /** 禁止复制独占管理及异步文件资源
    @param [in] other 原对象
    */
    NetGatewayDiscovery(const NetGatewayDiscovery& other);

    /** 禁止复制独占管理及异步文件资源
    @param [in] other 原对象
    @return 不可调用
    */
    NetGatewayDiscovery& operator=(const NetGatewayDiscovery& other);
    // 本模块创建和释放的发现状态
    NetGatewayDiscoveryState* m_state;
};