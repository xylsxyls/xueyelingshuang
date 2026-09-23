#pragma once

#include <QString>

/** 复用产品窗口与 Core C API 的回归入口；素材仅写入报告批次目录 */
class PdfReaderRegression
{
public:
    /** 执行数字选择（1、1,3、1-12），每项独立记录断言和耗时
    @param [in] selection 用例 ID 选择；空字符串表示全部瞬时
    @param [in] reportRoot 报告目录，空时使用测试程序旁 reports
    @return 全部选中项通过返回 0，失败或无效选择返回非零
    */
    static int run(const QString& selection = QString(), const QString& reportRoot = QString());
};