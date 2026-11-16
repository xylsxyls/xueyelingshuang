#pragma once

// 只校验遗留明文接口的内存边界，不把它作为安全网关入口
class NetLegacyBoundaryTest
{
public:
    /** 运行独立用例并将失败交给统一测试入口
    */
    static void run();

private:
    /** 核对生产接口的边界和资源收尾
    @return 0通过，非0断言失败
    */
    static int check();
};