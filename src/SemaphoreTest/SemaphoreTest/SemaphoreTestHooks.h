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

/** 在真实系统睡眠边界触发受控暂停
*/
void SemaphoreTestWakeHook();

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