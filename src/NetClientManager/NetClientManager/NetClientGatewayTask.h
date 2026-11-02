#pragma once
#include "NetClientManager.h"
#include "NetClientGateway.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>

// 拥有客户端网关运行期的长任务，外部仅通过原子状态和停止请求交互
class NetClientGatewayTask : public CTask
{
  public:
    /** 复制启动配置
    @param [in] configuration 启动前有效配置
    */
    explicit NetClientGatewayTask(const NetClientGatewayConfiguration& configuration);

    /** 擦除私钥副本
    */
    ~NetClientGatewayTask();

    /** 唯一执行者创建、驱动、停止所有网络/IPC
    */
    virtual void DoTask();

    /** 仅请求协作停止
    */
    virtual void StopTask();

    /** 查询任务运行状态，不能据此判定业务服务器已接受请求
    @return 3模式为本机入口运行，2模式为旧远端会话就绪
    */
    bool ready() const;

    /** 所有资源已经收敛
    @return 是否结束
    */
    bool done() const;

    /** 启动失败状态
    @return NI_OK或负数
    */
    int32_t error() const;

  private:
    // 启动配置的独占副本，任务开始后只读
    NetClientGatewayConfiguration m_configuration;
    // 跨线程协作停止请求，不从调用线程销毁网络对象
    std::atomic<bool> m_stop;
    // 唯一执行者已退出且所属网关资源完成回收
    std::atomic<bool> m_done;
    // 3模式表示本机入口已运行；2模式表示旧远端会话就绪，均不表示业务成功
    std::atomic<bool> m_ready;
    // 启动或执行失败码，供进程监督者读取
    std::atomic<int32_t> m_error;
};