#pragma once
#include "LibuvTcpMacro.h"
#include "LibuvTcpPoolConfig.h"
#include <stdint.h>

class LibuvChannelState;

// 回调在推进所属循环的线程上执行；监听者须活到终态回调返回。
class LibuvTcpAPI LibuvChannelListener
{
public:
    /** 析构监听接口，不拥有连接
    */
    virtual ~LibuvChannelListener();

    /** TCP连接已建立，尚不代表TLS或账号已认证
    @param [in] peer 本实例内永不复用的连接号
    */
    virtual void onConnected(uint64_t peer) = 0;

    /** 原始字节借用至回调返回，宿主须完整消费或复制
    @param [in] peer 连接号
    @param [in] data 非空的借用字节
    @param [in] size 本次字节数，不超过配置读缓冲大小，默认16384
    */
    virtual void onData(uint64_t peer, const uint8_t* data, uint32_t size) = 0;

    /** 在所属循环提示可以重试，触发来源包括本连接写完成及其他连接归还共享容量
    @param [in] peer 可以尝试继续提交的连接号，新的提交仍需检查背压
    */
    virtual void onWritable(uint64_t peer) = 0;

    /** 连接资源已回收，终态最多一次；没有旧指针供外部复用
    @param [in] peer 连接号
    @param [in] reason libuv错误或主动关闭时的原因码，0为主动关闭
    */
    virtual void onClosed(uint64_t peer, int32_t reason) = 0;
};

// 原始TCP字节通道，由所属执行者推进；消息边界和压缩由上层协议负责。
class LibuvTcpAPI LibuvChannel
{
public:
    /** 创建待配置实例；listen/connect才建立事件循环及网络资源
    */
    LibuvChannel();

    /** 在所属执行者结束所有回调并释放句柄；禁止在回调中销毁自身
    */
    ~LibuvChannel();

    /** listen/connect前设置实际写请求池，验证并准备存储；配置和生命周期由唯一执行者调用，post可跨线程
    @param [in] config 有效配置，借用到返回；失败不改变已有配置
    @return 0成功，UV_EINVAL配置无效，UV_ENOMEM分配失败，UV_EBUSY已启动或已停止
    */
    int32_t configureWritePool(const LibuvChannelWritePoolConfig& config);

    /** 启动前配置实际连接记录池；关闭中的句柄持续占槽，单执行者调用
    @param [in] config 真实槽数及增长策略，失败保持原配置
    @return 0或UV_EINVAL、UV_ENOMEM、UV_EBUSY
    */
    int32_t configurePeerPool(const LockFreeHashMapPoolConfig& config);

    /** listen/connect前设置每连接独占读缓冲大小，连接建立时才分配
    @param [in] bufferBytes 1至16777216字节，默认16384，不改变TCP流语义
    @return 0成功，UV_EINVAL非法参数，UV_EBUSY已启动或已停止
    */
    int32_t configureReadBuffer(uint32_t bufferBytes);

    /** 监听数值IPv4/IPv6地址，不执行阻塞域名解析
    @param [in] address 非空数值地址，显式0.0.0.0/::才监听所有网卡
    @param [in] port 0至65535，0供隔离测试分配临时端口
    @param [in] listener 借用到shutdown完成，不可空
    @param [in] maxPeers 非零兼容启动提示，实际连接容量由configurePeerPool决定
    @return 0成功，否则libuv错误；实例仅能启动一次
    */
    int32_t listen(const char* address, uint32_t port, LibuvChannelListener* listener, uint32_t maxPeers = 64);

    /** 异步连接数值地址，TLS域名由上层独立验证
    @param [in] address 数值IPv4/IPv6，不可空
    @param [in] port 1至65535
    @param [in] listener 借用到shutdown完成，不可空
    @param [out] peer 连接号，失败为0；成功只表示接纳连接请求
    @return 0成功，否则libuv错误
    */
    int32_t connect(const char* address, uint32_t port, LibuvChannelListener* listener, uint64_t& peer);

    /** 单次非阻塞事件处理，除post外的操作和回调必须在同一所属执行者；重入不执行循环
    @return 仍有活动句柄为true
    */
    bool poll();

    /** 多生产者并发复制字节并投递，uv_async唤醒loop后合并调用uv_write；不加长度头，写入上限使用初始化前配置；默认单条32KiB，接纳仅由实际共享写请求池决定
    @param [in] peer 有效已连接且未关闭的连接号
    @param [in] data size不为0时不可空，返回后可立即复用
    @param [in] size 字节数，0为无操作
    @return 0仅表示本库拥有副本，不保证送达；UV_ENOBUFS为非阻塞背压，其他libuv错误未接纳
    */
    int32_t post(uint64_t peer, const uint8_t* data, uint32_t size);

    /** 暂停/恢复读取，以连接窗口限制下游积压
    @param [in] peer 有效连接号
    @param [in] paused 是否暂停
    @return 0成功，否则libuv错误
    */
    int32_t pauseRead(uint64_t peer, bool paused);

    /** 请求关闭一个连接，回收完成后触发一次onClosed
    @param [in] peer 连接号；已关闭或不存在时无操作
    @param [in] reason 提交给终态回调的原因
    */
    void close(uint64_t peer, int32_t reason = 0);

    /** 停止接纳新操作并回收全部TCP句柄，在所属工作执行者调用
    回调中调用只发出关闭请求，回调返回后由外层poll继续完成回收
    */
    void shutdown();

    /** 查询能否从外层驱动或销毁通道；同一循环正在执行回调时须先返回外层
    @return 本通道及所属循环均未驱动时为true
    */
    bool canDestroy() const;

    /** 查询服务器实际监听端口
    @return 监听成功后的端口，其他状态为0
    */
    uint32_t localPort() const;

    /** 查询包含正在关闭连接的数量，用于容量和退出验证
    @return 当前拥有的连接数
    */
    uint32_t peerCount() const;

    /** 判断写入返回值是否为可重试的有界队列背压
    @param [in] result post返回值
    @return true应等待poll释放额度，其他错误不应原地重试
    */
    static bool isBackpressure(int32_t result);

private:
    /** 禁止复制事件循环和句柄所有权
    @param [in] other 不能复制的实例
    */
    LibuvChannel(const LibuvChannel& other);

    /** 禁止复制事件循环和句柄所有权
    @param [in] other 不能复制的实例
    @return 不可调用
    */
    LibuvChannel& operator=(const LibuvChannel& other);

private:
    // 独占，隔离uv头文件及内部回调对象
    LibuvChannelState* m_state;
};