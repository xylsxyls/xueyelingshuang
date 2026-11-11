#pragma once
#include "ProcessWorkMacro.h"
#include <stdint.h>

/** 实际存储扩容的资源类别；不包含积压额度调整
*/
enum ProcessWorkMemoryKind
{
    // Post小包实际存储，单位槽
    ProcessWorkPostBuffers,
    // 接收批次实际存储，单位字节
    ProcessWorkReceiveBuffers,
    // 共享正文实际存储，单位块
    ProcessWorkSharedBuffers
};

/** 交给外部算法的增长观察；并发调用的快照可能过期，返回值只表示建议增量
*/
struct ProcessWorkAPI ProcessWorkMemoryGrowContext
{
public:
    // 资源类别及其计量单位
    ProcessWorkMemoryKind m_kind;
    // 配置的初始容量
    uint64_t m_initialCapacity;
    // 当前已发布容量，不含其他申请者尚未成功的请求
    uint64_t m_currentCapacity;
    // 已成功发布的增长次数，不是策略被调用次数
    uint64_t m_successfulGrowCount;
    // 本次最低需要的新增量
    uint64_t m_requiredGrowth;

public:
    /** 建立确定的空观察
    */
    ProcessWorkMemoryGrowContext();
};

/** 已发布的增长事实；通知异常不撤销成功增长，也不重复通知
*/
struct ProcessWorkAPI ProcessWorkMemoryGrowth
{
public:
    // 实际可用存储的类别及其计量单位
    ProcessWorkMemoryKind m_kind;
    // 该次成功提交之前的容量
    uint64_t m_previousCapacity;
    // 该次提交新增的容量
    uint64_t m_addedCapacity;
    // 该次提交之后的容量；并发时通知的调用先后不保证有序
    uint64_t m_currentCapacity;
    // 本次成功增长编号
    uint64_t m_successfulGrowCount;

public:
    /** 建立确定的空事件
    */
    ProcessWorkMemoryGrowth();
};

/** 可继承的ProcessWork内存增长策略；保存强引用至资源关闭
    同一个实例可能被多个生产者并发调用。算法应短小，不依赖调用次数产生业务副作用。
    通知不可同步重入相同队列或关闭拥有者；异常在库边界处理，不向业务发送接口传播。
*/
class ProcessWorkAPI ProcessWorkMemoryPolicy
{
public:
    /** 允许经基类释放外部策略
    */
    virtual ~ProcessWorkMemoryPolicy();

    /** 按当前容量增加，起步使用初始容量和本次需求的较大值
    @param [in] context 弱一致容量观察，不包含上限；内部负责裁剪
    @return 建议新增量，0拒绝本次增长；抛异常按策略失败处理
    */
    virtual uint64_t grow(const ProcessWorkMemoryGrowContext& context);

    /** 成功发布后通知，默认不处理；初始化预留不计为运行期增长
    @param [in] growth 本次成功的实际增量，借用至回调返回
    */
    virtual void grown(const ProcessWorkMemoryGrowth& growth);
};