#pragma once
#include "BlackBoxTestSupport.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CTaskThreadManagerTest.h"

/** 用屏障强制多个生产者同时进入同一原任务的重做回调
*/
class TestConcurrentRedoTask : public RedoTask
{
public:
    /** 保存本轮独占的测试屏障
    @param [in] state 原任务与克隆共用的顺序账本
    @param [in,out] entered 已进入回调的生产者数
    @param [in,out] release 是否允许全部重做回调返回
    */
    TestConcurrentRedoTask(const std::shared_ptr<RedoState>& state, std::atomic<int32_t>& entered,
        std::atomic<bool>& release);

    /** 增加进入数并等测试线程统一释放
    @return true 请求重做
    */
    virtual bool reExecute();

private:
    // 本轮创建的生产者全部退出前有效
    std::atomic<int32_t>& m_entered;
    // 本轮创建的生产者全部退出前有效
    std::atomic<bool>& m_release;
};

/** 验证默认属性 ID、显式属性 ID 和克隆属性的任务
*/
class TestPropertyTask : public CTask
{
public:
    /** 使用基类默认属性 ID
    */
    TestPropertyTask();

    /** 保存显式属性 ID，不要求唯一
    @param [in] propertyId 属性 ID，允许完整 int32_t 范围
    */
    explicit TestPropertyTask(int32_t propertyId);

    /** 空任务体，只验证属性接口
    */
    virtual void doTask();

};

/** 返回复查发现边界的回归用例
@param [in] firstId 起始编号
@return 用例清单
*/
std::vector<BlackBoxCase> taskThreadRegressionCases(int32_t firstId);

/** 验证有界预读仍逐项检查优先级，保留重做队头、属性停止及等待计数
*/
void taskPrefetchedPriorityTest();

/** 验证高优先级反复置位、清空和重新发布不依赖 60 秒空闲补查
*/
void taskPriorityWakeTest();

/** 工厂回归任务；业务体与重做意图相互独立
*/
class TestFactoryTask : public TestTask
{
public:
    /** 保存业务体与重做意图
    @param [in] propertyId 属性 ID
    @param [in] body 独立业务断言及行为
    @param [in] redo 是否请求重做
    */
    TestFactoryTask(int32_t propertyId, const std::function<void(TestTask&)>& body,
        bool redo = true);

    /** 返回本任务配置的重做意图
    @return 构造时指定的意图
    */
    virtual bool reExecute();

private:
    // 构造后只读
    const bool m_redo;
};

/** 验证无抢占不创建、原任务返回后创建、捕获参数及原级队头顺序
*/
void taskRedoFactoryOrderTest();

/** 验证并发抢占只创建一次，以及工厂继承后再次重做
*/
void taskRedoFactoryConcurrentTest();

/** 验证工厂抛异常、空返回、自身返回及关闭竞争都能收敛
*/
void taskRedoFactoryFailureTest();

/** 可注入复制失败的工厂；仅在单线程配置或工作线程调用期间修改开关
*/
class TestFactoryCopyFailure
{
public:
    /** 保存跨本用例有效的故障开关与计数
    @param [in,out] failCopy 为 true 时复制构造抛异常
    @param [in,out] calls 工厂调用数
    @param [in] failAfterCreate 创建任务后是否使继承复制失败
    */
    TestFactoryCopyFailure(bool& failCopy, std::atomic<int32_t>& calls, bool failAfterCreate);

    /** 按外部开关注入复制失败
    @param [in] source 原工厂
    */
    TestFactoryCopyFailure(const TestFactoryCopyFailure& source);

    /** 创建无业务任务，按模式打开复制故障
    @return 新任务所有权
    */
    CTask* operator()() const;

private:
    // 借用复制故障开关
    bool* m_failCopy;
    // 借用调用计数
    std::atomic<int32_t>* m_calls;
    // 是否在创建之后使后续继承失败
    bool m_failAfterCreate;
};

/** 验证配置复制失败保留旧值、继承失败释放新对象和有限池耗尽时完整收尾
*/
void taskRedoFactoryResourceTest();

/** 验证工厂返回原始指针后，控制块分配失败仍释放新对象并继续处理高优先级任务
*/
void taskRedoFactoryAllocationTest();

/** 验证子任务自带工厂优先，以及工厂在途时关闭不能再提交新重做
*/
void taskRedoFactoryLifecycleTest();