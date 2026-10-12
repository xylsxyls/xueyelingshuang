#pragma once
#include <string>

/** Cook资源路径与升级策略；通用文件和路径机制由CSystem实现 */
class CookResourcePaths
{
  public:
    /** 启动前验证独立根目录，迁移旧资源，失败不启动HTTP
    @param [out] message 配置、冲突或迁移失败原因
    @return 配置和迁移完成返回true
    */
    static bool prepare(std::string &message);

    /** 将可信相对子目录或媒体文件引用解析到配置资源根
    @param [in] relative 相对路径，不允许绝对、零字符或父目录穿越
    @return 完整资源路径，非法返回空
    */
    static std::string path(const std::string &relative);

    /** 兼容旧数据库相对地址与旧EXE/data或EXE/res绝对地址
    @param [in] stored 数据库媒体地址
    @return 资源根内实际媒体路径，未知外部路径返回空
    */
    static std::string mediaPath(const std::string &stored);

  private:
    /** 验证相对路径及取得首级目录
    @param [in] relative 输入路径
    @param [out] first 第一层目录
    @return 非空合法相对路径返回true
    */
    static bool relativeRoot(const std::string &relative, std::string &first);

    /** 验证资源相对路径并去掉旧res首级目录
    @param [in] relative 新旧资源相对路径
    @return 统一分隔符后的路径，非法或仅res目录返回空
    */
    static std::string flatRelative(const std::string &relative);
};