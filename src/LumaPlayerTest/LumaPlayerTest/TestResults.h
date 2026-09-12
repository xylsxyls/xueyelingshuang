#pragma once
#include <QJsonArray>
#include <QString>
#include <stdint.h>

/** 单个隔离子进程的断言结果，每次检查即落盘，不能当作日志包装 */
class TestResults
{
public:
    /** 绑定结果目录
    @param [in] directory 此批子进程独占目录
    */
    explicit TestResults(const QString& directory, int32_t caseId = 0);

    /** 保存实际布尔断言及细节
    @param [in] id T编号
    @param [in] condition 实际比较结果
    @param [in] detail 预期及实际说明
    */
    void check(int32_t id, bool condition, const QString& detail);

    /** 明确记录缺失的子项，不将其他通过覆盖它
    @param [in] id T编号
    @param [in] detail 缺失内容及原因
    */
    void untested(int32_t id, const QString& detail);

    /** 获取失败数
    @return 当前失败断言数
    */
    int32_t failures() const;

private:
    /** 原子保存JSON，失败时也输出stderr
    @param [in] id T编号
    @param [in] status PASS/FAIL/UNTESTED
    @param [in] detail 说明
    */
    void append(int32_t id, const QString& status, const QString& detail);

private:
    // 当前稳定数字ID，0保留调用方传入ID
    int32_t m_caseId;
    // 独占报告目录
    QString m_directory;
    // 已实际执行的检查
    QJsonArray m_checks;
    // 失败数
    int32_t m_failures;
};