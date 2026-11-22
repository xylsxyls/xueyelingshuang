#pragma once

// TLS帧连接、恢复与生命周期测试集合，逐用例独立建立预期并检查实际结果
class NetInterfaceConnectionTestCases
{
public:
    /** 验证统一TLS/帧驱动的大帧、粘包和消费背压
    */
    static void framedTransfer();

    /** 普通请求队列满时仍保留控制消息容量，且按帧边界调度
    */
    static void controlPriority();

    /** 已开始的一兆帧不能被后到控制帧插断，完成后按控制优先及普通FIFO排出
    */
    static void partialFrameControl();

    /** 经真实TLS传来的非法明文帧必须使整个连接持续失败
    */
    static void invalidPeerFrame();

    /** 反复填满两种队列，验证槽回收、复制所有权、帧优先级和压缩后的预算归还
    */
    static void recycledQueues();

    /** 发现表原子解析、等价IP去重、别名输入和缓存完整性/期限
    */
    static void directoryBoundaries();

    /** 验证发现器固定候选轮转、数值去重、熵失败隔离和重新启动
    */
    static void discoverySelection();

    /** 验证管理运输推进与停止同时异常、唯一终态、回调重入和OPEN背压
    */
    static void gatewayChannelLifecycle();

    /** 发现器异常/异步停止、关闭回调重入、未接纳尝试和固定产品隔离
    */
    static void recoveryLifecycle();

    /** 使用真实单调时间等待30秒预算耗尽，迟到poll不得创建运输或重复通知
    */
    static void recoveryDeadline();

    /** 请求/取消/数据/查询异常、终态顺序、128额度、错误响应及不可续期截止
    */
    static void protocolSessionLifecycle();
};