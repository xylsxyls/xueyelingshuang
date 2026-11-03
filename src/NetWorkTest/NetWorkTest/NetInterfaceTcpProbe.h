#pragma once
#include "LibuvTcp/LibuvTcpAPI.h"
#include <map>
#include <vector>

// Tests drive real TCP from one thread; no replacement socket implementation.
class NetInterfaceTcpProbe : public LibuvChannelListener
{
public:
    /** 创建仅记录真实回调的观察者
    */
    NetInterfaceTcpProbe();

    /** 记录已建立连接
    @param [in] peer 真实连接号
    */
    virtual void onConnected(uint64_t peer);

    /** 拷贝真实接收的字节，不生成成功结果
    @param [in] peer 连接号
    @param [in] data 借用字节
    @param [in] size 真实长度
    */
    virtual void onData(uint64_t peer, const uint8_t* data, uint32_t size);

    /** 记录真实写完成通知
    @param [in] peer 连接号
    */
    virtual void onWritable(uint64_t peer);

    /** 记录关闭次数和原因
    @param [in] peer 连接号
    @param [in] reason 真实错误码
    */
    virtual void onClosed(uint64_t peer, int32_t reason);

public:
    // 当前用例使用的真实运输代次，不以PID替代
    uint64_t m_peer;
    // 实际连接成功回调次数
    uint32_t m_connections;
    // 可写事件观察状态；宿主回调只在有效上下文中调用
    uint32_t m_writable;
    // 只在受控用例中从输入回调抛出异常
    bool m_throwOnData;
    // 本用例已消费的实际输入记录，用于检查内容与顺序
    std::vector<uint8_t> m_received;
    // 本用例观察到的终态记录，用于检查关闭不丢失且不重复
    std::map<uint64_t, uint32_t> m_closed;
    // 各运输代次实际关闭原因，用于检查错误不串到健康连接
    std::map<uint64_t, int32_t> m_reasons;
};