#pragma once
#include "LibuvBase.h"

/** 兼容客户端的TCP和连接请求所有者，析构前必须完成循环排空
*/
struct LibuvClient : public LibuvBase
{
public:
    // 拥有的客户端TCP，全部uv回调结束后才释放
    uv_tcp_t *m_dest;
    // 拥有的连接请求，与客户端上下文共同存活到回调结束
    uv_connect_t *m_connect;

public:
    /** 分配尚未uv初始化的客户端句柄存储，不启动网络
    @param [in] settings 已检查的初始化配置，构造时复制
    */
    explicit LibuvClient(const LibuvPoolSettings& settings = LibuvPoolSettings());

    /** 所有uv回调结束后释放主TCP及连接请求
     */
    ~LibuvClient();
};