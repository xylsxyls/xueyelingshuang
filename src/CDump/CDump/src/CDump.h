#pragma once
#include "CDumpMacro.h"
#include <string>

/** 为宿主进程安装诊断处理器。由宿主统一注册，处理器所属模块须驻留至进程退出
*/
class CDumpAPI CDump
{
public:
    /** 同步安装本机异常或信号处理器，失败后可重试
    @param [in] dumpFileDir 已存在的输出目录，可省略末尾分隔符。空串选择可执行文件目录，路径仅在调用期间借用。Windows 按系统 ANSI 代码页解码此参数
    @return 首次成功安装返回 true。参数无效、后端不可用、安装失败或已成功安装时返回 false
    */
    static bool declareDumpFile(const std::string& dumpFileDir = "");
};