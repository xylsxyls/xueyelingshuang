#pragma once
#include <QtCore/QString>

/** GUI和命令行共用的逐项批次入口，取消在用例完成收尾后生效
*/
class SplitViewerTestBatchHelper
{
public:
    /** 严格解析并在独立报告子目录执行每个场景
    @param [in] selection 数字及范围表达式
    @param [in] directory 本批次可写报告目录
    @return 全部通过为0，失败为1，选择错误为2，取消为3
    */
    static int run(const QString& selection, const QString& directory);
};