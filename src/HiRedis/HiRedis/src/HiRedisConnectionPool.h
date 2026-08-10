#pragma once
#include "HiRedisConfig.h"
#include "HiRedisMacro.h"
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

    /** 析构函数，不做反初始化，调用方需要在退出前主动调用uninit
    */
    ~HiRedisConnectionPool();

public:
    /** 初始化连接池
    @param [in] config Redis连接配置
    @param [in] connectionCount 连接池大小
    @return 返回是否所有连接都创建成功
    */
    bool init(const HiRedisConfig& config, size_t connectionCount);

    /** 反初始化连接池，关闭空闲连接，正在借出的连接会在归还时释放
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

private:
    /** shared_ptr自定义释放器，用于把连接归还给连接池
    */
    struct ConnectionReleaser
    {
        // 连接归还的目标连接池
        HiRedisConnectionPool* m_pool;

        /** 归还连接
        @param [in] connection 要归还的连接对象
        */
        void operator()(HiRedis* connection) const;
    };

private:
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
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 当前连接池是否已经完成init
    bool m_isInit;
};