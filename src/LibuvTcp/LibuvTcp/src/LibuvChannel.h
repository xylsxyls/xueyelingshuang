#pragma once
#include "LibuvTcpMacro.h"
#include <stdint.h>

class LibuvChannelState;

// All callbacks run in the thread that calls poll. The listener must outlive close.
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
    @param [in] size 1至16384字节
    */
    virtual void onData(uint64_t peer, const uint8_t* data, uint32_t size) = 0;

    /** 一次底层写完成后释放额度；不代表业务接收确认
    @param [in] peer 连接号
    */
    virtual void onWritable(uint64_t peer) = 0;

    /** 连接资源已回收，终态最多一次；没有旧指针供外部复用
    @param [in] peer 连接号
    @param [in] reason libuv错误或主动关闭时的原因码，0为主动关闭
    */
    virtual void onClosed(uint64_t peer, int32_t reason) = 0;
};

// Raw TCP transport for NetInterface. No implicit length prefix, compression or
// background thread. Existing LibuvTcp behavior remains available to old callers.
class LibuvTcpAPI LibuvChannel
{
public:
    /** 创建独占事件循环；普通初始化失败由listen/connect返回libuv错误
    */
    LibuvChannel();

    /** 在所属执行者结束所有回调并释放句柄；禁止在回调中销毁自身
    */
    ~LibuvChannel();

    /** 监听数值IPv4/IPv6地址，不执行阻塞域名解析
    @param [in] address 非空数值地址，显式0.0.0.0/::才监听所有网卡
    @param [in] port 0至65535，0供隔离测试分配临时端口
    @param [in] listener 借用到shutdown完成，不可空
    @param [in] maxPeers 1至256，包含正在关闭的连接
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

    /** 单次非阻塞事件处理，所有操作和回调必须在同一所属执行者；重入不执行循环
    @return 仍有活动句柄为true
    */
    bool poll();

    /** 复制密文字节并异步发送，每次最大32KiB、每连接在途最多64KiB和64次写
    @param [in] peer 有效已连接且未关闭的连接号
    @param [in] data size不为0时不可空
    @param [in] size 字节数，0为无操作
    @return 0已接纳，UV_ENOBUFS为背压，其他libuv错误表示未接纳
    */
    int32_t write(uint64_t peer, const uint8_t* data, uint32_t size);

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

    /** 查询服务器实际监听端口
    @return 监听成功后的端口，其他状态为0
    */
    uint32_t localPort() const;

    /** 查询包含正在关闭连接的数量，用于容量和退出验证
    @return 当前拥有的连接数
    */
    uint32_t peerCount() const;

    /** 判断写入返回值是否为可重试的有界队列背压
    @param [in] result write返回值
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