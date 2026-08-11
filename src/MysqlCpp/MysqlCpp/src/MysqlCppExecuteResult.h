#pragma once
#include "MysqlCppMacro.h"
#include <stdint.h>
#include <string>

/** 非查询SQL的执行结果，用于返回执行状态、影响行数、自增ID和错误信息
*/
struct MysqlCppAPI MysqlCppExecuteResult
{
    /** 构造函数，会初始化为失败、影响行数0、自增ID0
    */
    MysqlCppExecuteResult();

    // SQL是否执行成功
    bool m_success;
    // INSERT/UPDATE/DELETE影响的行数，执行失败时为0
    int32_t m_affectedRows;
    // 本连接最后一次INSERT产生的自增ID，没有自增ID时为0
    uint64_t m_lastInsertId;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 执行失败时的错误说明，成功时为空
    std::string m_errorMessage;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};