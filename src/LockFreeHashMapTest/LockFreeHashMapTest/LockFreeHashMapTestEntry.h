#pragma once

#if defined(_WIN32) && defined(LOCKFREEHASHMAP_TEST_DLL)
#if defined(LOCKFREEHASHMAP_TEST_EXPORT)
#define LOCKFREEHASHMAP_TEST_API __declspec(dllexport)
#else
#define LOCKFREEHASHMAP_TEST_API __declspec(dllimport)
#endif
#else
#define LOCKFREEHASHMAP_TEST_API
#endif

/** 测试模块入口，产品模板库不导出此符号
@param [in] argc 命令行参数数量
@param [in] argv 命令行数组，省略编号运行全部用例
@return 0 全部通过，非零表示输入、用例或设施失败
*/
extern "C" LOCKFREEHASHMAP_TEST_API int LockFreeHashMapRunTests(int argc, char** argv);