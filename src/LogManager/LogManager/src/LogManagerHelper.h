#pragma once
#include <stdint.h>
#include <string>

/** LogManager内部辅助函数集合，封装日志路径、滚动文件命名、软链接和文件删除保护等细节
*/
class LogManagerHelper
{
public:
    /** 标准化日志目录
    @param [in] path 配置传入的日志目录，空串使用程序目录，Wasm 使用当前目录
    @return 返回带路径分隔符的日志目录
    */
    static std::string normalizeLogDir(const std::string& path);

    /** 递归创建目录
    @param [in] path 要创建的目录
    @return 返回目录是否存在或创建成功
    */
    static bool createDirectoryRecursive(const std::string& path);

    /** 生成当前时间命名片段
    @return 返回YYYYMMDD_HHMMSS格式时间
    */
    static std::string currentTimeName();

    /** 生成日志基础名
    @param [in] processName 进程名，不带后缀
    @param [in] pid 进程ID
    @param [in] timeName 当前时间命名片段
    @return 返回进程名_pid_YYYYMMDD_HHMMSS
    */
    static std::string buildBaseName(const std::string& processName, int32_t pid, const std::string& timeName);

    /** 生成软链接路径
    @param [in] logDir 日志目录
    @param [in] baseName 日志基础名
    @return 返回软链接路径
    */
    static std::string buildLinkPath(const std::string& logDir, const std::string& baseName);

    /** 生成实体日志路径
    @param [in] logDir 日志目录
    @param [in] baseName 日志基础名
    @param [in] fileIndex 实体文件下标
    @return 返回实体日志路径
    */
    static std::string buildEntityPath(const std::string& logDir, const std::string& baseName, int32_t fileIndex);

    /** 判断路径是否存在
    @param [in] path 文件或目录路径
    @return 返回路径是否存在
    */
    static bool exist(const std::string& path);

    /** 删除文件或软链接
    @param [in] path 文件路径
    @return 路径为空、删除成功或确认不存在时返回 true；权限和设备错误返回 false
    */
    static bool deleteFile(const std::string& path);

    /** 从路径中取文件名
    @param [in] path 文件路径
    @return 返回文件名
    */
    static std::string fileName(const std::string& path);

    /** 创建新软链接，已有目标名保持原样
    @param [in] linkPath 软链接路径
    @param [in] targetPath 目标实体日志路径
    @return 返回软链接是否创建成功
    */
    static bool createSymbolicLinkFile(const std::string& linkPath, const std::string& targetPath);

    /** 在 Windows 打开链接本身并禁止外部删除
    @param [in] path 文件或软链接路径
    @return Windows 返回保护句柄，失败或其他平台返回 nullptr
    */
    static void* lockLinkDelete(const std::string& path);

    /** 关闭删除保护句柄
    @param [in,out] lockHandle lockLinkDelete 返回的句柄
    */
    static void closeFileLock(void*& lockHandle);

    /** 规格化配置中的单文件最大字节数
    @param [in] maxFileBytes 配置值
    @return 返回最终使用的单文件最大字节数
    */
    static int64_t normalizeMaxFileBytes(int64_t maxFileBytes);

    /** 规格化配置中的最大保留文件数
    @param [in] maxFileCount 配置值
    @return 返回最终使用的最大保留文件数
    */
    static int32_t normalizeMaxFileCount(int32_t maxFileCount);

    /** 规格化配置中的文件大小检查间隔
    @param [in] checkFileSizeInterval 配置值
    @return 返回最终使用的检查间隔
    */
    static int32_t normalizeCheckFileSizeInterval(int32_t checkFileSizeInterval);

private:
#ifdef _WIN32
    /** 尝试启用Windows创建软链接需要的权限
    */
    static void enableSymbolicLinkPrivilege();
#endif
};