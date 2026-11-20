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
@return 0 已执行用例通过，1 用例失败，2 参数错误，3 未预期异常，77 表示单线程宿主跳过指定并发用例
*/
extern "C" LOCKFREEHASHMAP_TEST_API int LockFreeHashMapRunTests(int argc, char** argv);