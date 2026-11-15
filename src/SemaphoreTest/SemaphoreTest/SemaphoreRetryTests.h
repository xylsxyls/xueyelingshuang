#pragma once
#include <stdint.h>
#include <thread>

/** 测试线程的作用域回收；引用对象必须在本所有者之前构造
*/
class SemaphoreRetryJoin
{
public:
    /** 借用已创建线程
    @param [in,out] thread 作用域结束时回收的线程
    */
    explicit SemaphoreRetryJoin(std::thread& thread);

    /** 回收线程，不允许异常逃出析构
    */
    ~SemaphoreRetryJoin();

private:
    // 生命周期覆盖本所有者的线程
    std::thread* m_thread;
};

/** 用例88：重试策略的四种组合及普通发布等价性
    初始计数已满时，不等待容量的调用返回满载且不提交；等待容量的调用须等消费释放后仅提交一次。
    4096次快速容量重试之后，每次至少经过1毫秒休眠，计数不回绕。
    并发计数发布与消费同时执行，所有成功发布恰好消费一次；未打开对象不进入无限重试。
    命名满载检查使用Windows可配置计数上限；其他平台运行本地并发与退避检查。
*/
class SemaphoreRetryTests
{
public:
    /** 执行真实接口的容量释放、并发计数和计时边界检查
    @return 全部独立预期满足时返回true
    */
    static bool run();

private:
    /** 验证指定重试组合的命名满载行为
    @param [in] mode 0至3表示两个bool的组合，4表示普通发布
    @return 是否未重复提交且满载分类正确
    */
    static bool capacity(int32_t mode);
};