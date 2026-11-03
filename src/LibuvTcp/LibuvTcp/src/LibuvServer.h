#pragma once
#include "LibuvBase.h"

/** 兼容服务端的监听存储所有者，接纳连接由各自关闭回调回收
*/
struct LibuvServer : public LibuvBase
{
public:
    // 拥有的监听句柄，循环排空后由析构释放
    uv_tcp_t *m_server;

public:
    /** 分配尚未uv初始化的监听存储，不开始监听
    */
    LibuvServer();

    /** 所有uv回调结束后释放监听句柄
     */
    ~LibuvServer();
};