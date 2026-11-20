#include "LockFreeHashMapTest.h"

/** 统一原生及 Wasm 测试入口，错误由测试模块记录并返回非零
@param [in] argc 命令行参数数量
@param [in] argv 命令行参数，省略编号运行全部用例
@return 0 通过，1 用例失败，2 参数错误，3 未预期异常，77 单线程宿主跳过指定并发用例
*/
int main(int argc, char** argv)
{
    return LockFreeHashMapRunTests(argc, argv);
}