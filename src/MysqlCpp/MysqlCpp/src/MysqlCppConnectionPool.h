#pragma once
#include "MysqlCppConfig.h"
#include "MysqlCppMacro.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <vector>

class MysqlCpp;

/** MySQL连接池，提前维护固定数量连接，调用acquire后用shared_ptr自动归还连接
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
    /** 初始化连接池
    @param [in] config 每个连接使用的MySQL配置
    @param [in] connectionCount 需要创建的连接数量
    @return 返回是否全部连接创建成功
    */
    bool init(const MysqlCppConfig& config, size_t connectionCount);

    /** 反初始化连接池，关闭并释放池内所有连接
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
    // 连接池生命周期标记，外借连接的删除器会通过它判断池对象是否仍然存在
    std::shared_ptr<bool> m_aliveFlag;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 当前连接池是否已经完成init
    bool m_isInit;
};