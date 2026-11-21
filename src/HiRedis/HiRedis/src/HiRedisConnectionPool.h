#pragma once
#include "HiRedisConfig.h"
#include "HiRedisMacro.h"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <vector>

class HiRedis;

/** Redis连接池，用于复用多个HiRedis连接并控制并发访问
*/
class HiRedisAPI HiRedisConnectionPool
{
public:
    /** 构造函数
    */
    HiRedisConnectionPool();

    /** 析构函数，会兜底关闭连接池，调用方仍应在退出前主动调用uninit
    */
    ~HiRedisConnectionPool();

public:
    /** 按配置的空闲目标和总量上限初始化，默认预建一个、最多一个
    @param [in] config 包含连接参数及 m_poolMinConnections、m_poolMaxConnections
    @return 成功或已初始化时为 true，范围无效、仍有旧借用或资源不足时为 false
    */
    bool init(const HiRedisConfig& config);

    /** 初始化连接池，已经初始化时直接返回true，不会重建已有连接
    @param [in] config Redis连接配置
    @param [in] connectionCount 连接池大小
    @return 返回是否所有连接都创建成功
    */
    bool init(const HiRedisConfig& config, size_t connectionCount);

    /** 反初始化连接池，关闭空闲连接，正在借出的连接会在归还时释放，未初始化时直接返回
    */
    void uninit();

    /** 关闭连接池并释放连接，等同于uninit
    */
    void close();

    /** 获取一个连接，shared_ptr析构时自动归还连接池
    @param [in] timeoutMs 等待空闲连接的超时时间，负数表示一直等待
    @return 返回连接对象，超时或连接池关闭时返回空指针
    */
    std::shared_ptr<HiRedis> acquire(int32_t timeoutMs = 3000);

    /** 获取当前空闲连接数量
    @return 返回可立即获取的连接数量
    */
    size_t availableCount();

    /** 获取连接池总连接数量
    @return 返回连接池持有的连接总数
    */
    size_t totalCount();

    /** 获取最近一次错误信息
    @return 返回错误文本，没有错误返回空字符串
    */
    std::string lastError();

    /** 供HiRedisConnectionReleaser归还连接使用，业务代码不要直接调用
    @param [in] connection 需要归还的连接
    */
    void releaseConnectionFromReleaser(HiRedis* connection);

private:
    friend class HiRedisConnectionReleaser;

    /** 按统一的固定或弹性边界构造池
    @param [in] config 每个连接使用的参数
    @param [in] minimum 预建数量及空闲目标
    @param [in] maximum 总量上限，-1 表示不限制
    @return 是否初始化成功
    */
    bool initRange(const HiRedisConfig& config, size_t minimum, int64_t maximum);

    /** 在已保存的配置下创建一条连接，失败后对象随局部所有权释放
    @param [out] error 建连失败原因
    @return 成功时返回独占连接，否则为空
    */
    std::unique_ptr<HiRedis> createConnection(std::string& error);

    /** 将连接归还到空闲队列
    @param [in] connection 要归还的连接对象
    */
    void releaseConnection(HiRedis* connection);

    /** 拷贝构造函数禁用，连接池不能被复制
    @param [in] other 另一个Redis连接池对象
    */
    HiRedisConnectionPool(const HiRedisConnectionPool& other);

    /** 赋值函数禁用，连接池不能被复制
    @param [in] other 另一个Redis连接池对象
    @return 不会被调用
    */
    HiRedisConnectionPool& operator=(const HiRedisConnectionPool& other);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 保护连接池内部状态的互斥锁
    std::mutex m_mutex;
    // 等待和唤醒空闲连接的条件变量
    std::condition_variable m_condition;
    // 连接池创建并负责释放的全部连接
    std::vector<HiRedis*> m_allConnections;
    // 当前空闲、可被acquire取走的连接
    std::vector<HiRedis*> m_availableConnections;
    // 最近一次连接池操作的错误信息
    std::string m_lastError;
    // 借用归还与池析构通过同一把锁串行，删除器可以比池本身活得更久
    std::shared_ptr<std::mutex> m_returnGate;
    // 本轮连接使用的配置，创建请求全部结束前禁止重新初始化
    HiRedisConfig m_config;
    // 预建和空闲保留数
    size_t m_minimum;
    // 总连接上限，-1 可按需继续增长
    int64_t m_maximum;
    // 已预占名额、正在锁外执行建连的请求数
    size_t m_pending;
    // 成功初始化一次递增；关闭期间被唤醒的旧借用不能进入下一轮池
    uint64_t m_generation;
    // 连接池生命周期标记，外借连接的删除器会通过它判断池对象是否仍然存在
    std::shared_ptr<std::atomic<bool>> m_aliveFlag;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 当前连接池是否已经完成init
    bool m_isInit;
};