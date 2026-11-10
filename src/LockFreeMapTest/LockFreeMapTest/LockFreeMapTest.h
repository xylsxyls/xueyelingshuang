#pragma once
#include <atomic>
#include <stdint.h>
#include <cstdio>

/** 当前批次报告句柄，仅测试入口打开和关闭
*/
struct LockFreeMapTestReport
{
public:
    // 用例只借用此句柄，避免运行期控制台输出
    static FILE* m_file;
};

/** 稳定测试编号；原 LM 编号不因迁移发生变化
*/
enum LockFreeMapTestId
{
    // 与独立有序容器进行随机对照
    LOCK_FREE_MAP_TEST_BASIC = 1,
    // 版本凭据与地址复用
    LOCK_FREE_MAP_TEST_TOKENS = 2,
    // 用户代码异常
    LOCK_FREE_MAP_TEST_EXCEPTIONS = 3,
    // 同键插入的唯一胜者
    LOCK_FREE_MAP_TEST_SAME_KEY = 4,
    // 暂停读者与容量拒绝
    LOCK_FREE_MAP_TEST_PAUSED_READER = 5,
    // 最高优先级条件提取竞争
    LOCK_FREE_MAP_TEST_MAXIMUM = 6,
    // 清空与未发布写入竞争
    LOCK_FREE_MAP_TEST_CLEAR = 7,
    // 双池满容量和释放
    LOCK_FREE_MAP_TEST_FULL = 8,
    // 动态缩容与重新增长
    LOCK_FREE_MAP_TEST_SHRINK = 9,
    // 混合并发压力
    LOCK_FREE_MAP_TEST_STRESS = 10,
    // 借用对象所有权
    LOCK_FREE_MAP_TEST_OWNERSHIP = 11,
    // 每个申请点失败注入
    LOCK_FREE_MAP_TEST_ALLOCATIONS = 12,
    // 参数、重入及版本上限
    LOCK_FREE_MAP_TEST_LIMITS = 13,
    // 并发池预留额度
    LOCK_FREE_MAP_TEST_POOL_CONCURRENCY = 14,
    // 暂停析构时其他操作推进
    LOCK_FREE_MAP_TEST_PAUSED_RECLAIMER = 15,
    // 历史键版本有界
    LOCK_FREE_MAP_TEST_HISTORY_BOUND = 16,
    // 池模块的独立边界
    LOCK_FREE_MAP_TEST_POOL = 17,
    // 六端一致的接口编译与调用样例
    LOCK_FREE_MAP_TEST_EXAMPLES = 18,
    // 原有五十二项兼容性检查
    LOCK_FREE_MAP_TEST_HISTORICAL = 19,
    // 暂停分配时的额度统计
    LOCK_FREE_MAP_TEST_PAUSED_GROWTH = 20,
    // 提取与旧版本退役竞争
    LOCK_FREE_MAP_TEST_RETIRED_VERSION = 21,
    // 发布者暂停后的描述符帮助
    LOCK_FREE_MAP_TEST_HELPING = 22,
    // 无系统申请的真实更新与条件累加
    LOCK_FREE_MAP_TEST_FAST_UPDATES = 23,
    // 平台原子宽度、对齐和低内存
    LOCK_FREE_MAP_TEST_PLATFORM = 24,
    // 四库公开头共存与隔离调用
    LOCK_FREE_MAP_TEST_HEADERS = 25,
    // 池槽直接构造键及失败回滚
    LOCK_FREE_MAP_TEST_DIRECT_KEY = 26,
    // 参数错误和报告打开失败的退出码
    LOCK_FREE_MAP_TEST_ARGUMENTS = 27,
    // 保护解除后，小预算维护也能排空退役对象
    LOCK_FREE_MAP_TEST_SMALL_BUDGET = 28,
    // 索引引用转交、单项覆盖及暂停发布者
    LOCK_FREE_MAP_TEST_SHARED_INDEX = 29,
    // 空槽缓存转交、暂停持有者及释放后重建
    LOCK_FREE_MAP_TEST_SLOT_CACHE = 30,
    // 删除独占载荷时暂停析构，容量拒绝及恢复后复用
    LOCK_FREE_MAP_TEST_DETACHED_DESTRUCTION = 31,
    // 累计失败者暂停时其他记录仍能推进，读写计数及记录正常归还
    LOCK_FREE_MAP_TEST_CONTENTION = 32,
    // 持续生产、读取、提取及记录不足时的逐线程进展
    LOCK_FREE_MAP_TEST_PROGRESS = 33,
    // 无关键变化不拖延精确查询，旧视图与删除重插和清空正确交错
    LOCK_FREE_MAP_TEST_QUERY_PROGRESS = 34,
    // 普通状态接口内部重试、条件失效和真实资源不足分离
    LOCK_FREE_MAP_TEST_STATUS_CONTRACT = 35,
    // 热线程持续运行后加入其他生产和消费线程，各观察窗口均须推进
    LOCK_FREE_MAP_TEST_LATE_ARRIVAL = 36
};

/** 申请故障注入状态，仅测试入口显式开启
*/
struct LockFreeMapTestFaults
{
public:
    // 剩余成功申请次数，负数不注入
    static std::atomic<int64_t> m_failAfter;
    // 防止运行库初始化期间提前注入故障
    static std::atomic<int> m_enabled;
    // 第几次申请暂停，负数不暂停
    static std::atomic<int> m_pauseAllocation;
    // 成功申请总数
    static std::atomic<uint64_t> m_allocations;
    // 尚未释放的申请数
    static std::atomic<int64_t> m_outstanding;
};

/** 执行独立有序参考对照
@return 失败数，零表示全部通过
*/
int LockFreeMapBasic();

/** 执行池模块边界测试
@return 零表示全部通过
*/
int LockFreeMapPoolModule();

/** 检查失败计数不拥有资源、不等待暂停线程且记录正常归还
@return 独立协议检查通过时返回零
*/
int LockFreeMapContentionProtocol();

/** 持续竞争下分别核对生产和消费进展及所有接纳条目的完整归属
@return 持续有工作场景逐线程成功、混合场景逐线程返回且整体提交、内容核对一致时返回零
*/
int LockFreeMapConcurrentProgress();

/** 检查精确查询的进展、重叠删除和清空时的值与版本生命期
@return 所有受控交错符合独立预期时返回零
*/
int LockFreeMapQueryProgress();

/** 编译并执行六端相同的接口样例
@return 零表示全部通过
*/
int LockFreeMapCompileExamples();

/** 执行原有五十二项兼容性检查
@return 零表示全部通过
*/
int LockFreeMapHistorical();

/** 执行当前目标的原子、对齐及分配失败检查
@return 零表示全部通过
*/
int LockFreeMapPlatform();

/** 执行四库公开头共存检查
@return 零表示全部通过
*/
int LockFreeMapHeaders();

/** 测试入口，支持全部、即时、压力及指定编号四种模式
@param [in] argc 参数个数
@param [in] argv 参数数组
@return 0 通过，1 测试失败，2 参数错误，3 报告写入失败
*/
int main(int argc, char** argv);