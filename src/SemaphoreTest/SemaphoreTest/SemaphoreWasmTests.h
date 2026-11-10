#pragma once
#include <stdint.h>

/** Wasm主线程能力及工作线程计数/组合等待验证，不提供库的C接口
*/
class SemaphoreWasmTests
{
public:
    /** 检查Wasm32/64内部登记顺序和64位计数表示，不在主线程执行系统等待
    @return 通过返回0，失败返回非零；并发等待仍由独立工作线程用例覆盖
    */
    static int32_t admission();

    /** 运行能力测试并由事件循环收集工作线程结果
    @return 普通模块返回退出码，线程模块保持事件循环直到结果可见
    */
    static int32_t run();

    /** 非阻塞检查工作线程最后一次发布的结果
    @param [in] context 未使用，必须为空
    */
    static void poll(void* context);

    /** 捕获全部异常的工作线程入口
    @param [in] context 未使用，必须为空
    @return 空指针
    */
    static void* worker(void* context);
};