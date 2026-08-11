#pragma once
#include "MysqlCppMacro.h"

class MysqlCpp;

/** MySQL事务作用域封装，析构时会自动回滚未提交事务
*/
class MysqlCppAPI MysqlCppTransaction
{
public:
    /** 构造函数，创建后立即尝试开启事务
    @param [in] mysql 事务绑定的MySQL连接对象
    */
    explicit MysqlCppTransaction(MysqlCpp& mysql);

    /** 析构函数，事务仍处于活动状态时自动回滚
    */
    ~MysqlCppTransaction();

public:
    /** 判断事务是否仍处于活动状态
    @return 返回事务是否已成功开启且尚未提交或回滚
    */
    bool isActive() const;

    /** 提交事务
    @return 返回是否提交成功
    */
    bool commit();

    /** 回滚事务，回滚后事务变为非活动状态
    */
    void rollback();

private:
    /** 拷贝构造函数禁用，事务对象不能被复制
    @param [in] other 另一个事务对象
    */
    MysqlCppTransaction(const MysqlCppTransaction& other);

    /** 赋值函数禁用，事务对象不能被复制
    @param [in] other 另一个事务对象
    @return 不会被调用
    */
    MysqlCppTransaction& operator=(const MysqlCppTransaction& other);

private:
    // 事务绑定的MySQL连接对象，不负责释放
    MysqlCpp* m_mysql;
    // 事务是否已开启且尚未提交或回滚
    bool m_active;
};