#include "LockFreeHashMapTest.h"

/** 统一原生及 Wasm 测试入口，错误由测试模块记录并返回非零
@param [in] argc 命令行参数数量
@param [in] argv 命令行参数，省略编号运行全部用例
@return 0 通过，非零表示输入、用例或未预期异常失败
*/
int main(int argc, char** argv)
{
    return LockFreeHashMapRunTests(argc, argv);
}