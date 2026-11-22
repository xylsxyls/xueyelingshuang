#pragma once

// 独立保留从生产文件迁出的边界断言，失败由统一测试入口记录。
class NetInterfaceCombinedTest
{
public:
    /** 运行功能及资源边界测试
    */
    static void run();

    /** 编码并解析50万条完整正文，记录模块耗时
    */
    static void benchmark();

private:
    /** 执行原始用例中的独立预期
    @param [in] argc 0为功能，2为模块基准
    @param [in] argv 仅基准模式借用两项参数
    @return 0通过，非0失败
    */
    static int check(int argc, const char* argv[]);
};