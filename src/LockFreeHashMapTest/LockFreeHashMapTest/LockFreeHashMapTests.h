#pragma once
#include "LockFreeHashMapTestThread.h"
#include "LockFreeHashMapAPI.h"
#include <atomic>
#include <map>
#include <string>
#include <thread>

/** 无需分配内存的用例失败信号，由最外层入口捕获并正常返回
*/
enum LockFreeHashMapTestFailure
{
    // 独立预期未满足，停止当前用例，防止继续解引用失败结果
    LockFreeHashMapTestCheckFailed = 1
};

/** 确定性的全键碰撞哈希，用于独立模型和并发压力测试，无共享状态
*/
struct LockFreeHashMapCollisionHash
{
public:
    /** 将所有整数放入同一哈希值
    @param [in] value 未使用的整数键
    @return 始终为 0
    */
    size_t operator()(int value) const;
};

/** 通过合法用户值操作注入复制异常和暂停，计数载荷存活数量
*/
struct LockFreeHashMapTestValue
{
public:
    // 独立校验用的整数载荷
    int m_value;
    // 当前完整构造且未析构的载荷数量
    static std::atomic<int> s_live;
    // 累计复制尝试次数，失败复制也计入
    static std::atomic<int> s_copies;
    // 需要失败的复制序号，-1 关闭注入
    static std::atomic<int> s_failCopy;
    // 需要暂停复制的载荷内容，-1 关闭注入
    static std::atomic<int> s_pauseValue;
    // 被暂停操作已经进入的原子通知
    static std::atomic<bool> s_entered;
    // 由测试所有者发布的继续通知
    static std::atomic<bool> s_resume;
    // 需要暂停析构的载荷内容，-1 关闭注入
    static std::atomic<int> s_pauseDestructor;
    // 析构等待超过测试期限的累计失败标志
    static std::atomic<bool> s_destructorTimedOut;
public:
    /** 显式构造值，故意不提供默认构造
    @param [in] value 用于独立校验的整数载荷
    */
    explicit LockFreeHashMapTestValue(int value);

    /** 复制时按开关注入异常或暂停，失败构造不增加存活数量
    @param [in] other 调用期间借用的源值
    */
    LockFreeHashMapTestValue(const LockFreeHashMapTestValue& other);

    /** 结束一个载荷生命周期，必要时按有限截止时间暂停析构
    */
    ~LockFreeHashMapTestValue();
};

/** 独立注入哈希异常，正常分支强制所有键碰撞
*/
struct LockFreeHashMapTestHash
{
public:
    // 仅由夹具按约定设置的异常注入开关
    static std::atomic<bool> s_fail;
public:
    /** 按开关注入哈希异常
    @param [in] key 待查询整数键，不影响碰撞结果
    @return 正常为 0，故障时抛运行时异常供被测库捕获
    */
    size_t operator()(int key) const;
};

/** 比较异常注入器，无资源所有权
*/
struct LockFreeHashMapTestEqual
{
public:
    // 仅由夹具按约定设置的异常注入开关
    static std::atomic<bool> s_fail;
public:
    /** 按开关注入比较异常
    @param [in] left 左键
    @param [in] right 右键
    @return 正常为整数相等结果，故障时抛运行时异常
    */
    bool operator()(int left, int right) const;
};

/** 不可默认构造的键，通过串行开关注入复制失败，不拥有外部资源
*/
struct LockFreeHashMapTestKey
{
public:
    // 独立校验用的整数载荷
    int m_value;
    // 仅由夹具按约定设置的异常注入开关
    static bool s_fail;
public:
    /** 构造测试键
    @param [in] value 键的整数内容
    */
    explicit LockFreeHashMapTestKey(int value);

    /** 复制键或在发布前注入异常
    @param [in] other 调用期间借用的源键
    */
    LockFreeHashMapTestKey(const LockFreeHashMapTestKey& other);
};

/** 测试键的无状态哈希器
*/
struct LockFreeHashMapKeyHash
{
public:
    /** 确定性地计算测试键哈希
    @param [in] key 调用期间借用的测试键
    @return 整数内容转为 size_t
    */
    size_t operator()(const LockFreeHashMapTestKey& key) const;
};

/** 测试键的无状态等价比较器
*/
struct LockFreeHashMapKeyEqual
{
public:
    /** 不复制键地比较内容
    @param [in] left 左键，调用期间借用
    @param [in] right 右键，调用期间借用
    @return 整数内容相等为 true
    */
    bool operator()(const LockFreeHashMapTestKey& left, const LockFreeHashMapTestKey& right) const;
};

/** 单次比较故障检测器，检查用户异常是否被错误重试后提交
*/
struct LockFreeHashMapOnceEqual
{
public:
    // 下一次操作抛异常后自动关闭的原子开关
    static std::atomic<bool> s_throw;
    // 累计比较执行次数，用于发现隐藏重试
    static std::atomic<int> s_calls;
public:
    /** 首次请求故障时抛异常，后续调用恢复正常比较
    @param [in] left 左键
    @param [in] right 右键
    @return 整数相等结果，故障时不返回
    */
    bool operator()(int left, int right) const;
};

/** 非平凡读取载荷，分别控制复制、赋值异常及暂停，不持有外部资源
*/
struct LockFreeHashMapReadPayload
{
public:
    // 独立校验用的整数载荷
    int m_value;
    // 当前完整构造且未析构的载荷数量
    static std::atomic<int> s_live;
    // 累计复制尝试次数，失败复制也计入
    static std::atomic<int> s_copies;
    // 复制故障开关，与赋值故障独立
    static std::atomic<bool> s_throwCopy;
    // 累计输出赋值次数，失败赋值也计入
    static std::atomic<int> s_assignments;
    // 下一次赋值暂停后自动关闭的开关
    static std::atomic<bool> s_pause;
    // 下一次操作抛异常后自动关闭的原子开关
    static std::atomic<bool> s_throw;
    // 被暂停操作已经进入的原子通知
    static std::atomic<bool> s_entered;
    // 由测试所有者发布的继续通知
    static std::atomic<bool> s_resume;
public:
    /** 记录非平凡值的生命周期
    @param [in] value 独立校验用的整数载荷
    */
    explicit LockFreeHashMapReadPayload(int value = 0);

    /** 构造不可变值副本，复制异常发生在存活计数增加前
    @param [in] other 调用期间借用的源载荷
    */
    LockFreeHashMapReadPayload(const LockFreeHashMapReadPayload& other);

    /** 在真实 get 输出赋值处暂停或失败，此时源值仍须被保护
    @param [in] other 调用期间借用的源载荷
    @return 当前对象，故障时不重复赋值
    */
    LockFreeHashMapReadPayload& operator=(const LockFreeHashMapReadPayload& other);

    /** 精确记录一次载荷析构，不抛异常
    */
    ~LockFreeHashMapReadPayload();
};

/** 只可移动的遍历回调，验证内部移动异常边界及旧接口兼容性
*/
struct LockFreeHashMapMoveCallback
{
public:
    // 借用调用方计数，调用方必须存活到遍历结束
    int* m_calls;
    // 移动时是否注入异常，不修改原对象后再抛出
    bool m_throwOnMove;
public:
    /** 准备独立的回调状态
    @param [in,out] calls 调用次数计数
    @param [in] throwOnMove 是否让后续移动构造失败
    */
    LockFreeHashMapMoveCallback(int& calls, bool throwOnMove);

    /** 转移借用或在转移前注入失败
    @param [in,out] other 成功后不再使用，失败时保持原借用
    */
    LockFreeHashMapMoveCallback(LockFreeHashMapMoveCallback&& other);

    /** 累计一次完整回调，键值只在调用期间借用
    @param [in] key 当前键
    @param [in] value 当前值
    */
    void operator()(const int& key, const LockFreeHashMapReadPayload& value) const;
private:
    /** 禁止复制，以编译期验证移动接口仍然可用
    @param [in] other 不可复制的源对象
    */
    LockFreeHashMapMoveCallback(const LockFreeHashMapMoveCallback& other);

    /** 禁止复制赋值
    @param [in] other 不可复制的源对象
    @return 无实现，不可调用
    */
    LockFreeHashMapMoveCallback& operator=(const LockFreeHashMapMoveCallback& other);
};

/** 将不同整数位模式归入同一模十等价类，检查独立键缓存被正确关闭
*/
struct LockFreeHashMapModuloEqual
{
public:
    /** 比较模十等价类
    @param [in] left 左整数键
    @param [in] right 右整数键
    @return 取余结果相同为 true
    */
    bool operator()(int left, int right) const;
};

/** 测试工程内的静止期白盒夹具，注入实际难以达到的计数边界，不要求产品声明测试入口
*/
struct LockFreeHashMapTestAccess
{
public:
    /** 普通函数回调，验证函数引用能在内部正确衰减为指针
    @param [in] key 当前键，仅调用期借用
    @param [in] value 当前值，仅调用期借用
    */
    static void ignoreReadPayload(const int& key, const LockFreeHashMapReadPayload& value);

    /** 检查真实表代次、值版本和空桶身份耗尽
    @return 耗尽不回绕且边界操作符合合同为 true
    */
    static bool exhaustion();

    /** 检查后端转移、固定缓存和可独立领取的回收格对抗时序
    */
    static void optimizations();

    /** 检查直接读取保护、迟到值提示及复制和赋值异常
    */
    static void readOptimizations();

/** 校验弱计数的唯一记账、跨代迟到及紧凑表示耗尽回退 */
    static void countOptimizations();

/** 校验分配批次提示的更新、越界回退和静止释放后复用 */
    static void allocationOptimizations();

/** 检查同键全碰撞、clear、条件修改及维护并发时的共享所有权
    */
    static void sharedContention();

    /** 仅在静止期跳转值版本并丢弃未使用的本地版本范围
    @param [in,out] map 独占且外部静止的被测表
    @param [in] value 新的版本范围高水位，不回收或复用旧令牌
    */
    static void sequence(LockFreeHashMap<int, int>& map, uint64_t value);

    /** 将空历史夹具初始化到 31 或 32 位前缀边界附近
    @param [in,out] map 尚无操作记录使用的空表
    @param [in] value 初始版本高水位
    */
    static void historySequence(LockFreeHashMap<int, int, LockFreeHashMapCollisionHash>& map, uint64_t value);

};

/** 原生进程资源快照，与逻辑池计账分开报告，不拥有系统句柄
*/
struct LockFreeHashMapTestResources
{
public:
    // Windows 本进程句柄数量，查询不可用时为 0
    unsigned long m_handles;
    // Windows 本进程线程数量，查询不可用时为 0
    unsigned long m_threads;
    // Windows 峰值工作集字节数，不等于 map 所占堆内存
    size_t m_peakWorkingSet;
    // 本进程内核及用户 CPU 时间总秒数
    double m_cpuSeconds;
public:
    /** 查询本进程并立即释放临时观察句柄，非 Windows 未提供字段保持 0
    */
    LockFreeHashMapTestResources();
};

/** 一次外部观察的操作，用于穷举满足真实时间先后关系的顺序历史
*/
struct LockFreeHashMapHistory
{
    // 0 写入、1 仅插入、2 删除、3 查找、4 清空
    int m_operation;
    // 操作键，历史模型使用两个键
    int m_key;
    // 本次写入的独立候选整数
    int m_input;
    // 观察到的整数值，不存在时为 -1
    int m_output;
    // 调用开始时取得的全局顺序号
    int m_begin;
    // 调用完成时取得的全局顺序号
    int m_end;
    // 本次实际状态码
    LockFreeHashMapStatus m_status;
    // 本次实际提交标志
    bool m_committed;
};

/** 稳定编号的测试集合，各项创建和清理独立夹具，不以主动崩溃判定失败
*/
class LockFreeHashMapTests
{
public:
    // 累计失败检查次数，支持工作线程报告
    static std::atomic<int> s_failures;
    // 累计独立检查次数
    static std::atomic<int> s_checks;
    // 入口启动后保持不变的申请跟踪开关
    static bool s_tracking;
    // 申请倒计数，0 使下一次失败，-1 关闭注入
    LOCKFREEHASHMAP_TEST_ALIGN_64 static std::atomic<long long> s_failAllocation;
    // 累计被跟踪的成功申请次数
    LOCKFREEHASHMAP_TEST_ALIGN_64 static std::atomic<long long> s_allocations;
    // 尚未释放的被跟踪申请数量
    LOCKFREEHASHMAP_TEST_ALIGN_64 static std::atomic<long long> s_liveAllocations;
    // 申请暂停倒计数，-1 关闭注入
    LOCKFREEHASHMAP_TEST_ALIGN_64 static std::atomic<long long> s_pauseAllocation;
    // 内存申请已到达暂停点的通知
    static std::atomic<bool> s_allocationEntered;
    // 允许暂停申请继续的通知
    static std::atomic<bool> s_allocationResume;
public:
    /** 记录独立预期，失败抛无需分配的用例信号以停止依赖失败前提的操作
    @param [in] condition 独立预期是否满足
    @param [in] message 同步借用的非空诊断文本
    */
    static void check(bool condition, const char* message);

    /** 按真实时间截止等待测试同步点
    @param [in] flag 原子条件，调用期间必须存活
    @param [in] milliseconds 最长等待毫秒数，默认 5000
    @return 返回时观察为 true 表示已到达同步点
    */
    static bool wait(const std::atomic<bool>& flag, int milliseconds = 5000);

    /** 仅测试使用的系统延迟，Windows 避免引入 v120 ConcRT 延迟缓存
    @param [in] milliseconds 延迟毫秒数，不属于被测容器算法
    */
    static void pause(unsigned milliseconds);

    /** 仅测试使用的调度让出，不属于被测容器算法
    */
    static void yieldThread();

    /** 读取高精度单调秒数，避免 VS2013 steady_clock 的毫秒分辨率
    @return 单调秒数，用于本进程内时间差
    */
    static double timestamp();

    /** 先稳定 CRT 线程设施，再比较分配基线，不创建被测 map
    */
    static void warmRuntime();

    /** 编号 1，检查槽位预算、增长阈值、策略、保护及多批次缩容
    */
    static void pool();

    /** 编号 2，固定种子下将新旧接口与独立 std::map 模型比较，包含全碰撞
    */
    static void model();

    /** 编号 3，检查实例和版本身份、共享所有权及缓存和注册表示例
    */
    static void versions();

    /** 编号 4，注入用户异常和每个已观察的系统申请失败点
    */
    static void exceptions();

    /** 编号 5，检查同键唯一发布、条件递增和唯一终态取走
    */
    static void contention();

    /** 编号 6，暂停受保护读者、未发布构造及回收者，检查 clear 和有限池进展
    */
    static void paused();

    /** 编号 7，独立检查包含重叠 clear 的并发历史可线性化
    */
    static void histories();

    /** 编号 8，全碰撞持续更新并同时维护和遍历，校验各生产者的精确值
    */
    static void stress();

    /** 编号 9，测量低、高、全碰撞的延迟和保留资源，仅作诊断负载
    */
    static void performance();

    /** 编号 10，检查冷桶 clear、身份溢出、异常、预留暂停、保护耗尽及优化反例
    */
    static void boundaries();

    /** 穷举与真实时间先后关系一致的合法顺序历史，不调用被测表
    @param [in] history 已完成的操作记录，最多 12 项
    @param [in] used 已放入顺序历史的操作位图
    @param [in] state 独立 std::map 顺序状态副本
    @return 存在合法完整顺序为 true；模型分配异常由用例入口捕获
    */
    static bool linearize(const std::vector<LockFreeHashMapHistory>& history, unsigned used, std::map<int, int> state);
};