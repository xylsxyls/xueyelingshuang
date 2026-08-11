#pragma once
#include <stdint.h>
#include <istream>
#include <map>
#include <string>
#include <vector>
#include "MysqlCppMacro.h"

namespace sql
{
    class ResultSet;
}

class MysqlCpp;

/** MySQL查询结果集封装，屏蔽底层驱动的异常和列下标规则
*/
class MysqlCppAPI MysqlCppResultSet
{
public:
    /** 析构函数，释放底层结果集对象
    */
    ~MysqlCppResultSet();

public:
    /** 判断结果集是否可用
    @return 返回是否持有有效的底层结果集
    */
    bool isValid() const;

    /** 将剩余结果行按列顺序转换为二维数组
    @return 返回从当前位置继续读取到末尾的所有行
    */
    std::vector<std::vector<std::string>> toVector();

    /** 将剩余结果行按列名转换为字典数组
    @return 返回从当前位置继续读取到末尾的所有行
    */
    std::vector<std::map<std::string, std::string>> toMapVector();

public:
    /** 按列下标读取二进制流，列下标从0开始
    @param [in] columnIndex 列下标
    @return 返回底层二进制输入流，失败返回nullptr
    */
    std::istream* getBlob(uint32_t columnIndex) const;

    /** 按列名读取二进制流
    @param [in] columnLabel 列名或别名
    @return 返回底层二进制输入流，失败返回nullptr
    */
    std::istream* getBlob(const std::string& columnLabel) const;

    /** 按列下标读取布尔值，列下标从0开始
    @param [in] columnIndex 列下标
    @return 返回读取到的布尔值，失败返回false
    */
    bool getBoolean(uint32_t columnIndex) const;

    /** 按列名读取布尔值
    @param [in] columnLabel 列名或别名
    @return 返回读取到的布尔值，失败返回false
    */
    bool getBoolean(const std::string& columnLabel) const;

    /** 按列下标读取浮点数，列下标从0开始
    @param [in] columnIndex 列下标
    @return 返回读取到的浮点数，失败返回0
    */
    long double getDouble(uint32_t columnIndex) const;

    /** 按列名读取浮点数
    @param [in] columnLabel 列名或别名
    @return 返回读取到的浮点数，失败返回0
    */
    long double getDouble(const std::string& columnLabel) const;

    /** 按列下标读取32位有符号整数，列下标从0开始
    @param [in] columnIndex 列下标
    @return 返回读取到的整数，失败返回0
    */
    int32_t getInt(uint32_t columnIndex) const;

    /** 按列名读取32位有符号整数
    @param [in] columnLabel 列名或别名
    @return 返回读取到的整数，失败返回0
    */
    int32_t getInt(const std::string& columnLabel) const;

    /** 按列下标读取32位无符号整数，列下标从0开始
    @param [in] columnIndex 列下标
    @return 返回读取到的无符号整数，失败返回0
    */
    uint32_t getUInt(uint32_t columnIndex) const;

    /** 按列名读取32位无符号整数
    @param [in] columnLabel 列名或别名
    @return 返回读取到的无符号整数，失败返回0
    */
    uint32_t getUInt(const std::string& columnLabel) const;

    /** 按列下标读取64位有符号整数，列下标从0开始
    @param [in] columnIndex 列下标
    @return 返回读取到的整数，失败返回0
    */
    int64_t getInt64(uint32_t columnIndex) const;

    /** 按列名读取64位有符号整数
    @param [in] columnLabel 列名或别名
    @return 返回读取到的整数，失败返回0
    */
    int64_t getInt64(const std::string& columnLabel) const;

    /** 按列下标读取64位无符号整数，列下标从0开始
    @param [in] columnIndex 列下标
    @return 返回读取到的无符号整数，失败返回0
    */
    uint64_t getUInt64(uint32_t columnIndex) const;

    /** 按列名读取64位无符号整数
    @param [in] columnLabel 列名或别名
    @return 返回读取到的无符号整数，失败返回0
    */
    uint64_t getUInt64(const std::string& columnLabel) const;

    /** 按列下标读取字符串，列下标从0开始
    @param [in] columnIndex 列下标
    @return 返回读取到的字符串，失败返回空字符串
    */
    std::string getString(uint32_t columnIndex) const;

    /** 按列名读取字符串
    @param [in] columnLabel 列名或别名
    @return 返回读取到的字符串，失败返回空字符串
    */
    std::string getString(const std::string& columnLabel) const;

    /** 移动到下一行
    @return 返回是否存在下一行
    */
    bool next();

    /** 移动到上一行
    @return 返回是否存在上一行
    */
    bool previous();

    /** 获取结果集行数
    @return 返回结果集行数，失败返回0
    */
    size_t rowsCount();

    /** 获取结果集列数
    @return 返回列数，失败返回0
    */
    int32_t columnCount();

    /** 获取结果集列名列表
    @return 返回列名或别名列表，失败返回空列表
    */
    std::vector<std::string> columnNames() const;

private:
    friend class MysqlCpp;

    /** 构造函数，只允许MysqlCpp创建
    @param [in] resultSet 底层结果集指针，析构时由本对象释放
    */
    explicit MysqlCppResultSet(sql::ResultSet* resultSet);

    /** 禁止拷贝构造，避免多个对象同时释放同一个底层结果集
    @param [in] other 另一个结果集对象
    */
    MysqlCppResultSet(const MysqlCppResultSet& other);

    /** 禁止赋值，避免多个对象同时释放同一个底层结果集
    @param [in] other 另一个结果集对象
    @return 不会被调用
    */
    MysqlCppResultSet& operator=(const MysqlCppResultSet& other);

private:
    // 底层MySQL查询结果集对象
    sql::ResultSet* m_resultSet;
};