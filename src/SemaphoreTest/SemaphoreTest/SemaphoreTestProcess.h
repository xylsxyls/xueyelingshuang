#pragma once
#include <stdint.h>
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#if defined(SEMAPHORE_TEST_HOST_LIBRARY) || (defined(__APPLE__) && TARGET_OS_IPHONE) || \
    (defined(__ANDROID__) && __ANDROID_API__ < 28)
// 由移动端宿主逐例启动；这些 SDK 或宿主入口无法通过重新启动自身来运行测试进程。
#define SEMAPHORE_TEST_EXTERNAL_LAUNCH
#endif
#include <string>
#include <vector>

/** 测试专用独立进程执行器，超时只终止本次创建且仍持有身份的子进程
*/
class SemaphoreTestProcess
{
public:
    /** 取得本测试程序的绝对路径
    @return 当前程序路径，无法取得时抛出异常交给主入口记录
    */
    static std::string executable();

    /** 查询已加载 Semaphore 代码所属的文件；静态链接返回程序，动态链接返回库
    @return 模块完整路径；无法确认时抛异常，不能默认为已验证
    */
    static std::string library();

    /** 将本机文件API使用的路径转换为报告中的UTF-8文本
    @param [in] path 运行时使用的文件路径
    @return 可写入报告的UTF-8路径，转换失败抛出异常
    */
    static std::string reportPath(const std::string& path);

    /** 独占创建空报告，防止并发批次或不可读的既有文件被覆盖
    @param [in] path 新文件路径，父目录已存在
    @return 本调用成功创建且关闭了文件才返回true
    */
    static bool reserveReport(const std::string& path);

    /** 执行子进程并将标准输出及错误写入独立文件
    @param [in] executable 程序绝对路径，不含NUL
    @param [in] arguments 各个参数，不能含NUL
    @param [in] log 非空且不含NUL的新日志路径；父目录须存在，不覆盖已有报告
    @param [in] timeout 总预算毫秒
    @param [out] started 可空。由系统确认子进程已开始执行时置真，启动前失败保持假
    @return 程序退出码，124超时，125执行器或清理失败
    */
    static int32_t run(const std::string& executable, const std::vector<std::string>& arguments, const std::string& log,
        uint32_t timeout, bool* started = nullptr);

    /** 逐块计算文件 SHA256
    @param [in] path 输入文件完整路径
    @return 64个十六进制字符；文件打开或读取失败时抛出异常
    */
    static std::string hash(const std::string& path);

    /** 创建一层输出目录，父目录必须已存在
    @param [in] path 本批新目录，已有目录允许但已有文件不得覆盖
    @return 创建成功或目录已存在
    */
    static bool makeDirectory(const std::string& path);

    /** 取得单调时钟
    @return 毫秒
    */
    static double now();
};