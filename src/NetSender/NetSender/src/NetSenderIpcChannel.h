#pragma once
#include "NetSenderMacro.h"
#include <stdint.h>

class NetSenderIpcState;

// 新网关使用的ProcessWork发送结果；桌面链路与ProcessWork v2配套重编。
enum NetSenderIpcResult
{
    // 正文完整提交到共享区，不代表业务完成；v2不在本层保留待通知队列
    NET_SENDER_IPC_ACCEPTED = 0,
    // 未发布，登记待批准、共享块满或竞争，调用者可在截止时间内保序重试
    NET_SENDER_IPC_BUSY = 1,
    // 参数或共享布局不合法
    NET_SENDER_IPC_INVALID = -1,
    // 接收端未就绪或资源失效，未发布
    NET_SENDER_IPC_UNAVAILABLE = -2,
    // 超过声明或实际共享区容量，未发布
    NET_SENDER_IPC_TOO_LARGE = -3,
    // 旧版兼容状态；v2不返回此值，提交后不因通知失败重发正文
    NET_SENDER_IPC_UNCERTAIN = -4
};

// 桌面单执行者有界IPC发送器，与ProcessWork的占位、发布和故障恢复顺序配对。
// 身份、实例代次、请求截止时间及重试幂等由上层网关信封验证。
class NetSenderAPI NetSenderIpcChannel
{
public:
    /** 创建空发送器，不启动线程
    */
    NetSenderIpcChannel();

    /** 释放本实例映射；已发布消息不能撤销，不擅自修改接收端数据
    */
    ~NetSenderIpcChannel();

    /** 打开接收端的既有ProcessWork共享对象，不创建接收端
    @param [in] destinationPid 注册并验证过的真实接收进程号
    @param [in] senderPid 当前发送进程号，上层不得从网络请求采用此值
    @param [in] payloadLimit 注册时声明的最大消息体，1至1048608，不含12字节IPC头
    @return ACCEPTED已打开；TOO_LARGE表示目标声明正文容量小于payloadLimit；失败不保留半通道，尚未发送正文
    */
    int32_t open(int32_t destinationPid, int32_t senderPid, uint32_t payloadLimit);

    /** 共用v2通道做有界申请与提交，登记未完成或满时返回BUSY，不调用阻塞send
    @param [in] data size非0时不可空，返回后不借用
    @param [in] size 消息体字节数
    @param [in] type 非负MessageType编号
    @return ACCEPTED/BUSY或负数错误；v2不返回UNCERTAIN，提交成功不因通知丢失重发
    */
    int32_t trySend(const uint8_t* data, uint32_t size, int32_t type);

    /** 保留兼容入口并检查当前接收器代次；v2接收器主动补查已提交正文
    @return ACCEPTED接收器仍就绪，UNAVAILABLE已关闭；不重放正文
    */
    int32_t flushNotifications();

    /** 兼容查询；v2不在NetSender层缓存待通知消息
    @return v2固定返回0
    */
    uint32_t pendingNotifications() const;

    /** 释放映射和句柄，不清理对端内存；生命周期方先停止新投递
    */
    void close();

private:
    /** 禁止复制资源所有权
    @param [in] other 不可复制实例
    */
    NetSenderIpcChannel(const NetSenderIpcChannel& other);

    /** 禁止复制资源所有权
    @param [in] other 不可复制实例
    @return 不可调用
    */
    NetSenderIpcChannel& operator=(const NetSenderIpcChannel& other);

private:
    // 独占映射缓存，调用方只在同一执行者访问
    NetSenderIpcState* m_state;
};