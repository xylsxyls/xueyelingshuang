#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// 批次中的一个独立子套件，保留原场景名称和明确的未运行原因
struct NetTestBatchCase
{
    // 稳定套件名称，--only按此选择
    std::string m_name;
    // 实际可执行文件，不经shell
    std::string m_executable;
    // 原始独立参数
    std::vector<std::string> m_arguments;
    // 完整套件最大等待毫秒
    uint32_t m_deadlineMs;
    // 非空时记录NOT_RUN，不能执行后冒充通过
    std::string m_missing;

    /** 创建确定的套件元数据
    @param [in] name 唯一名称
    @param [in] executable 实际程序路径
    @param [in] arguments 原始参数
    @param [in] deadlineMs 完整期限
    */
    NetTestBatchCase(const std::string& name, const std::string& executable,
        const std::vector<std::string>& arguments, uint32_t deadlineMs);
};

// 跨语言测试只由C++顺序调度；JavaScript仍由Node执行自身的真实断言
class NetTestBatch
{
public:
    /** 运行所选独立套件，输出完整/未运行/失败分别记录的JSON
    @param [in] argc --batch及--bin、--module、--node、--repository、--output和可选--only、--protocol-module、--modules-bin、--wire-baseline等
    @param [in] argv 原始参数，Windows通过CSystem读取UTF8
    @return 0全部通过，1失败或缺前提，2参数错误，3证据写入失败
    */
    static int run(int argc, char** argv);

private:
    /** 建立全目录，缺前提仍保留场景，不能靠删除用例使全量通过
    @param [in] options 已校验的独立参数
    @param [out] cases 完整套件目录
    */
    static void catalog(const std::map<std::string, std::string>& options, std::vector<NetTestBatchCase>& cases);

    /** 输出合法JSON字符串，保留UTF8正文并转义引号、反斜杠和控制字节
    @param [in] value 任意UTF8值
    @return 带引号的JSON字符串
    */
    static std::string quote(const std::string& value);

    /** 覆盖本批次自有报告，并核对完整写入及关闭
    @param [in] filename 新批次目录下文件路径
    @param [in] value 完整内容
    @return 成功写入及关闭返回true
    */
    static bool write(const std::string& filename, const std::string& value);
};