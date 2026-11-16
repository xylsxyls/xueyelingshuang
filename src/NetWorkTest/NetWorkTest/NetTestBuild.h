#pragma once
#include <string>
#include <vector>

// 只构建测试工程的Wasm程序，消费已准备的SDK；不编译、改写或发布任何生产库
class NetTestBuild
{
public:
    /** 用官方Emscripten编译测试程序，所有目标及日志放入新建目录
    @param [in] argc --build-tests及target、bits、sdk、headers、emscripten、python、repository、output参数
    @param [in] argv 原始参数，Windows由CSystem读取UTF8
    @return 0全部构建成功，1编译或证据失败，2参数或输入错误
    */
    static int run(int argc, char** argv);

private:
    /** 执行一个有期限的编译步骤并等待整个自有进程树退出
    @param [in] executable 官方编译器所用解释器绝对路径
    @param [in] arguments 独立命令行参数，不经过shell
    @param [in] log 不存在的日志路径
    @return 正常零码退出且无遗留后代返回true
    */
    static bool compile(const std::string& executable, const std::vector<std::string>& arguments, const std::string& log);

    /** 从测试专用公开声明生成Memory64指针参数转换表，不改变uint64编号
    @param [in] header 测试专用绑定头文件
    @param [out] result 成功写入合法JSON数组；失败不修改
    @return 声明匹配且读取成功返回true
    */
    static bool signatures(const std::string& header, std::string& result);
};