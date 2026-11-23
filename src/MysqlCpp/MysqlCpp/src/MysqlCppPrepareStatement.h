#pragma once
#include "MysqlCppMacro.h"
#include "mysql-connector-cpp/cppconn/connection.h"
#include <memory>
#include <stdint.h>
#include <istream>
#include <string>

struct MysqlCppExecution;
class MysqlCppResultSet;

/** MySQL预编译语句封装，用于绑定参数后交给MysqlCpp执行
*/
class MysqlCppAPI MysqlCppPrepareStatement
{
public:
    /** 析构函数，释放底层预编译语句对象
    */
    ~MysqlCppPrepareStatement();

public:
    /** 释放原生语句和它的活动结果，重复关闭安全
    */
    void close();

    /** 判断预编译语句是否可用
    @return 返回是否属于有效会话且创建及绑定均成功；绑定失败后需重新 prepare
    */
    bool isValid() const;

    /** 获取本语句创建或参数绑定失败的原因
    @return 无上述错误时为空；关闭连接会使语句失效，但不产生绑定错误
    */
    std::string lastError() const;

    /** 获取创建预编译语句时使用的SQL文本
    @return 返回SQL文本，创建失败时返回原始传入文本
    */
    std::string sql() const;

    /** 绑定大整数字符串参数，下标从0开始
    @param [in] parameterIndex 参数下标
    @param [in] value 要绑定的大整数字符串
    */
    void setBigInt(uint32_t parameterIndex, const std::string& value);

    /** 绑定二进制流参数，下标从0开始
    @param [in] parameterIndex 参数下标
    @param [in] blob 调用方拥有的可读输入流，绑定后保持有效直至该次语句执行完；SQL NULL 使用 setNull
    */
    void setBlob(uint32_t parameterIndex, std::istream* blob);

    /** 绑定布尔参数，下标从0开始
    @param [in] parameterIndex 参数下标
    @param [in] value 要绑定的布尔值
    */
    void setBoolean(uint32_t parameterIndex, bool value);

    /** 绑定日期时间字符串参数，下标从0开始
    @param [in] parameterIndex 参数下标
    @param [in] value 要绑定的日期时间字符串
    */
    void setDateTime(uint32_t parameterIndex, const std::string& value);

    /** 绑定双精度浮点参数，下标从0开始
    @param [in] parameterIndex 参数下标
    @param [in] value 要绑定的浮点值
    */
    void setDouble(uint32_t parameterIndex, double value);

    /** 绑定32位有符号整数参数，下标从0开始
    @param [in] parameterIndex 参数下标
    @param [in] value 要绑定的整数
    */
    void setInt(uint32_t parameterIndex, int32_t value);

    /** 绑定32位无符号整数参数，下标从0开始
    @param [in] parameterIndex 参数下标
    @param [in] value 要绑定的无符号整数
    */
    void setUInt(uint32_t parameterIndex, uint32_t value);

    /** 绑定64位有符号整数参数，下标从0开始
    @param [in] parameterIndex 参数下标
    @param [in] value 要绑定的整数
    */
    void setInt64(uint32_t parameterIndex, int64_t value);

    /** 绑定64位无符号整数参数，下标从0开始
    @param [in] parameterIndex 参数下标
    @param [in] value 要绑定的无符号整数
    */
    void setUInt64(uint32_t parameterIndex, uint64_t value);

    /** 绑定nullptr参数，下标从0开始
    @param [in] parameterIndex 参数下标
    @param [in] sqlType 底层驱动要求的SQL类型
    */
    void setNull(uint32_t parameterIndex, int32_t sqlType);

    /** 绑定字符串参数，下标从0开始
    @param [in] parameterIndex 参数下标
    @param [in] value 要绑定的字符串
    */
    void setString(uint32_t parameterIndex, const std::string& value);

public:
    /** 在本次同步调用的会话中准备 SQL，失败原因保存在该语句
    @param [in] execution 库内部借用的驱动连接，未建连时生成无效语句
    @param [in] sqlString 保存在语句对象中的 SQL 文本
    */
    MysqlCppPrepareStatement(const MysqlCppExecution& execution, const std::string& sqlString);

    /** 为同一会话的同步执行选择已绑定的语句
    @param [in,out] execution 当前调用的上下文，成功时填入原生语句和当前结果的登记位置
    @return 语句有效且会话匹配时返回 true，失败时清空借用指针
    */
    bool selectForExecution(MysqlCppExecution& execution);

private:
    /** 将业务下标转为驱动下标，执行绑定并记录第一次失败
    @param [in] parameterIndex 从 0 开始的参数下标
    @param [in] bind 同步调用驱动绑定函数，接收已转换的下标
    */
    template<class DriverHelper, class Binder>
    void bindParameter(uint32_t parameterIndex, const Binder& bind);

    /** 禁止拷贝构造，避免多个对象同时释放同一个底层预编译语句
    @param [in] other 另一个预编译语句对象
    */
    MysqlCppPrepareStatement(const MysqlCppPrepareStatement& other);

    /** 禁止赋值，避免多个对象同时释放同一个底层预编译语句
    @param [in] other 另一个预编译语句对象
    @return 不会被调用
    */
    MysqlCppPrepareStatement& operator=(const MysqlCppPrepareStatement& other);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 连接重建后原生语句不再可执行。
    std::weak_ptr<unsigned char> m_session;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 同一驱动语句的执行共享结果缓冲，下一次执行前关闭仍被调用方持有的游标
    std::weak_ptr<MysqlCppResultSet> m_cursor;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 底层MySQL预编译语句对象
    sql::PreparedStatement* m_preparedStatement;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 创建预编译语句时使用的SQL文本
    std::string m_sqlString;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 创建或绑定失败的诊断，供执行入口和直接检查语句的调用者读取
    std::string m_error;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

#include "MysqlCppPrepareStatement.inl"