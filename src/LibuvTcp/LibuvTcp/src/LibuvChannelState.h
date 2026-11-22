#pragma once
#include "LibuvChannelModels.h"
#include "LibuvHostLoop.h"
#include "LibuvChannel.h"
#include "LibuvBase.h"
#include <unordered_map>
#include <memory>
#include <thread>
#include "LockFreeHashMap/LockFreeHashMapAPI.h"

/** 独占原始TCP循环、监听及连接对象；post可并发，其余操作由唯一loop执行者驱动
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

    /** 取得独立或共同循环，仅本执行者调用
    @return 已初始化循环的借用指针
    */
    uv_loop_t* loop();

    /** 收敛本通道关闭回调，不等待共同循环中的其他活动通道
    */
    void drain();

    /** 记录协议可继续推进的真实进展，不启动额外线程
    */
    void activity();

    /** 启动前从预配置写池保留生产者通道，同时为合批和后备路径留出容量
    @return 0成功，负数分配失败；小池不启用通道
    */
    int32_t prepareLanes();

    /** 提交入口关闭且网络收敛后归还通道记录
    */
    void releaseLanes();

    /** 获取本生产者固定通道，碰撞者走后备队列，不等待其他生产者
    @return 本线程独占通道或空
    */
    LibuvChannelLane* producerLane();

    /** 查询并发连接索引，正常命中不创建拥有值的Snapshot
    @param [in] peer 本实例不复用的连接号
    @return 0仍连接，UV_ENOTCONN不存在，UV_ENOMEM查询保护资源不足
    */
    int32_t connectedStatus(uint64_t peer) const;

    /** 复制并发布一条消息，成功后不得再访问该槽
    @param [in,out] lane 本线程独占通道
    @param [in] peer 不复用连接号
    @param [in] data 借用至返回的正文
    @param [in] size 已验证的字节数
    @return 0接纳，负数未接纳
    */
    int32_t postLane(LibuvChannelLane& lane, uint64_t peer, const uint8_t* data, uint32_t size);

    /** loop有界轮转，小正文合批、大正文转移所有权后才归还通道槽
    @return 尚有待处理数据且本轮有进展时为true；满池无进展时等资源事件唤醒
    */
    bool drainLanes();

    /** 监听关闭完成后归还句柄存储
    @param [in] handle 本实例监听句柄
    */
    static void listenerClosed(uv_handle_t* handle);

    /** 池耗尽时拒绝句柄完成关闭，后续可复用同一嵌入存储
    @param [in] handle 本实例拒绝句柄
    */
    static void overflowClosed(uv_handle_t* handle);

    /** 解析数值地址，不接触系统DNS
    @param [in] address 数值地址
    @param [in] port 端口
    @param [out] result 地址结构
    @return 0成功或libuv错误
    */
    int32_t parseAddress(const char* address, uint32_t port, sockaddr_storage& result);

    /** 初始化并登记一个拥有的连接
    @return 成功连接；分配或uv初始化失败返回null
    */
    LibuvChannelPeer* createPeer();

    /** 归还实际连接槽；只能在句柄未初始化或最终关闭回调后调用
    @param [in] peer 本实例拥有的记录，可空
    */
    void releasePeer(LibuvChannelPeer* peer);

    /** 多生产者从实际池取得请求并复制正文，尚未发布
    @param [in] peer 不复用的连接号，不借用loop对象
    @param [in] data 非空且借用至返回的输入，不能指向本缓存
    @param [in] size 有效字节数，调用者已检查配置写入上限和在途额度
    @param [out] result 0成功，失败区分池满与系统分配失败
    @return 准备好的独占请求；分配失败为nullptr，无网络副作用
    */
    LibuvChannelWrite* prepareWrite(uint64_t peer, const uint8_t* data, uint32_t size, int32_t& result);

    /** 拥有者回收未提交或已完成回调的请求，不得回收已交接或仍在途的请求
    @param [in] request 本循环独占请求，可空；调用后不得继续访问
    */
    void releaseWrite(LibuvChannelWrite* request);

    /** 唯一owner将已复制请求挂入连接待写链，不在这里调用uv_write
    @param [in] peer 仍存活且未关闭的本循环连接
    @param [in] request 独占请求，交接后调用者不得再访问
    */
    void appendWrite(LibuvChannelPeer* peer, LibuvChannelWrite* request);

    /** loop先收集连接编号再派发容量恢复，允许回调新建或关闭连接
    @param [in] exclude 已由当前写完成回调单独通知的连接号，0不排除
    */
    void notifyCapacity(uint64_t exclude = 0);

    /** 将有待提交正文的连接追加到就绪FIFO，已登记时无操作
    @param [in] peer 本owner拥有的未关闭连接
    */
    void queueReady(LibuvChannelPeer* peer);

    /** 摘除就绪连接，关闭和批量消费均先解除登记
    @param [in] peer 本owner拥有的连接
    */
    void removeReady(LibuvChannelPeer* peer);

    /** 归还同一连接已完成或取消的整条请求链，不执行业务回调
    @param [in] first 链头；返回后不得访问，允许空
    */
    void releaseBatch(LibuvChannelWrite* first);

    /** loop中轮流消费连接FIFO，将同连接字节合并交给uv_write
    @param [in] handle 本通道唤醒句柄；只在所属loop执行
    */
    static void posted(uv_async_t* handle);

    /** 提交唤醒句柄最终关闭，允许通道存储回收
    @param [in] handle 本通道已关闭的唤醒句柄
    */
    static void postClosed(uv_handle_t* handle);

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
    // 初始化时发布的唯一loop线程身份，生命周期内不迁移
    std::thread::id m_ownerThread;
    // 初始化执行者的不复用线程令牌；耗尽时才回退系统线程身份查询
    uint32_t m_ownerToken;
    // 多生产者共享提交借用；shutdown封闭后才关闭uv_async句柄
    LibuvLifecycleGate m_submissionGate;
    // producer只查询编号快照，禁止跨线程读取m_peers或TCP对象
    std::unique_ptr<LockFreeHashMap<uint64_t, uint32_t>> m_connectedPeers;
    // 多生产者发布、唯一loop消费的原始字节描述符
    std::unique_ptr<LockFreeQueue<LibuvChannelWrite*>> m_incoming;
    // 池压力通知由loop统一派发，不让生产者修改等待链
    std::atomic<bool> m_notifyCapacity;
    // 接纳后唤醒失败只置终态，不把已转交的正文伪报为未接纳
    std::atomic<int32_t> m_submissionError;
    // 连接集合变化使生产者快照失效
    std::atomic<uint64_t> m_connectionVersion;
    // 从实际写池初始容量划分的专用通道
    std::unique_ptr<LibuvChannelLane[]> m_lanes;
    // 已配置通道数，0表示公共队列
    uint32_t m_laneCount;
    // 唯一loop下一次轮转起点
    uint32_t m_laneCursor;
    // 原始字节写请求、提交队列及连接索引池参数，启动后不可变
    std::unique_ptr<LibuvChannelWritePoolConfig> m_writeConfig;
    // 连接实际存储，在关闭中也保持占槽，不按活跃数量重复限流
    std::unique_ptr<LockFreeHashMapPool<LibuvChannelPeer>> m_peerPool;
    // 写入记录的实际存储，在途请求占槽，完成或取消后归还
    std::unique_ptr<LockFreeHashMapPool<LibuvChannelWrite>> m_writePool;
    // 池构造及最近写入准备错误，区分真实满池与系统内存失败
    int32_t m_peerPoolStatus;
    // 写记录池的初始化结果，可通过启动前重新配置恢复
    int32_t m_writePoolStatus;
    // 每个连接拥有的读缓冲字节数，建立连接时按需分配
    uint32_t m_readBufferBytes;
    // 最近一次循环初始化状态；还需结合m_loopInitialized判断能否驱动
    int32_t m_loopStatus;
    // 本实例独占的事件循环，仅在所属执行者推进
    uv_loop_t m_loop;
    // 可空的共同循环，initialize借用至本通道最终析构
    LibuvHostLoop* m_group;
    // 监听句柄，仅m_listenerInitialized为true后可以关闭
    uv_tcp_t m_listener;
    // 无需分配的拒绝句柄，只在实际连接池耗尽时接收并关闭新连接
    uv_tcp_t m_overflow;
    // 拒绝句柄已初始化且仍在等待关闭回调
    bool m_overflowClosing;
    // 借用的事件接收者，必须存活至全部关闭回调结束
    LibuvChannelListener* m_callback;
    // 唯一执行者拥有的连接索引，不依赖键顺序；关闭中的对象最终由closed删除
    std::unordered_map<uint64_t, LibuvChannelPeer*> m_peers;
    // 原始字节投递唤醒；自身不保活，活动TCP句柄维持loop生命周期
    uv_async_t m_postAsync;
    // 唤醒句柄从初始化成功保持到最终关闭回调
    bool m_postInitialized;
    // 有待提交正文的连接FIFO头，每轮每连接提交一批
    LibuvChannelPeer* m_readyFirst;
    // 有待提交正文的连接FIFO尾，单owner维护
    LibuvChannelPeer* m_readyLast;
    // 下次分配的连接号，0表示单调编号已耗尽，不再复用
    uint64_t m_nextPeer;
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
    // 连接索引登记失败，未登记的TCP句柄仍等待最终关闭回调
    bool m_rejectPending;
    // libuv的接纳就绪尚未消费；关闭后由下一次异步回调或poll重试，不能在关闭栈递归接纳
    bool m_acceptPending;
};