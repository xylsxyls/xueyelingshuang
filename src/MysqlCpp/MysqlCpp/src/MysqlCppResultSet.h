#pragma once
#include "MysqlCppMacro.h"
#include "mysql-connector-cpp/cppconn/resultset.h"
#include <memory>
#include <stdint.h>
#include <istream>
#include <map>
#include <string>
#include <vector>

struct MysqlCppExecution;
class MysqlCppPrepareStatement;

/** MySQL查询结果集封装，屏蔽底层驱动的异常和列下标规则
*/
class MysqlCppAPI MysqlCppResultSet
{
public:
    /** 析构函数，释放底层结果集对象
    */
    ~MysqlCppResultSet();

public:
    /** 提前释放结果和来源语句的借用，消耗过程的后续响应，重复关闭安全
    @return 正常释放或已经关闭返回 true，驱动报告后续响应错误时返回 false
    */
    bool close();

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
    @return 成功时调用方拥有返回的新流并负责释放；读取失败返回nullptr
    */
    std::istream* getBlob(uint32_t columnIndex) const;

    /** 按列名读取二进制流
    @param [in] columnLabel 不含 NUL 的完整列名或别名
    @return 成功时调用方拥有返回的新流并负责释放；读取失败返回nullptr
    */
    std::istream* getBlob(const std::string& columnLabel) const;

    /** 按列下标读取布尔值，列下标从0开始
    @param [in] columnIndex 列下标
    @return 返回读取到的布尔值，失败返回false
    */
    bool getBoolean(uint32_t columnIndex) const;

    /** 按列名读取布尔值
    @param [in] columnLabel 不含 NUL 的完整列名或别名
    @return 返回读取到的布尔值，失败返回false
    */
    bool getBoolean(const std::string& columnLabel) const;

    /** 按列下标读取浮点数，列下标从0开始
    @param [in] columnIndex 列下标
    @return 返回读取到的浮点数，失败返回0
    */
    long double getDouble(uint32_t columnIndex) const;

    /** 按列名读取浮点数
    @param [in] columnLabel 不含 NUL 的完整列名或别名
    @return 返回读取到的浮点数，失败返回0
    */
    long double getDouble(const std::string& columnLabel) const;

    /** 按列下标读取32位有符号整数，列下标从0开始
    @param [in] columnIndex 列下标
    @return 返回读取到的整数，失败返回0
    */
    int32_t getInt(uint32_t columnIndex) const;

    /** 按列名读取32位有符号整数
    @param [in] columnLabel 不含 NUL 的完整列名或别名
    @return 返回读取到的整数，失败返回0
    */
    int32_t getInt(const std::string& columnLabel) const;

    /** 按列下标读取32位无符号整数，列下标从0开始
    @param [in] columnIndex 列下标
    @return 返回读取到的无符号整数，失败返回0
    */
    uint32_t getUInt(uint32_t columnIndex) const;

    /** 按列名读取32位无符号整数
    @param [in] columnLabel 不含 NUL 的完整列名或别名
    @return 返回读取到的无符号整数，失败返回0
    */
    uint32_t getUInt(const std::string& columnLabel) const;

    /** 按列下标读取64位有符号整数，列下标从0开始
    @param [in] columnIndex 列下标
    @return 返回读取到的整数，失败返回0
    */
    int64_t getInt64(uint32_t columnIndex) const;

    /** 按列名读取64位有符号整数
    @param [in] columnLabel 不含 NUL 的完整列名或别名
    @return 返回读取到的整数，失败返回0
    */
    int64_t getInt64(const std::string& columnLabel) const;

    /** 按列下标读取64位无符号整数，列下标从0开始
    @param [in] columnIndex 列下标
    @return 返回读取到的无符号整数，失败返回0
    */
    uint64_t getUInt64(uint32_t columnIndex) const;

    /** 按列名读取64位无符号整数
    @param [in] columnLabel 不含 NUL 的完整列名或别名
    @return 返回读取到的无符号整数，失败返回0
    */
    uint64_t getUInt64(const std::string& columnLabel) const;

    /** 按列下标读取字符串，列下标从0开始
    @param [in] columnIndex 列下标
    @return 返回读取到的字符串，失败返回空字符串
    */
    std::string getString(uint32_t columnIndex) const;

    /** 按列名读取字符串
    @param [in] columnLabel 不含 NUL 的完整列名或别名
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

public:
    /** 执行已校验的语句并接管驱动返回的结果，驱动异常交给连接入口处理
    @param [in] execution 库内部同步执行上下文
    @param [in] statement 保持结果来源语句存活，空指针表示构造无效结果
    */
    MysqlCppResultSet(const MysqlCppExecution& execution, const std::shared_ptr<MysqlCppPrepareStatement>& statement);

private:
    /** 在当前结果中解析完整的列名，供按名读取复用相应的下标接口
    @param [in] label 不含 NUL 的列名或别名，大小写匹配使用驱动规则
    @return 找到时返回从 0 开始的下标，结果无效或找不到时返回 UINT32_MAX
    */
    uint32_t findColumnIndex(const std::string& label) const;

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
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 保持建结果时的预处理语句存活，检查所属会话是否仍有效。
    std::shared_ptr<MysqlCppPrepareStatement> m_statement;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 动态结果所属的连接代次，绑定失败时仍需消费同一连接上的响应
    std::weak_ptr<unsigned char> m_pendingSession;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 底层MySQL查询结果集对象
    sql::ResultSet* m_resultSet;
    // 仅动态返回列时借用来源语句，释放首组行后用它消费过程的剩余响应
    sql::Statement* m_pendingStatement;
};