#pragma once

// 专供跨进程测试的受控业务服务入口；测试程序与网关生产进程分开运行。
class NetGatewayProcessProbe
{
public:
    /** 执行服务、内部服务、取消及容量测试子进程
    @param [in] argc 包含程序名的参数数量
    @param [in] argv 平台参数，Windows从系统Unicode命令行读取
    @return 0通过；2参数不合法；其他非零值为探针失败
    */
    static int run(int argc, char** argv);
};