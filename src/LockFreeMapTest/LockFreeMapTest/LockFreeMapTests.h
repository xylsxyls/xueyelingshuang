#pragma once
#include "LockFreeMapTest.h"
#include "LockFreeMapTestThread.h"
#include "LockFreeMap/LockFreeMapAPI.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

/** 受控用户代码异常和暂停，不向生产实现注入测试状态
*/
struct LockFreeMapTestValue
{
public:
    // 独立载荷编号
    int m_value;
    // 在受保护读取中暂停此赋值目标
    bool m_pauseAssignment;
    // 完整构造的载荷计数
    static std::atomic<int> m_live;
    // 注入载荷构造失败
    static std::atomic<int> m_throwCopy;
    // 注入载荷赋值失败
    static std::atomic<int> m_throwAssignment;
    // 暂停此编号的构造，负一表示关闭
    static std::atomic<int> m_pauseCopyValue;
    // 暂停此编号的最终析构，负一表示关闭
    static std::atomic<int> m_pauseDestructorValue;
    // 发布已到达受控暂停点
    static std::atomic<int> m_entered;
    // 解除受控暂停
    static std::atomic<int> m_release;
    // 暂停同步超时累计数，禁止把超时自行解除当作成功推进
    static std::atomic<size_t> m_timeouts;

public:
    /** 构造计数受跟踪的载荷
    @param [in] value 测试编号
    */
    explicit LockFreeMapTestValue(int value = 0);

    /** 复制时注入受控异常或暂停
    @param [in] other 原对象
    */
    LockFreeMapTestValue(const LockFreeMapTestValue& other);

    /** 准确释放一个被跟踪载荷
    */
    ~LockFreeMapTestValue();

    /** 赋值时注入受控异常或暂停
    @param [in] other 原对象
    @return 当前载荷
    */
    LockFreeMapTestValue& operator=(const LockFreeMapTestValue& other);

    /** 在确定的测试同步点等待
    */
    static void pause();
};

/** 比较器等价关系独立于相等运算符
*/
struct LockFreeMapTestLess
{
public:
    // 注入比较异常
    static std::atomic<int> m_throw;
    // 注入比较器构造失败
    static std::atomic<int> m_throwConstructor;

public:
    /** 构造比较器或注入构造失败
    */
    LockFreeMapTestLess();

    /** 按十位分组比较，与对象身份无关
    @param [in] left 左操作数
    @param [in] right 右操作数
    @return 严格弱序比较结果
    */
    bool operator()(const LockFreeMapTestValue& left, const LockFreeMapTestValue& right) const;
};

/** 拥有独立判定条件的稳定编号测试用例
*/
class LockFreeMapTests
{
public:
    /** 验证键仅在池槽中复制一次，构造失败退还全部私有资源
    @return 复制次数、旧值和资源计数均满足独立预期时返回 true
    */
    static bool directKeyConstruction();

    /** 检查单引用预算的维护能保留暂停读者并最终排空退役对象
    @return 保护期间旧值有效，解除保护后旧对象和索引均释放时通过
    */
    static bool smallBudget();

    /** 验证共享索引反复插删、旧视图隔离和暂停发布者的引用转交
    @return 无申请循环、旧凭据拒绝、暂停时推进和全部资源收尾均正确时通过
    */
    static bool sharedIndex();

    /** 检查已析构空槽的预算、独占转交、异常和静止期释放
    @return 对象及时析构、其他调用不等待且同域不同池不混用时通过
    */
    static bool slotCache();

    /** 检查删除中的最后一个载荷暂停时，槽仍被占用且其他键可以推进
    @return 容量拒绝、解除暂停后的复用、旧凭据拒绝和资源归还均正确时通过
    */
    static bool detachedDestruction();

    /** 仅在测试框架中等待，最长十秒
    @param [in] flag 解除等待的标志
    @return 超时前解除等待时返回 true
    */
    static bool wait(std::atomic<int>& flag);

    /** 检查比较等价关系和地址复用后的旧凭据
    @return 全部独立预期通过时返回 true
    */
    static bool tokens();

    /** 检查构造、复制、比较器和回调异常
    @return 全部独立预期通过时返回 true
    */
    static bool exceptions();

    /** 检查唯一成功者及实际返回值
    @return 全部独立预期通过时返回 true
    */
    static bool sameKey();

    /** 检查暂停读者时的保留上限和满池拒绝
    @return 全部独立预期通过时返回 true
    */
    static bool pausedReader();

    /** 检查更高键插入使旧最大键条件提取失效
    @return 全部独立预期通过时返回 true
    */
    static bool maximumRace();

    /** 检查未发布写者不能恢复已清空的根
    @return 全部独立预期通过时返回 true
    */
    static bool clearRace();

    /** 检查双池独立预算和满容量删除
    @return 全部独立预期通过时返回 true
    */
    static bool fullPools();

    /** 检查动态策略、容量下限、关闭缩容及重新增长
    @return 全部独立预期通过时返回 true
    */
    static bool shrinking();

    /** 用值关系和有序性不变量检查并发混合操作
    @return 全部独立预期通过时返回 true
    */
    static bool stress();

    /** 检查借用所有权和 BCAD 适配时序
    @return 全部独立预期通过时返回 true
    */
    static bool ownership();

    /** 逐个申请点注入失败并验证清理和重试
    @return 全部独立预期通过时返回 true
    */
    static bool allocations();

    /** 检查策略失败、各类上限和计数不回绕
    @return 全部独立预期通过时返回 true
    */
    static bool limits();

    /** 检查并发扩容预留和槽位复用
    @return 全部独立预期通过时返回 true
    */
    static bool poolConcurrency();

    /** 检查延迟析构时无关操作继续推进
    @return 全部独立预期通过时返回 true
    */
    static bool pausedReclaimer();

    /** 检查历史键反复更换不会累积保留对象
    @return 全部独立预期通过时返回 true
    */
    static bool historyBound();

    /** 检查分配器发布前暂停时仍能观察剩余额度
    @return 预留有界且无关操作正常返回时返回 true
    */
    static bool pausedGrowth();

    /** 使暂停的条件提取与其版本退役竞争
    @return 旧提取失败且替换版本保留时返回 true
    */
    static bool retiredVersionRace();

    /** 检查发布者封存私有描述符之前的帮助完成
    @return 成功、冲突及清空路径均不重新发布旧索引时返回 true
    */
    static bool descriptorHelping();

    /** 检查无系统申请的真实更新和并发条件累加
    @return 最终值准确、旧凭据失效且对象保留有界时返回 true
    */
    static bool fastUpdates();

    /** 回收全部测试线程后观察进程资源
    @param [out] threads Windows 当前进程原生线程数
    @param [out] handles Windows 当前进程句柄数
    @return 系统资源观察成功时返回 true
    */
    static bool resources(size_t& threads, size_t& handles);

};