#pragma once
#include <atomic>
#include <thread>
#include <utility>

// 测试自身的计数也必须满足 v120 Win32 的六十四位原子地址对齐要求
#if defined(_MSC_VER)
#define LOCKFREEHASHMAP_TEST_ALIGN_64 __declspec(align(8))
#else
#define LOCKFREEHASHMAP_TEST_ALIGN_64 alignas(8)
#endif

/** 用例线程的作用域所有者，创建其他线程失败时仍等待已启动线程退出
线程函数的异常单独计入设施失败，不能从自身工作函数销毁本对象
*/
class LockFreeHashMapTestThread
{
public:
    /** 构造未启动对象，用于测试中先声明后接管线程
    */
    LockFreeHashMapTestThread();

    /** 复制工作函数并启动线程，创建异常交给用例入口捕获
    @param [in] function 工作函数副本，引用捕获对象须存活到 join 返回
    */
    template <typename FunctionType>
    explicit LockFreeHashMapTestThread(const FunctionType& function);

    /** 转移线程所有权，不创建线程
    @param [in,out] other 转移后不再拥有线程，可正常析构
    */
    LockFreeHashMapTestThread(LockFreeHashMapTestThread&& other) throw();

    /** 先收拢原线程再接管新线程
    @param [in,out] other 转移后不再拥有线程
    @return 当前所有者
    */
    LockFreeHashMapTestThread& operator=(LockFreeHashMapTestThread&& other) throw();

    /** 等待已启动线程完成，异常路径也不会直接析构可连接的 std::thread
    */
    ~LockFreeHashMapTestThread();

    /** 由外部所有者等待线程结束，重复调用允许
    系统接口异常计入设施失败，由外层独立进程截止时间约束无法收拢的异常环境
    */
    void join() throw();

    /** 查询外部所有者是否仍有待收拢线程
    @return 持有可连接线程为 true，不与转移或 join 并发调用
    */
    bool joinable() const;

    /** 读取工作函数或线程设施异常累计数
    @return 单调异常数量，不由用例重置
    */
    static int failures();
private:
    /** 禁止复制线程所有权
    @param [in] other 不可复制的源对象
    */
    LockFreeHashMapTestThread(const LockFreeHashMapTestThread& other);

    /** 禁止复制赋值线程所有权
    @param [in] other 不可复制的源对象
    @return 无实现，不可调用
    */
    LockFreeHashMapTestThread& operator=(const LockFreeHashMapTestThread& other);
private:
    // 独占线程句柄，构造完成后必须在夹具销毁前收拢
    std::thread m_thread;
    // 工作函数和设施异常计数，不影响被测库状态
    static std::atomic<int> s_failures;
};

/** 暂停夹具的作用域清理，检查抛异常时也先放行并收拢工作线程
*/
class LockFreeHashMapTestResume
{
public:
    /** 借用仍存活的放行标记和线程；本对象必须先于二者析构
    @param [in,out] resume 放行标记
    @param [in,out] thread 外部所有者线程，不可为当前线程
    */
    LockFreeHashMapTestResume(std::atomic<bool>& resume, LockFreeHashMapTestThread& thread);

    /** 放行并收拢，正常路径已经 join 时也允许重复清理
    */
    ~LockFreeHashMapTestResume();
private:
    /** 禁止复制清理责任
    @param [in] other 不可复制对象
    */
    LockFreeHashMapTestResume(const LockFreeHashMapTestResume& other);

    /** 禁止赋值清理责任
    @param [in] other 不可赋值对象
    @return 无实现
    */
    LockFreeHashMapTestResume& operator=(const LockFreeHashMapTestResume& other);
private:
    // 借用当前夹具的原子放行标记
    std::atomic<bool>& m_resume;
    // 借用当前夹具的线程所有者
    LockFreeHashMapTestThread& m_thread;
};

#include "LockFreeHashMapTestThread.inl"