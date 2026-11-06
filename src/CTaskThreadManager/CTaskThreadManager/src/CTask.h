#pragma once
#include "CTaskThreadManagerMacro.h"
#include <stdint.h>

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
    virtual void DoTask() = 0;

    /** 合作中断，可能与 DoTask 并发；不得等待本线程退出
    */
    virtual void StopTask();

    /** 判断被高优先级任务打断后是否重做
    @return true 请求使用预备克隆重新执行
    */
    virtual bool ReExecute();

    /** 在每次执行前准备独立副本
    @return 新对象所有权交给库；nullptr 不重做，禁止返回 this
    */
    virtual CTask* Clone();

    /** 查询任务的属性 ID
    @return 构造时的属性 ID，不要求唯一，运行期间不得修改
    */
    int32_t GetPropertyId();
protected:
    // 构造后不再修改的属性 ID，不要求唯一
    int32_t m_propertyId;
};