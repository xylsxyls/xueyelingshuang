#pragma once
#include "LockFreeHashMapAPI.h"
#include "LockFreeHashMapPoolTests.h"
#include "LockFreeHashMapTestThread.h"

/** 把不同整数键全部映射到同一桶，同时提供哈希异常注入
*/
struct LockFreeHashMapRewriteHash
{
public:
    // 全线程可见的测试故障开关
    static std::atomic<bool> s_throw;
public:
    /** 返回常量哈希或抛测试异常
    @param [in] key 键，仅在调用期间借用
    @return 零，制造完全碰撞
    */
    size_t operator()(const int& key) const;
};

/** 比较异常注入，检查异常不能当作不存在
*/
struct LockFreeHashMapRewriteEqual
{
public:
    // 比较故障开关
    static std::atomic<bool> s_throw;
public:
    /** 按整数等价关系比较
    @param [in] left 已发布键
    @param [in] right 查询键
    @return 等价结果，故障时抛测试异常
    */
    bool operator()(const int& left, const int& right) const;
};

/** 不可默认构造的拥有型测试值，支持复制异常及复制期间暂停
*/
struct LockFreeHashMapRewriteValue
{
public:
    // 独立业务值
    int m_value;
    // 当前活对象数
    LOCKFREEHASHMAP_TEST_ALIGN_64 static std::atomic<int64_t> s_live;
    // 复制故障开关
    static std::atomic<bool> s_throw;
    // 指定值复制时暂停，负一关闭
    static std::atomic<int> s_pause;
    // 复制线程已进入暂停点
    static std::atomic<bool> s_entered;
    // 允许复制线程继续
    static std::atomic<bool> s_resume;
public:
    /** 构造业务值并计账
    @param [in] value 业务标记
    */
    explicit LockFreeHashMapRewriteValue(int value);

    /** 复制业务值，可注入异常和暂停
    @param [in] other 受保护的来源
    */
    LockFreeHashMapRewriteValue(const LockFreeHashMapRewriteValue& other);

    /** 赋值与复制使用相同故障开关
    @param [in] other 受保护的来源
    @return 当前对象
    */
    LockFreeHashMapRewriteValue& operator=(const LockFreeHashMapRewriteValue& other);

    /** 归还活对象计账
    */
    ~LockFreeHashMapRewriteValue();
};

/** 新算法的独立模型与并发测试，所有退出都正常返回结果
*/
class LockFreeHashMapRewriteTests
{
public:
    /** 运行模块、模型、版本、暂停及高碰撞交错测试
    @param [in] alignmentOnly 为 true 时仅执行原子对齐回归，用于修复前后独立对照
    @return 零为通过，非零为失败
    */
    static int run(bool alignmentOnly = false);
private:
    /** 记录可核对的预期，失败抛出可捕获测试状态以结束依赖步骤
    @param [in] condition 预期结果
    @param [in] name 用例上下文
    */
    static void check(bool condition, const char* name);

    /** 核对栈、堆、数组、池和缓存中的真实原子字段地址，不执行错位的原子操作
    */
    static void alignment();

    /** 核对一个仍存活字段的八字节对齐，失败记录实际地址
    @param [in] field 被测字段地址，只观察数值，不解引用
    @param [in] name 用例上下文
    */
    static void aligned(const void* field, const char* name);

    /** 使用独立标准容器模型核对旧接口和完整碰撞
    */
    static void model();

    /** 核对版本、独立引用、跨实例、清空和池满删除
    */
    static void versions();

    /** 覆盖内存、哈希、比较、复制及回调故障
    */
    static void exceptions();

    /** 核对副本缓存完整身份、无效标记、暂停发布者及代次边界
    */
    static void scalarCache();

    /** 并发修改及清空时核对整数键值编码，完成后核对最后写入版本
    */
    static void scalarStress();

    /** 重复构造大表、逐条预填充、覆盖、清空和释放，核对全部键及每轮分配责任
    */
    static void largeTableLifecycle();

    /** 让读取的复制停住，同时删除、清空和重插
    */
    static void paused();

    /** 高冲突下核对唯一插入、条件累加和唯一取走
    */
    static void contention();

    /** 多线程混合全碰撞及 clear，核对值编码和最终独立全集
    */
    static void collisions();
private:
    // 所有工作线程共享的检查数量
    LOCKFREEHASHMAP_TEST_ALIGN_64 static std::atomic<uint64_t> s_checks;
    // 所有工作线程共享的失败数量
    LOCKFREEHASHMAP_TEST_ALIGN_64 static std::atomic<uint64_t> s_failed;
};

/** 普通聚合嵌入场景，前置一个字节后仍须由类型本身保证所有表原子的对齐
*/
struct LockFreeHashMapAlignmentFixture
{
public:
    // 模拟业务结构中的非对齐大小前缀
    char m_prefix;
    // 连续嵌入的两个表，各自独立拥有资源
    LockFreeHashMap<int, int> m_maps[2];
public:
    /** 按正常成员布局构造，不对对象地址作人工修正
    */
    LockFreeHashMapAlignmentFixture();
};

/** 仅在测试编译中访问缓存发布状态，不给正式接口增加测试开关
*/
class LockFreeHashMapValueCacheTestAccess
{
public:
    /** 借用指定固定格，仅供测试夹具的生命周期内使用
    @param [in,out] cache 已初始化的一格缓存
    @return 第一格的测试引用
    */
    static LockFreeHashMapValueCacheEntry& first(LockFreeHashMapValueCache<uint64_t, uint64_t, std::equal_to<uint64_t>>& cache);
};