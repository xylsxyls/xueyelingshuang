#pragma once
#include "MysqlCppConfig.h"
#include "MysqlCppMacro.h"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <vector>

class MysqlCpp;

/** MySQL连接池，根据配置预建连接并可按需扩容，调用acquire后用shared_ptr自动归还连接
*/
class MysqlCppAPI MysqlCppConnectionPool
{
public:
    /** 构造函数，只做成员变量默认值初始化，不创建连接
    */
    MysqlCppConnectionPool();

    /** 析构函数，会兜底关闭连接池，调用方仍应在退出前主动调用uninit
    */
    ~MysqlCppConnectionPool();

public:
    /** 按配置的空闲目标和总量上限初始化，默认预建一个、最多一个
    @param [in] config 包含连接参数及 m_poolMinConnections、m_poolMaxConnections
    @return 成功或已初始化时为 true，范围无效、仍有旧借用或资源不足时为 false
    */
    bool init(const MysqlCppConfig& config);

    /** 初始化连接池，已经初始化时直接返回true，不会重建已有连接
    @param [in] config 每个连接使用的MySQL配置
    @param [in] connectionCount 需要创建的连接数量
    @return 返回是否全部连接创建成功
    */
    bool init(const MysqlCppConfig& config, size_t connectionCount);

    /** 反初始化连接池，关闭空闲连接，正在借出的连接会在归还时释放，未初始化时直接返回
    */
    void uninit();

    /** 关闭连接池并释放所有连接，正在借出的连接会在归还时释放
    */
    void close();

    /** 获取一个可用连接
    @param [in] timeoutMs 等待连接归还的超时时间，单位毫秒
    @return 返回连接对象，超时或连接池未初始化时返回空shared_ptr
    */
    std::shared_ptr<MysqlCpp> acquire(int32_t timeoutMs = 3000);

    /** 获取当前空闲连接数量
    @return 返回可立即借出的连接数
    */
    size_t availableCount();

    /** 获取连接池总连接数量
    @return 返回已创建且由连接池管理的连接数
    */
    size_t totalCount();

    /** 获取最近一次失败原因
    @return 返回错误文本，成功后通常为空字符串
    */
    std::string lastError();

    /** 供MysqlCppConnectionReleaser归还连接使用，业务代码不要直接调用
    @param [in] connection 需要归还的连接
    */
    void releaseConnectionFromReleaser(MysqlCpp* connection);

private:
    friend class MysqlCppConnectionReleaser;

    /** 按统一的固定或弹性边界构造池
    @param [in] config 每个连接使用的参数
    @param [in] minimum 预建数量及空闲目标
    @param [in] maximum 总量上限，-1 表示不限制
    @return 是否初始化成功
    */
    bool initRange(const MysqlCppConfig& config, size_t minimum, int64_t maximum);

    /** 在已保存的配置下创建一条连接，失败后对象随局部所有权释放
    @param [out] error 建连失败原因
    @return 成功时返回独占连接，否则为空
    */
    std::unique_ptr<MysqlCpp> createConnection(std::string& error);

    /** 将连接放回空闲队列并唤醒等待线程
    @param [in] connection 需要归还的连接
    */
    void releaseConnection(MysqlCpp* connection);

    /** 禁止拷贝构造，避免多个连接池同时管理同一批连接
    @param [in] other 另一个连接池对象
    */
    MysqlCppConnectionPool(const MysqlCppConnectionPool& other);

    /** 禁止赋值，避免多个连接池同时管理同一批连接
    @param [in] other 另一个连接池对象
    @return 不会被调用
    */
    MysqlCppConnectionPool& operator=(const MysqlCppConnectionPool& other);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 保护连接池内部队列和错误文本
    std::mutex m_mutex;
    // 等待空闲连接归还时使用的条件变量
    std::condition_variable m_condition;
    // 连接池创建并拥有的所有连接
    std::vector<MysqlCpp*> m_allConnections;
    // 当前可借出的空闲连接
    std::vector<MysqlCpp*> m_availableConnections;
    // 最近一次初始化或获取连接失败原因
    std::string m_lastError;
    // 借用归还与池析构通过同一把锁串行，删除器可以比池本身活得更久
    std::shared_ptr<std::mutex> m_returnGate;
    // 本轮连接使用的配置，创建请求全部结束前禁止重新初始化
    MysqlCppConfig m_config;
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