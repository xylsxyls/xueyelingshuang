#pragma once
#ifdef _WIN32
#include <string>

/** 为 Windows 宿主预备输出路径和 DbgHelp 并安装异常过滤器，由注册入口串行调用
@param [in] directory 已存在的本机代码页目录，空串选择可执行文件目录
@return 安装成功返回 true
*/
bool InstallWindowsDump(const std::string& directory);
#endif