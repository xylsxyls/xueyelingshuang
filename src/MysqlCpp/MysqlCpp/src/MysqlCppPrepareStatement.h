#pragma once
#include <stdint.h>
#include <istream>
#include <string>
#include "MysqlCppMacro.h"

namespace sql
{
    class PreparedStatement;
}

class MysqlCpp;

/** MySQL预编译语句封装，用于绑定参数后交给MysqlCpp执行
*/
class MysqlCppAPI MysqlCppPrepareStatement
{
public:
    /** 析构函数，释放底层预编译语句对象
    */
    ~MysqlCppPrepareStatement();

public:
    /** 判断预编译语句是否可用
    @return 返回是否持有有效的底层预编译语句
    */
    bool isValid() const;

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
    @param [in] blob 要绑定的二进制输入流
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

private:
    friend class MysqlCpp;

    /** 构造函数，只允许MysqlCpp创建
    @param [in] prepareStatement 底层预编译语句指针，析构时由本对象释放
    @param [in] sqlString 预编译语句对应的SQL文本
    */
    MysqlCppPrepareStatement(sql::PreparedStatement* prepareStatement, const std::string& sqlString);

    /** 获取底层预编译语句对象
    @return 返回底层预编译语句指针，可能为空
    */
    sql::PreparedStatement* preparedStatement() const;

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
};