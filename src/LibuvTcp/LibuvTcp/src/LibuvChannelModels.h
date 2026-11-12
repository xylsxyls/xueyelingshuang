#pragma once
#include "uv.h"
#include <stdint.h>
#include <vector>

class LibuvChannelState;

/** 由循环拥有的单条连接，最终关闭回调统一回收
*/
struct LibuvChannelPeer
{
public:
    // 拥有的TCP句柄，close回调结束前不可释放所在对象
    uv_tcp_t m_tcp;
    // 连接请求存储，与TCP句柄共享对象生命周期
    uv_connect_t m_connect;
    // 借用的所属循环，生命周期覆盖全部句柄回调
    LibuvChannelState* m_owner;
    // 实例内永不复用的连接号，0表示拒绝槽位
    uint64_t m_id;
    // 已交给uv_write而尚未完成的字节数，上限来自所属实例配置
    uint32_t m_queuedBytes;
    // 在途写请求数，完成回调逐一归还，上限来自所属实例配置
    uint32_t m_queuedWrites;
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
    // 写请求存储，data回指拥有它的LibuvChannelWrite
    uv_write_t m_request;
    // 借用的连接对象，libuv保证全部写回调先于连接关闭回调
    LibuvChannelPeer* m_peer;
    // 拥有的待写字节副本，保留至写完成或取消回调
    std::vector<uint8_t> m_bytes;
    // 仅空闲缓存期间有效；在途请求不挂入链表，不与其他执行者共享
    LibuvChannelWrite* m_next;

public:
    /** 创建空闲写请求；prepareWrite复制正文后才能交给libuv
    */
    LibuvChannelWrite();
};