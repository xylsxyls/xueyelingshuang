#pragma once
#include "libuv/uv.h"
#include <stdint.h>
#include <vector>
#include <atomic>
#include <memory>

class LibuvChannelState;
struct LibuvChannelWrite;

/** 由循环拥有的单条连接，最终关闭回调统一回收
*/
struct LibuvChannelPeer
{
public:
    // 实际连接池槽，全部libuv回调完成后才归还
    void* m_storage;
    // 本连接已接纳但尚未交给uv_write的FIFO，正文由实际写池拥有
    LibuvChannelWrite* m_postFirst;
    // 待提交FIFO尾，唯一loop执行者维护
    LibuvChannelWrite* m_postLast;
    // 就绪连接双向链前项，支持关闭时常数时间摘除
    LibuvChannelPeer* m_readyPrevious;
    // 就绪连接双向链后项，各连接轮流提交一个批次
    LibuvChannelPeer* m_readyNext;
    // 是否已在所属通道的就绪链中
    bool m_readyQueued;
    // 拥有的TCP句柄，close回调结束前不可释放所在对象
    uv_tcp_t m_tcp;
    // 连接请求存储，与TCP句柄共享对象生命周期
    uv_connect_t m_connect;
    // 借用的所属循环，生命周期覆盖全部句柄回调
    LibuvChannelState* m_owner;
    // 实例内永不复用的连接号，登记失败的句柄在关闭前置零
    uint64_t m_id;
    // 首次关闭原因，后续重复关闭不能覆盖
    int32_t m_reason;
    // TCP连接已成功建立，尚不代表上层完成认证
    bool m_connected;
    // 已提交异步关闭，此后不再写入或重复关闭
    bool m_closing;
    // 是否暂停读取，恢复时重新提交uv_read_start
    bool m_paused;
    // 连接号已返回调用方或已经发布连接事件，需要配对关闭回调
    bool m_reportClose;
    // 本连接按配置拥有的读缓冲，不按libuv建议尺寸临时分配
    uint8_t* m_input;

public:
    /** 初始化纯状态，uv_tcp_t随后由uv_tcp_init初始化
    @param [in] owner 生命周期覆盖全部句柄回调
    @param [in] id 不复用的连接号；0为超限后仅用于拒绝的连接
    */
    LibuvChannelPeer(LibuvChannelState* owner, uint64_t id);

    /** 所有网络回调结束后释放本连接的独占读缓冲
    */
    ~LibuvChannelPeer();
};

/** 单次uv_write请求及其字节副本；完成后由唯一循环执行者回收或有界缓存
*/
struct LibuvChannelWrite
{
public:
    // 提交时仅携带不复用编号，loop验证后才填充m_peer
    uint64_t m_peerId;
    // 本次有效字节数，与连接号和正文开头相邻，减少逐条消费的缓存访问
    uint32_t m_size;
    // 小正文直接保存在请求槽，避免每包额外堆分配
    uint8_t m_inline[256];
    // 实际写请求池槽，缓存期间同样占有该槽
    void* m_storage;
    // 借用的连接对象，libuv保证全部写回调先于连接关闭回调
    LibuvChannelPeer* m_peer;
    // 拥有的待写字节副本，保留至写完成或取消回调
    std::vector<uint8_t> m_bytes;
    // 写请求存储，data回指拥有它的LibuvChannelWrite
    uv_write_t m_request;

    /** 取得本次独占正文，存活到写完成或取消
    @return 有效字节首地址，长度为m_size
    */
    uint8_t* data();
    // 互斥用于空闲链、待提交FIFO或同一uv_write批次链；仅唯一owner访问
    LibuvChannelWrite* m_next;

public:
    /** 创建空闲写请求；prepareWrite复制正文后才能交给libuv
    */
    LibuvChannelWrite();
};

/** 固定生产者和唯一loop之间的有序通道；记录来自实际写池，不额外扩大接纳预算
*/
struct LibuvChannelLane
{
public:
    // 不复用的生产者令牌，0允许首次领取；独立于频繁更新的发布位置
    std::atomic<uint32_t> m_token;
    // 启动时固定的槽数，0禁用，否则为2次幂
    uint32_t m_capacity;
    // 每条记录持续占有写池槽，关闭后统一归还；启动后数组地址不变
    std::unique_ptr<LibuvChannelWrite*[]> m_items;
    // 只读身份和配置不与发布位置共用缓存行，避免其他生产者读主通道身份时争用
    char m_identityPadding[128];
    // 已发布记录尾，消费者只读取
    std::atomic<uint64_t> m_published;
    // 写入侧位置，仅令牌拥有者访问
    uint64_t m_write;
    // 最近观察到的消费位置
    uint64_t m_seenRead;
    // 最近验证通过的连接编号，不存跨线程TCP指针
    uint64_t m_peer;
    // 连接快照代次，连接集合改变后必须重新验证
    uint64_t m_version;
    // 隔离写入和消费热点
    char m_padding[128];
    // 该通道已经安排唤醒；loop先撤销通知再观察已发布记录，防止遗漏空转非空的边界
    std::atomic<bool> m_notified;
    // 正文已复制或转移给loop的记录尾，生产者据此复用槽
    std::atomic<uint64_t> m_consumed;
    // 唯一loop的消费位置
    uint64_t m_read;
    // 消费者更新不与下一个通道的只读身份共用缓存行
    char m_consumerPadding[128];

public:
    /** 创建未分配通道
    */
    LibuvChannelLane();
};