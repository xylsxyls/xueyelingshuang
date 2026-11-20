#pragma once
#include <stdint.h>
#include "SemaphoreTestPosixName.h"

class Semaphore;
class SemaphoreEventPool;

/** 注入一次系统调用或记录申请失败，只在测试宏构建中链接
@param [in] point 故障点编号
@return 0不注入，非零为模拟原生错误码
*/
int32_t SemaphoreTestFault(int32_t point);

/** 记录x86本轮CAS退避使用的处理器提示次数
@param [in] pauses 生产循环将执行的次数
*/
void SemaphoreTestObservePauses(uint32_t pauses);

/** 记录配置用例首次交给后端的休眠时间
@param [in] milliseconds 将交给后端的一段等待预算
*/
void SemaphoreTestObserveSlice(uint32_t milliseconds);

/** 在真实系统睡眠边界触发受控暂停
*/
void SemaphoreTestWakeHook();

/** 测试虚假系统唤醒：只在受控预算用例启用，真实等待路径仍使用原有截止时间
@return 是否模拟一次未消费许可的系统返回
*/
bool SemaphoreTestSpuriousWake();

/** 观察命名发布入口的真实调用次数，用于确认容量等待已发生重试
@param [in] reset 是否在无发布者时清零
@return 清零前或当前的调用次数
*/
uint32_t SemaphoreTestPublishAttempts(bool reset = false);

/** 观察命名容量重试第一次交给系统的休眠时间
@param [in] milliseconds 已送入退避路径的休眠毫秒数
*/
void SemaphoreTestObserveCapacitySleep(uint32_t milliseconds);

/** 报告实际选用的Windows睡眠后端
@param [in] addressWait 是否使用地址等待
*/
void SemaphoreTestBackend(bool addressWait);

/** 计数和事件路径的受控暂停入口
@param [in] point 钩子编号
@param [in] object 被测实例，仅在调用内借用
*/
void SemaphoreTestHook(int32_t point, Semaphore* object);

/** 外部静止期注入计数边界值
@param [in] object 被测实例
@param [out] value 指定计数值
@return 本实例是否应使用该值
*/
bool SemaphoreTestCountSeed(Semaphore* object, uint64_t& value);

/** 事件池的登记和安全回收暂停入口
@param [in] point 池钩子编号
@param [in] pool 被测事件池，仅在调用内借用
*/
void SemaphoreTestPoolHook(int32_t point, SemaphoreEventPool* pool);