#pragma once
#include "LibuvTcp.h"
#include "LibuvChannel.h"

// C句柄的唯一所有者，回调和所有方法只允许同一个执行者
class LibuvHostState : private LibuvChannelListener
{
public:
    /** 初始化有效回调表，尚不启动连接
    @param [in] context 借用至停止和销毁完成
    @param [in] connected 非空连接回调
    @param [in] data 非空字节回调
    @param [in] writable 非空可写回调
    @param [in] closed 非空终态回调
    */
    LibuvHostState(void* context, LibuvHostConnected connected, LibuvHostData data,
        LibuvHostWritable writable, LibuvHostClosed closed);

    /** 请求停止，不在外部回调栈上释放
    */
    ~LibuvHostState();

    /** 将自身作为稳定的监听者
    @param [in] address 数值地址
    @param [in] port 监听端口
    @param [in] maxPeers 最大连接数
    @return libuv接纳结果
    */
    int32_t listen(const char* address, uint32_t port, uint32_t maxPeers);

    /** 将自身作为稳定的监听者
    @param [in] address 数值地址
    @param [in] port 目标端口
    @param [out] peer 连接号
    @return libuv接纳结果
    */
    int32_t connect(const char* address, uint32_t port, uint64_t& peer);

private:
    /** 转交连接事件并防止异常跨过底层回调
    @param [in] peer 连接代次
    */
    virtual void onConnected(uint64_t peer);

    /** 转交借用字节，不扩大借用期
    @param [in] peer 连接代次
    @param [in] data 借用字节
    @param [in] size 字节数
    */
    virtual void onData(uint64_t peer, const uint8_t* data, uint32_t size);

    /** 转交本地写完成
    @param [in] peer 连接代次
    */
    virtual void onWritable(uint64_t peer);

    /** 转交唯一终态
    @param [in] peer 连接代次
    @param [in] reason 终态原因
    */
    virtual void onClosed(uint64_t peer, int32_t reason);

    /** 禁止复制拥有的循环
    @param [in] other 原实例
    */
    LibuvHostState(const LibuvHostState& other);

    /** 禁止复制拥有的循环
    @param [in] other 原实例
    @return 不可调用
    */
    LibuvHostState& operator=(const LibuvHostState& other);

public:
    // 唯一字节循环，先显式停止再销毁
    LibuvChannel m_channel;
    // 借用回调上下文至停止完成
    void* m_context;
    // 有效事件表，构造时复制
    LibuvHostConnected m_connected;
    // 仅回调期间借用输入
    LibuvHostData m_data;
    // 本地写完成事件
    LibuvHostWritable m_writable;
    // 唯一连接终态事件
    LibuvHostClosed m_closed;
    // 非零时禁止destroy/poll
    uint32_t m_depth;
    // 拒绝底层驱动重入
    bool m_polling;
    // 停止后不再接纳启动或写操作
    bool m_stopping;
};