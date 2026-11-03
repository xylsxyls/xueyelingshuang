#pragma once
#include "NetSenderLocal.h"
#include <mutex>
#include <vector>

// 接收器归属的唯一阶段；回调登记只在IDLE到REGISTERED/运行阶段之间发生。
enum NetSenderLocalHubPhase
{
    // 没有本Hub回调登记及租约。
    NET_LOCAL_HUB_IDLE,
    // 已登记稳定回调，但init失败或还未取得接收器，允许原地重试。
    NET_LOCAL_HUB_REGISTERED,
    // 锁外执行登记/init期间，拒绝其他生命周期修改。
    NET_LOCAL_HUB_STARTING,
    // Hub拥有接收器；最后租约和收件箱都归还后才停止。
    NET_LOCAL_HUB_MANAGED,
    // 宿主拥有接收器，prepare已登记回调，但接收器不一定已启动。
    NET_LOCAL_HUB_EXTERNAL,
    // 锁外等待自有接收器退出；异常时保持拒绝接纳，不冒充已停止。
    NET_LOCAL_HUB_STOPPING
};

// 模块内稳定ProcessWork回调；拥有接收器租约和有界借用表，不拥有业务收件箱。
class NetSenderLocalHub : public ProcessReceiveCallback
{
public:
    /** 取得模块生命周期入口，不创建接收线程
    @return 稳定实例，首次调用由生命周期owner完成
    */
    static NetSenderLocalHub& instance();

    /** 附加一个收件箱，必须已取得acquire租约；不在此方法登记原IPC回调
    @param [in] inbox 非空，在detach返回前存活，重复附加幂等
    @return OK或负数错误，最多64箱
    */
    int32_t attach(NetSenderLocalInbox* inbox);

    /** 移除借用；外部模式不替宿主停止，管理模式必要时在锁外等待退出
    @param [in] inbox 已收敛协议的箱，允许未附加/空指针
    */
    void detach(NetSenderLocalInbox* inbox);

    /** 取得最多64个接收租约之一；首个管理租约创建接收器
    @return OK、INVALID_STATE未就绪、BUFFER_TOO_SMALL容量不足或有界错误；失败不取得租约
    */
    int32_t acquire();

    /** 宿主在自己的initReceive之前登记稳定回调；重复prepare幂等
    @return OK或错误，不创建接收器；随后由宿主按ProcessWork规范串行启动
    */
    int32_t prepareExternal();

    /** 全部租约/箱归还且宿主uninitReceive后，清除本Hub登记状态
    @return OK或INVALID_STATE，不替宿主关闭接收器
    */
    int32_t externalStopped();

    /** 归还一个租约；生命周期owner调用，不在协议或ProcessWork回调中等待自身
    */
    void release();

    /** 稳定原回调；快速过滤无关类型，仅在有限借用表内分派有界复制
    @param [in] sendPid 原传输PID，不作为业务授权
    @param [in] bytes 回调期间借用的完整NIP2字节
    @param [in] size 字节数
    @param [in] type 原ProcessWork消息类型
    */
    virtual void receive(int32_t sendPid, const char* bytes, int32_t size, MessageType type);

private:
    /** 建立空借用表，不登记回调或创建线程
    */
    NetSenderLocalHub();

    /** 使用者应已显式归还租约，不跨静态析构次序停止其他组件
    */
    ~NetSenderLocalHub();

    /** 共用的登记/启动事务，失败保留已经成功登记的回调供重试
    @param [in] external true只准备宿主模式；false取得管理或已就绪外部租约
    @return OK或负数，不关闭其他组件的接收器
    */
    int32_t prepare(bool external);

    /** 最后租约与借用均退出后，锁外停止仅由本Hub拥有的接收器
    */
    void stopUnused();

    /** 禁止复制稳定回调和接收器所有权
    @param [in] other 原实例
    */
    NetSenderLocalHub(const NetSenderLocalHub& other);

    /** 禁止复制稳定回调和接收器所有权
    @param [in] other 原实例
    @return 不可调用
    */
    NetSenderLocalHub& operator=(const NetSenderLocalHub& other);

private:
    // 只保护归属和借用表；init/uninit都在锁外，防止等待持有本锁的回调。
    std::mutex m_mutex;
    // 最多64个借用收件箱；detach取得本锁后即可证明原回调不再访问该箱。
    std::vector<NetSenderLocalInbox*> m_boxes;
    // 接收器唯一归属，替代四个可互相矛盾的布尔状态。
    NetSenderLocalHubPhase m_phase;
    // 已成功取得而尚未归还的租约数，0至64。
    uint32_t m_clients;
};