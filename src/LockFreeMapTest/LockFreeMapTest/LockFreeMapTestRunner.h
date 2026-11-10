#pragma once
#include <set>

/** 固定编号对应的中文说明、压力分类和独立判定函数
*/
struct LockFreeMapTestEntry
{
public:
    // 用例中文说明
    const char* m_name;
    // 是否属于压力模式
    bool m_stress;
    // 返回 true 才视为通过
    bool (*m_run)();
};

/** 参数严格校验、结果落盘和资源复核入口
*/
class LockFreeMapTestRunner
{
public:
    /** 校验参数并执行实际存在的用例
    @param [in] argc 参数个数
    @param [in] argv 参数数组
    @return 测试进程退出码
    */
    static int run(int argc, char** argv);

    /** 以外部参数合同检查错误退出码，不运行递归用例
    @return 非法参数返回 2、报告打开失败返回 3 时通过
    */
    static bool arguments();

private:
    /** 读取不溢出的正整数
    @param [in,out] text 输入位置，成功后指向第一个非数字字符
    @param [out] value 正整数结果
    @return 输入合法时返回 true
    */
    static bool number(const char*& text, unsigned& value);

    /** 解析逗号分隔的编号和闭区间
    @param [in] text 完整选择表达式
    @param [out] ids 去重后的编号集合
    @return 所有输入均合法且非空时返回 true
    */
    static bool selection(const char* text, std::set<unsigned>& ids);
};