#pragma once
#include "LibuvChannel.h"
#include "LibuvChannelModels.h"
#include <unordered_map>

/** 独占原始TCP循环、监听及连接对象，全部操作由同一执行者串行驱动
*/
class LibuvChannelState
{
public:
    /** 保存默认参数，尚不初始化uv循环或连接资源
    */
    LibuvChannelState();

    /** 关闭监听和连接，驱动已取消写入的最终回调后释放循环
    */
    ~LibuvChannelState();

    /** listen/connect参数检查后初始化循环，失败可再次尝试
    @return 0成功，否则libuv错误
    */
    int32_t initialize();

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

    /** 唯一执行者取得空闲请求并复制正文，尚未提交给libuv
    @param [in] peer 本循环连接，成功请求持有借用至写回调
    @param [in] data 非空且借用至返回的输入，不能指向本缓存
    @param [in] size 有效字节数，调用者已检查配置写入上限和在途额度
    @return 准备好的独占请求；分配失败为nullptr，无网络副作用
    */
    LibuvChannelWrite* prepareWrite(LibuvChannelPeer* peer, const uint8_t* data, uint32_t size);

    /** 唯一执行者回收未提交或已完成回调的请求，不得回收仍在途的请求
    @param [in] request 本循环独占请求，可空；调用后不得继续访问
    */
    void releaseWrite(LibuvChannelWrite* request);

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
    @param [out] buffer 本连接按配置分配的独占读缓冲
    */
    static void allocateRead(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buffer);

    /** 接收真实TCP字节或结束事件
    @param [in] stream TCP流
    @param [in] size 实际字节或负数错误
    @param [in] buffer 借用缓冲
    */
    static void read(uv_stream_t* stream, ssize_t size, const uv_buf_t* buffer);

    /** 底层写完成，归还在途额度，再回收请求和缓冲供唯一执行者复用
    @param [in] request 拥有的写请求
    @param [in] status libuv结果
    */
    static void written(uv_write_t* request, int status);

    /** 句柄及所有在途写回调结束，释放连接并上报终态
    @param [in] handle 关闭句柄
    */
    static void closed(uv_handle_t* handle);

public:
    // 创建实例只保存参数，listen/connect成功初始化循环后才置true
    bool m_loopInitialized;
    // 原始字节写预算和空闲缓存参数，启动后不可变
    LibuvChannelWritePoolConfig m_writeConfig;
    // 每个连接拥有的读缓冲字节数，建立连接时按需分配
    uint32_t m_readBufferBytes;
    // 最近一次循环初始化状态；还需结合m_loopInitialized判断能否驱动
    int32_t m_loopStatus;
    // 本实例独占的事件循环，仅在所属执行者推进
    uv_loop_t m_loop;
    // 监听句柄，仅m_listenerInitialized为true后可以关闭
    uv_tcp_t m_listener;
    // 借用的事件接收者，必须存活至全部关闭回调结束
    LibuvChannelListener* m_callback;
    // 唯一执行者拥有的连接索引，不依赖键顺序；关闭中的对象最终由closed删除
    std::unordered_map<uint64_t, LibuvChannelPeer*> m_peers;
    // 已完成写请求的空闲链，唯一执行者维护；shutdown立即清空，不能缓存仍在途请求
    LibuvChannelWrite* m_freeWrites;
    // 当前空闲请求数，不超过m_writeConfig中的数量上限
    uint32_t m_cachedWrites;
    // 空闲vector实际capacity合计，不超过配置字节上限；对象数另受数量限制
    size_t m_cachedWriteBytes;
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