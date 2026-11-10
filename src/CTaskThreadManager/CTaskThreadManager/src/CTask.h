#pragma once
#include "CTaskThreadManagerMacro.h"
#include <stdint.h>
#include <functional>
#include <memory>

/** 任务基类；结果由任务自行交付，停止为合作通知，回调需支持并发
*/
class CTaskThreadManagerAPI CTask
{
public:
    /** 创建默认属性 ID 为 1 的任务
    */
    CTask();

    /** 创建指定属性 ID 的任务
    @param [in] propertyId 属性 ID，用于批量停止同属性任务，不要求唯一
    */
    explicit CTask(int32_t propertyId);

    /** 释放资源；析构不得抛异常或等待工作线程
    */
    virtual ~CTask();

    /** 工作线程执行业务；异常被记录，不影响后续任务
    */
    virtual void doTask() = 0;

    /** 合作中断，可能与 doTask 并发；不得等待本线程退出
    */
    virtual void stopTask();

    /** 判断被高优先级任务打断后是否重做
    @return true 请求由已配置工厂生成任务重新执行；未配置工厂时不重做
    */
    virtual bool reExecute();

    /** 静止期配置延迟重做工厂；只保存函数，不立即生成任务
    @param [in] factory 无参数且返回独立、尚未投递 CTask 指针的 lambda 或函数；空函数清除重做工厂
    @return 保存是否成功；内部复制或分配失败返回 false 并保留旧配置；入参构造异常由调用方处理
    */
    bool clone(const std::function<CTask*()>& factory);

    /** 借用冻结的工厂配置；所有投递和回调结束前不得修改此任务的配置
    @return 未配置时为 nullptr；非空指针借用至下次静止配置或析构，捕获对象须满足并发回调契约
    */
    const std::function<CTask*()>* cloneFactory() const;

    /** 查询任务的属性 ID
    @return 构造时的属性 ID，不要求唯一，运行期间不得修改
    */
    int32_t getPropertyId();

protected:
    // 构造后不再修改的属性 ID，不要求唯一
    int32_t m_propertyId;

private:
    // 非空时只在确认重做后调用；返回任务未配置工厂时继承本工厂，支持再次重做
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::shared_ptr<const std::function<CTask*()>> m_cloneFactory;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

#include "CTask.inl"