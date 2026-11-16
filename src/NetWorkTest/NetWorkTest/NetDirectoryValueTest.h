#pragma once

// 从生产库迁出的独立边界用例，保留全部原有断言。
class NetDirectoryValueTest
{
public:
    /** 运行用例并向统一runner报告失败
    */
    static void run();

private:
    /** 核对实际接口与独立预期
    @return 0通过，非0断言失败
    */
    static int check();
};