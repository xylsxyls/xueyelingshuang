#pragma once
#include "NetSenderService.h"

class NetSenderServiceProxyState;

/** 有界内部服务转发器，不解释业务字段；父子上下文仍由端点路由器验证。
宿主只把需要内部服务0处理的请求交给本对象，其余业务自己处理。
*/
class NetSenderAPI NetSenderServiceProxy
{
public:
    /** 借用一个已初始化的服务对象；全部接口只由同一协议执行者调用，不可跨线程
    @param [in] service 生命周期覆盖本对象；代理不创建线程或关闭服务
    */
    explicit NetSenderServiceProxy(NetSenderService& service);

    /** 清除尚未发送的敏感请求副本，不自动重放
    */
    ~NetSenderServiceProxy();

    /** 复制根请求、内部响应或根取消；正文失败或超预算时不提交状态
    @param [in] frame 已认证服务交付的RPC帧，正文最多1048560字节，借用到返回
    @param [in] origin 网关附加的根来源，响应及取消必须精确匹配
    @return true已处理；false仅为无副作用背压，可用原输入重试
    */
    bool accept(const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin);

    /** 非阻塞推进最多32条；固定槽位合计8MiB正文，外加每槽16字节头
    子请求在发送现场取号；首个子请求背压后不让后继子请求越过，响应及取消仍可推进。
    本层不在poll中分配整包；终止或回调外reset时擦除拥有正文。禁止重入或析构。
    */
    void poll();

    /** 网关连接代次结束时清空全部关联，丢弃旧回复
    */
    void reset();

    /** 检查当前代次状态
    @return false为来源/协议违反契约或服务发送失败，宿主须在回调外关闭本代次
    */
    bool valid() const;
private:
    // 仅注释main和NetWorkTest核对真实槽位与状态转换。
    friend class NetSenderServiceProxyTest;
    /** 禁止复制独占关联状态
    @param [in] other 不可复制对象
    */
    NetSenderServiceProxy(const NetSenderServiceProxy& other);

    /** 禁止赋值
    @param [in] other 不可复制对象
    @return 不可调用
    */
    NetSenderServiceProxy& operator=(const NetSenderServiceProxy& other);
    // 本模块独占全部关联和敏感正文；析构在本模块释放。
    NetSenderServiceProxyState* m_state;
};