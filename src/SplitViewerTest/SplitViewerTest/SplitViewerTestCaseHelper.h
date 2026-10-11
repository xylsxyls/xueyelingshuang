#pragma once
#include "SplitViewerTestCase.h"
#include <map>
#include <set>
#include <vector>

/** 用例目录和所有选择入口共用的严格解析；不保存执行状态
*/
class SplitViewerTestCaseHelper
{
public:
    // 历史101—216保留原编号，新增顺序追加
    static const std::map<SplitViewerTestCaseId, SplitViewerTestCase> kCases;

public:
    /** 检查登记的编号、中文元数据及类型
    @return 登记完整且有效
    */
    static bool validate();

    /** 严格解析例如101,103,207-210的选择，去重并按编号排序
    @param [in] selection 用户输入，空串非法
    @param [out] ids 输出选择，失败保持原值
    @param [out] error 失败原因，成功清空
    @return 输入和全部编号有效
    */
    static bool parse(const std::wstring& selection, std::vector<int32_t>& ids, std::wstring& error);

    /** 核验最后一行唯一汇总与本批选择数量，缺项或取消不算通过
    @param [in] report UTF-8报告字节，支持LF或CRLF末行
    @param [in] expectedCount 本次实际选择的用例数，必须大于零
    @return 唯一汇总的total等于预期且failures为零
    */
    static bool reportSucceeded(const std::string& report, int32_t expectedCount);

    /** 按类型取得有效编号
    @param [in] type 瞬时或压力属性
    @return 已排序编号
    */
    static std::vector<int32_t> byType(SplitViewerTestCaseType type);
};