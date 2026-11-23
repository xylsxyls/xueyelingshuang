#pragma once
#include <mutex>

class MysqlCppConnectionPool;

/** 由连接池及所有借用共同持有，串行化归还与析构
*/
struct MysqlCppPoolLifetime
{
    // 保护池指针，从检查到归还或析构结束始终持锁
    std::mutex m_mutex;
    // 池存在时借用其地址，析构时在锁内清空
    MysqlCppConnectionPool* m_pool;
};