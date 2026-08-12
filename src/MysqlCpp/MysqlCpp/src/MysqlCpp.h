#pragma once
#include "MysqlCppConfig.h"
#include "MysqlCppExecuteResult.h"
#include "MysqlCppMacro.h"
#include "MysqlCppResultSet.h"
#include "MysqlCppPrepareStatement.h"
#include <memory>
#include <stdint.h>
#include <string>

namespace sql
{
    class Driver;
    class Connection;
}

/** MySQL连接封装，隐藏mysql-connector-cpp底层类型，提供连接、查询、更新和事务操作
*/
class MysqlCppAPI MysqlCpp
{
public:
    /** 构造函数，只做成员变量默认值初始化，不获取驱动也不连接服务器
    */
    MysqlCpp();

    /** 析构函数，兜底关闭当前MySQL连接，重复uninit安全
    */
    ~MysqlCpp();

public:
    /** 初始化MySQL驱动入口，可在connect前显式调用，也会在首次connect时自动触发
    @return 返回true表示底层驱动入口获取成功
    */
    bool init();

    /** 反初始化MySQL连接对象，关闭当前连接并清空驱动状态
    */
    void uninit();

    /** 判断当前对象是否已经完成初始化
    @return 返回true表示已经成功调用init
    */
    bool isInit() const;

    /** 兼容旧接口的连接函数，只设置地址、端口、账号和密码，不自动选择数据库
    @param [in] ip MySQL服务器IP或域名
    @param [in] port MySQL端口
    @param [in] account 登录账号
    @param [in] password 登录密码
    @return 返回是否连接成功
    */
    bool connect(const std::string& ip,
                 int32_t port,
                 const std::string& account,
                 const std::string& password);

    /** 按完整配置建立连接，连接成功后会按配置选择数据库和设置自动提交
    @param [in] config 连接配置
    @return 返回是否连接成功
    */
    bool connect(const MysqlCppConfig& config);

    /** 关闭当前连接，重复调用安全
    */
    void close();

    /** 使用上一次保存的连接配置重连当前连接
    @return 返回是否重连成功
    */
    bool reconnect();

    /** 检测当前连接是否仍可用，失败时不会抛异常
    @return 返回连接是否可访问MySQL服务器
    */
    bool ping() const;

    /** 检测本对象是否持有可用连接对象
    @return 返回当前连接对象是否存在且底层认为有效
    */
    bool isConnected() const;

    /** 获取最近一次失败原因
    @return 返回错误文本，成功后通常为空字符串
    */
    std::string lastError() const;

    /** 选择当前连接使用的数据库，并设置自动提交策略
    @param [in] dbName 数据库名
    @param [in] autoCommit 是否自动提交事务
    @return 返回是否选择成功
    */
    bool selectDb(const std::string& dbName, bool autoCommit = false);

    /** 兼容旧接口的预处理语句创建函数
    @param [in] sqlString 带占位符的SQL语句
    @return 返回预处理语句对象，创建失败时返回空对象或无效对象
    */
    std::shared_ptr<MysqlCppPrepareStatement> preparedStatementCreator(const std::string& sqlString);

    /** 创建预处理语句
    @param [in] sqlString 带占位符的SQL语句
    @return 返回预处理语句对象，创建失败时返回空对象或无效对象
    */
    std::shared_ptr<MysqlCppPrepareStatement> prepare(const std::string& sqlString);

    /** 兼容旧接口的执行函数，会根据SQL类型返回结果集
    @param [in] statement 已绑定参数的预处理语句
    @param [in] isCommit 执行后是否提交事务
    @return 查询SQL返回结果集，非查询SQL返回空结果集
    */
    std::shared_ptr<MysqlCppResultSet> execute(const std::shared_ptr<MysqlCppPrepareStatement>& statement, bool isCommit = true);

    /** 执行查询SQL并返回结果集
    @param [in] statement 已绑定参数的预处理语句
    @param [in] isCommit 执行后是否提交事务
    @return 返回结果集，执行失败时返回无效结果集
    */
    std::shared_ptr<MysqlCppResultSet> query(const std::shared_ptr<MysqlCppPrepareStatement>& statement, bool isCommit = true);

    /** 执行INSERT/UPDATE/DELETE等非查询SQL
    @param [in] statement 已绑定参数的预处理语句
    @param [in] isCommit 执行后是否提交事务
    @return 返回执行结果，包括影响行数、自增ID和错误信息
    */
    MysqlCppExecuteResult executeUpdate(const std::shared_ptr<MysqlCppPrepareStatement>& statement, bool isCommit = true);

    /** 开启事务，会把当前连接切到非自动提交
    @return 返回是否开启成功
    */
    bool beginTransaction();

    /** 提交当前事务
    @return 返回是否提交成功
    */
    bool commit();

    /** 回滚当前事务
    @return 返回是否回滚成功
    */
    bool rollback();

    /** 执行mysql命令行导入SQL文件
    @param [in] sqlPath SQL文件路径
    @param [in] host MySQL服务器地址
    @param [in] user 登录账号
    @param [in] password 登录密码
    @param [in] database 目标数据库名
    */
    void importSql(const std::string& sqlPath,
                   const std::string& host,
                   const std::string& user,
                   const std::string& password,
                   const std::string& database);

    /** 执行mysql命令行导出数据
    @param [in] sqlPath 导出文件保存路径
    @param [in] host MySQL服务器地址
    @param [in] user 登录账号
    @param [in] password 登录密码
    @param [in] exportData mysqldump后面的导出目标参数
    */
    void exportSql(const std::string& sqlPath,
                   const std::string& host,
                   const std::string& user,
                   const std::string& password,
                   const std::string& exportData);

    /** 设置自动提交状态
    @param [in] autoCommit true表示每条SQL自动提交，false表示由调用方显式提交或回滚
    */
    void setAutoCommit(bool autoCommit);

protected:
    /** 内部连接有效性检查
    @return 返回当前连接是否可继续执行SQL
    */
    bool check() const;

    /** 保存最近一次错误文本
    @param [in] error 错误文本，传空字符串表示清空错误
    */
    void setLastError(const std::string& error);

    /** 查询当前连接最后一次INSERT产生的自增ID
    @return 返回自增ID，没有或查询失败时返回0
    */
    uint64_t queryLastInsertId() const;

private:
    /** 禁止拷贝构造，避免多个对象同时释放同一底层连接
    @param [in] other 另一个MySQL连接对象
    */
    MysqlCpp(const MysqlCpp& other);

    /** 禁止赋值，避免多个对象同时释放同一底层连接
    @param [in] other 另一个MySQL连接对象
    @return 不会被调用
    */
    MysqlCpp& operator=(const MysqlCpp& other);

private:
    // mysql-connector-cpp驱动对象，不由本类释放
    sql::Driver* m_driver;
    // 当前连接对象，由本类负责释放
    sql::Connection* m_con;
    // 当前对象是否已经完成init
    bool m_isInit;
    // 最近一次成功或尝试连接使用的配置，用于重连和恢复数据库设置
    MysqlCppConfig m_config;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 最近一次操作失败时的错误信息
    std::string m_lastError;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};