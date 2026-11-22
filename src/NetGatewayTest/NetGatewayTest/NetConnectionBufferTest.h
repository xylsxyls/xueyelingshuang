#pragma once

// 通过实际TLS检查连接帧边界，确定性随机源只用于本测试进程
class NetConnectionBufferTest
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