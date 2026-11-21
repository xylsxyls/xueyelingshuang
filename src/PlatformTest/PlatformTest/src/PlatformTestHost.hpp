#pragma once
#include <string>
#include <vector>

// 每次宿主测试独占临时目录。子进程参数按原值传输，超时由 CMake 的 execute_process 回收。
class PlatformTestHost
{
public:
    /** 创建测试用临时目录，创建失败时抛出异常
    */
    PlatformTestHost();

    /** 删除本次运行创建的临时文件及目录
    */
    ~PlatformTestHost();

    /** 执行宿主命令并读取标准输出及错误输出
    @param [in] args 程序名及各参数，每个元素独立传递
    @param [in] timeoutSeconds 超时秒数
    @param [out] output 程序输出
    @param [in] required 非零退出码是否抛出异常
    @return 子进程退出状态；无法启动或超时返回 1
    */
    int command(const std::vector<std::string>& args, int timeoutSeconds, std::string& output, bool required = true) const;

    /** 获得本次独占的目录路径
    @return 宿主对象期间有效的绝对路径
    */
    const std::string& directory() const;

    /** 获得本次设备目录及状态标记共用的唯一标识
    @return 小写 UUID 字符串
    */
    const std::string& token() const;

    /** 读取本地文件
    @param [in] path 文件路径
    @return 读取的字节，文件不存在时返回空串
    */
    static std::string read(const std::string& path);

    /** 在指定路径创建或覆盖文件，失败时抛出异常
    @param [in] path 目标路径
    @param [in] data 文件内容
    */
    static void write(const std::string& path, const std::string& data);

    /** 列出指定目录下的普通文件名
    @param [in] directory 待访问的目录
    @return 文件名集合，目录不存在时返回空集合
    */
    static std::vector<std::string> files(const std::string& directory);

    /** 判断文件或目录是否存在
    @param [in] path 本机目标路径
    @return 成功找到时返回 true
    */
    static bool exists(const std::string& path);

private:
    PlatformTestHost(const PlatformTestHost& other);
    PlatformTestHost& operator=(const PlatformTestHost& other);

private:
    // 本次进程独占的文件目录
    std::string m_directory;
    // Android 目录和 iOS 沙箱结果共用的小写 UUID
    std::string m_token;
};