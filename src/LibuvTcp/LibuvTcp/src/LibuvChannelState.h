#pragma once
#include "LibuvChannel.h"
#include "LibuvChannelModels.h"
#include <map>

/** 独占原始TCP循环、监听及连接对象，全部操作由同一执行者串行驱动
*/
class LibuvChannelState
{
public:
    /** 初始化专属uv循环，实际结果由m_loopStatus保存，不以异常表达普通失败
    */
    LibuvChannelState();

    /** 关闭监听和连接，驱动已取消写入的最终回调后释放循环
    */
    ~LibuvChannelState();

    /** 解析数值地址，不接触系统DNS
    @param [in] address 数值地址
    @param [in] port 端口
    @param [out] result 地址结构
    @return 0成功或libuv错误
    */
    int32_t parseAddress(const char* address, uint32_t port, sockaddr_storage& result);

    /** 初始化并登记一个拥有的连接
    @param [in] rejectOnly 仅接收后拒绝，不发业务回调
    @return 成功连接；分配或uv初始化失败返回null
    */
    LibuvChannelPeer* createPeer(bool rejectOnly);

    /** 开始读取固定缓冲
    @param [in] peer 活动连接
    @return 0成功或libuv错误
    */
    int32_t startRead(LibuvChannelPeer* peer);

    /** 幂等关闭，最终回调负责释放对象
    @param [in] peer 拥有的连接
    @param [in] reason 终态原因
    */
    void closePeer(LibuvChannelPeer* peer, int32_t reason);

    /** 关闭全部句柄并禁止新操作，回调仍在当前执行者结束
    */
    void shutdown();

    /** 接受或拒绝新TCP连接
    @param [in] server 本实例监听句柄
    @param [in] status libuv结果
    */
    static void acceptPeer(uv_stream_t* server, int status);

    /** 异步连接完成
    @param [in] request 连接请求
    @param [in] status libuv结果
    */
    static void connected(uv_connect_t* request, int status);

    /** 提供固定接收内存
    @param [in] handle TCP句柄
    @param [in] suggestedSize libuv建议，不作为分配依据
    @param [out] buffer 本连接16KiB缓冲
    */
    static void allocateRead(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buffer);

    /** 接收真实TCP字节或结束事件
    @param [in] stream TCP流
    @param [in] size 实际字节或负数错误
    @param [in] buffer 借用缓冲
    */
    static void read(uv_stream_t* stream, ssize_t size, const uv_buf_t* buffer);

    /** 底层写完成，释放额度和写入副本
    @param [in] request 拥有的写请求
    @param [in] status libuv结果
    */
    static void written(uv_write_t* request, int status);

    /** 句柄及所有在途写回调结束，释放连接并上报终态
    @param [in] handle 关闭句柄
    */
    static void closed(uv_handle_t* handle);

public:
    // 0表示循环有效，负数表示初始化失败，失败循环不得驱动或关闭
    int32_t m_loopStatus;
    // 本实例独占的事件循环，仅在所属执行者推进
    uv_loop_t m_loop;
    // 监听句柄，仅m_listenerInitialized为true后可以关闭
    uv_tcp_t m_listener;
    // 借用的事件接收者，必须存活至全部关闭回调结束
    LibuvChannelListener* m_callback;
    // 拥有的连接，包含异步关闭中对象，最终由closed删除
    std::map<uint64_t, LibuvChannelPeer*> m_peers;
    // 下次分配的连接号，0表示单调编号已耗尽，不再复用
    uint64_t m_nextPeer;
    // 登记连接数上限，包含正在关闭的连接
    uint32_t m_maxPeers;
    // 服务器实际监听端口，未监听成功为0
    uint32_t m_port;
    // 实例是否已提交一次启动，启动失败也不能复用实例
    bool m_started;
    // 监听句柄是否完成uv初始化
    bool m_listenerInitialized;
    // 已进入停止状态，不再接纳或发起连接
    bool m_stopping;
    // 正处于循环或析构排空中，防止回调重入驱动循环
    bool m_polling;
    // 最多一个未登记的拒绝句柄等待关闭，避免连接洪峰无限创建临时对象
    bool m_rejectPending;
    // libuv的接纳就绪尚未消费；分配失败后仅由下一轮poll重试，关闭回调不能递归接纳
    bool m_acceptPending;
};