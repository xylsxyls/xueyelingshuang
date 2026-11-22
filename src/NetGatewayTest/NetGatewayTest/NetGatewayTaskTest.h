#pragma once
#include <string>

// 在独立子进程用真实运输注入宿主轮询终止与异常，不影响其他用例的宿主表。
class NetGatewayTaskTest
{
public:
    /** 创建独立子进程验证任务退出码和收尾
    */
    static void run();

    /** 在首次宿主登记前执行两种协议的五种故障模式
    @param [in] root 当前子进程独占的节点身份目录
    @return 0通过，非0失败
    */
    static int check(const std::string& root);
};