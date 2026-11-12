#pragma once
#include "BlackBoxTestSupport.h"
#include <map>

/** 测试工具的一个性能场景，参数与基准入口一致 */
struct ProcessWorkTestWorkload
{
    // 稳定名称
    std::string m_name;
    // 生产进程数
    int32_t m_processes;
    // 每进程生产线程数
    int32_t m_threads;
    // 每线程消息数
    int32_t m_count;
    // 正文字节数
    int32_t m_bytes;
    // 初始共享块数
    int32_t m_blocks;
    // 0 PID、1名称、2 Post、3直接Channel诊断
    int32_t m_mode;
};

/** Test专用原生调度、私有构建及结果汇总；不链接进生产库 */
class ProcessWorkTestAutomation
{
public:
    /** 执行一个测试工具命令，错误由测试main捕获并返回非零
    @param [in] argc 从工具命令开始的参数数量
    @param [in] argv 命令及参数，借用至返回
    @return 0成功，其他值表示失败
    */
    static int32_t run(int32_t argc, char** argv);

    /** 验证严格数值解析、汇总、场景清单和进程退出判定 */
    static void selfTest();

    /** 准备隔离源码并使用VS2013构建，不发布或替换公共SDK
    @param [in] root 仓库根目录
    @param [in] output 尚不存在、父目录已存在的批次目录
    @param [in] arch x64或x86
    @param [in] mode tests、dll、benchmark、memory、profile、demo或map
    */
    static void build(const std::string& root, const std::string& output,
        const std::string& arch, const std::string& mode);

    /** 私有构建子进程中的cmd启动适配；沿用父进程的作业对象和日志
    @param [in] path 本工具生成的build.cmd路径，不接受命令文本
    @return 构建退出码
    */
    static int32_t shell(const std::string& path);

    /** 向刚创建的私有构建副本插入诊断，不修改仓库生产源码
    @param [in] output 私有构建目录
    @param [in] mode memory或profile
    @param [in] benchmark 私有基准源码完整路径
    */
    static void instrument(const std::string& output, const std::string& mode, const std::string& benchmark);

    /** 创建独立目录，拒绝覆盖已有输出
    @param [in] path 父目录已存在的路径
    */
    static void directory(const std::string& path);

    /** 执行明确路径的程序，超时仅清理本工具创建的进程组
    @param [in] executable 程序路径
    @param [in] arguments 独立参数
    @param [in] log 输出文件
    @param [in] seconds 超时秒数
    */
    static void execute(const std::string& executable, const std::vector<std::string>& arguments,
        const std::string& log, int32_t seconds);

    /** 记录文件身份，FNV-1a用于可重复性核对而非安全签名
    @param [in] path 完整文件路径
    @return 哈希与字节数
    */
    static std::string fingerprint(const std::string& path);

    /** 转换文本为LF并去掉BOM，供唯一源码锚点检查
    @param [in] text 完整文件内容
    @return 规范化内容
    */
    static std::string plain(std::string text);

    /** 保存私有C++副本，UTF8 BOM、CRLF且无尾空白
    @param [in] path 目标文件
    @param [in] text LF文本
    */
    static void writeCode(const std::string& path, const std::string& text);

    /** 检查并替换唯一锚点，不匹配或重复时拒绝构建
    @param [in,out] text 完整源码
    @param [in] before 原始上下文
    @param [in] after 替换内容
    */
    static void replaceOnce(std::string& text, const std::string& before, const std::string& after);

private:
    /** 保留全部23项标准场景
    @return 场景列表
    */
    static std::vector<ProcessWorkTestWorkload> workloads();

    /** 用两个明确程序路径运行五组跨位数真实互发
    @param [in] output 新建批次目录
    @param [in] first 第一种架构程序
    @param [in] second 第二种架构程序
    */
    static void cross(const std::string& output, const std::string& first, const std::string& second);

    /** 运行交错性能矩阵；所有样本即时落盘，失败组不输出有效比较
    @param [in] argc 从matrix命令开始的参数数量
    @param [in] argv 命令参数
    @return 0全部完成，1存在失败
    */
    static int32_t matrix(int32_t argc, char** argv);

    /** 解析基准输出末行的扁平数字/布尔JSON，拒绝重复键和截断
    @param [in] text 原始日志
    @return 字段表
    */
    static std::map<std::string, double> parse(const std::string& text);

    /** 求有限样本中位数
    @param [in] values 非空样本
    @return 中位数
    */
    static double median(std::vector<double> values);
};