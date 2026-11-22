#pragma once
#include <string>
#include <utility>
#include <vector>

// 对测试执行器进行黑盒反向验证；只有真实退出码和报告均符合独立预期才通过
class NetRunnerSelfTest
{
public:
    /** 检查三提供方完整门面及原生选择、失败、取消、超时和报告错误
    @param [in] argc 包含程序名、--runner-selfcheck、--repository及--output
    @param [in] argv 原始命令行，Windows通过CSystem取得UTF8
    @return 0全部通过，1用例失败，2参数错误，3报告无法写入
    */
    static int run(int argc, char** argv);

    /** 子进程自检入口，只回显原始参数或等待被父测试取消
    @param [in] argc 完整命令行项数
    @param [in] argv 包含--test-process-probe及探针模式
    @return 0正常完成，2参数错误；等待模式必须由拥有者终止
    */
    static int processProbe(int argc, char** argv);

    /** 按独立取得的用例列表校验全量原生报告，拒绝漏测及空报告
    @param [in] listing --list输出，要求非空且编号严格递增
    @param [in] report 本次--all生成的完整CSV
    @return 全部已登记用例恰好通过一次且汇总正确返回true
    */
    static bool nativeReport(const std::string& listing, const std::string& report);

private:
    /** 去除注释并提取public声明；不依赖固定方法数量
    @param [in] source 完整NetSender头文件
    @param [out] declarations 规范化后的完整声明序列
    @return 成功解析非空声明列表返回true
    */
    static bool declarations(const std::string& source, std::vector<std::string>& declarations);

    /** 比较三提供方的全部公开声明，包含默认参数和const等修饰
    @param [in] repository 源码仓库绝对路径
    @return 全部签名一致且解析器能检测差异返回true
    */
    static bool contracts(const std::string& repository);

    /** 校验完整CSV及总计，拒绝缺行、重复总计、未知状态和伪造通过
    @param [in] text 执行器生成的完整文本
    @param [in] expectedRows 独立预期的ID与状态序列
    @return 行序、状态和各类计数全部正确返回true
    */
    static bool reportMatches(const std::string& text, const std::vector<std::string>& expectedRows);

    /** 用固定正反例检查CSV判定器本身，不调用生产逻辑生成预期
    @return 正例接受且每个损坏样本被拒绝返回true
    */
    static bool reportChecks();

    /** 实际启动探针，检查参数、退出、超时、日志冲突和所有权回收
    @param [in] executable 本测试程序的绝对路径
    @param [in] output 本批次独占目录
    @param [out] results 追加独立自检编号及结果
    */
    static void processChecks(const std::string& executable, const std::string& output,
        std::vector<std::pair<int, bool>>& results);

    /** 用缺失候选和非法选项检查批次器，不启动任何业务网关
    @param [in] executable 当前测试程序
    @param [in] repository 仓库根路径
    @param [in] output 本批次独占目录
    @param [out] results 追加950至955稳定自检结果
    */
    static void batchChecks(const std::string& executable, const std::string& repository,
        const std::string& output, std::vector<std::pair<int, bool>>& results);

    /** 启动正式测试程序并核对退出状态、排序或CSV记录
    @param [in] executable 正式测试EXE绝对路径
    @param [in] output 本批次独占目录
    @param [in] id 保留原测试身份编号
    @param [in] arguments 真实参数，不经shell
    @param [in] expectedCode 独立预期退出码
    @param [in] expectedRows 预期CSV的ID和状态对，空表示不读取CSV
    @param [in] listed 是否要求非空且严格升序的用例列表
    @return 进程按期限结束且各项判定全部匹配返回true
    */
    static bool check(const std::string& executable, const std::string& output, int id,
        const std::vector<std::string>& arguments, int expectedCode,
        const std::vector<std::string>& expectedRows, bool listed = false);
};