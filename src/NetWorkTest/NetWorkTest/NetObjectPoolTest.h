#pragma once
#include <atomic>
#include <cstdint>

// 测试用生命周期探针；仅测试工程持有，生产库不包含测试分支
struct NetObjectPoolTestValue
{
public:
    // 全局存活计数的借用，覆盖全部池对象
    std::atomic<uint32_t>* m_live;
    // 构造时保存的校验值
    uint64_t m_value;

public:
    /** 建立一份生命周期探针
    @param [in,out] live 存活统计，生命周期覆盖本对象
    @param [in] value 校验值
    */
    NetObjectPoolTestValue(std::atomic<uint32_t>& live, uint64_t value);

    /** 精确归还一次存活计数
    */
    ~NetObjectPoolTestValue();
};

// 对象池容量、跨线程引用与所有权转移测试
class NetObjectPoolTest
{
public:
    /** 验证增长、耗尽、复用、复制移动及并发最后释放；失败交给测试runner
    */
    static void run();

    /** 冷池并发扩容时不把其它增长者的预算预留误判为消息容量耗尽
    */
    static void concurrentGrowth();
};