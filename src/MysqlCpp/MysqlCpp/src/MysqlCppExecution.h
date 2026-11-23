#pragma once
#include "mysql-connector-cpp/cppconn/connection.h"
#include <memory>

class MysqlCppResultSet;

/** 单次同步驱动调用的上下文，连接和语句由原包装对象拥有，不延长会话生命周期
*/
struct MysqlCppExecution
{
    // 本次调用借用的连接，未建连时为空
    sql::Connection* m_connection;
    // 捕获连接代次的弱标识，关闭或重连后失效
    std::weak_ptr<unsigned char> m_session;
    // 通过语句归属检查后才填写，使用期限限于本次执行
    sql::PreparedStatement* m_statement;
    // 选中语句的游标登记处，仅在此次同步执行期间借用
    std::weak_ptr<MysqlCppResultSet>* m_cursor;
};