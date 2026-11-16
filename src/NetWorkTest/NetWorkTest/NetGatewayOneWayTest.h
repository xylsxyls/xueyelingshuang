#pragma once
#include "NetInterface/NetInterfaceAPI.h"
#include <vector>

// 单向性能测试的解密后校验器；仅服务执行者访问，不发送逐条业务回包
class NetGatewayOneWayTest
{
public:
    /** 初始化未启用的校验器
    */
    NetGatewayOneWayTest();

    /** 配置一次独立批次；无效参数不改变状态
    @param [in] count 总帧数，1至10000000
    @param [in] size 正文字节数，0至65536
    @param [in] producers 生产者数，1或4
    @return 是否配置成功
    */
    bool configure(uint32_t count, uint32_t size, uint32_t producers);

    /** 校验正文及每个生产者的完整顺序；失败不推进计数
    @param [in] frame 解密后的借用帧，正文仅在本次调用有效
    @return 是否接受当前帧
    */
    bool receive(const NetInterfaceFrame& frame);

    /** 查询是否已经显式配置
    @return 已配置返回true
    */
    bool enabled() const;

    /** 查询所有预期帧是否已校验完成
    @return 每个生产者数量与顺序均匹配返回true
    */
    bool complete() const;

    /** 输出一次批次统计；调用者仅在最后一帧消费后调用
    */
    void report() const;

    /** 取得测试时钟；Windows同机进程共用QPC纪元，其他平台仅用于本进程
    @return 单调微秒数，0表示时钟不可用
    */
    static uint64_t now();

    /** 校验计数、乱序、重复、错误正文及无效配置，失败由测试runner记录
    */
    static void run();

private:
    // 本批次预期正文，配置后不可变
    std::vector<uint8_t> m_body;
    // 预期总帧数；0表示未启用
    uint32_t m_expected;
    // 生产者数，限定1或4
    uint32_t m_producers;
    // 已通过校验的总帧数
    uint32_t m_received;
    // 各生产者已收到的序号，从1递增；不同生产者允许交错
    uint32_t m_sequences[4];
    // 最后一条正文校验之后立即取得的时间，不包含统计输出与控制文件
    uint64_t m_finished;
};