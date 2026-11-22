#pragma once

// 通过受控文件后端核对缓存的背压、畸形完成和停机回收
class NetFileFacadeTest
{
public:
    /** 运行独立用例并将失败交给统一测试入口
    */
    static void run();

public:
    /** 核对生产接口的边界和资源收尾
    @return 0通过，非0断言失败
    */
    static int check();
};