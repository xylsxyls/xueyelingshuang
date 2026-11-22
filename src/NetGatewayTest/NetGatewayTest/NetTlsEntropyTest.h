#pragma once

// 独立保留从生产文件迁出的边界断言，失败由统一测试入口记录。
class NetTlsEntropyTest
{
public:
    /** 运行功能及资源边界测试
    */
    static void run();

private:
    /** 执行原始用例中的独立预期
    @return 0通过，非0失败
    */
    static int check();
};