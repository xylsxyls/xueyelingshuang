#pragma once
#include "LockFreeMapAPI.h"

/** 在独立模块中重入公开写接口，仅测试工程导出
@param [in,out] instance 外层仍持有借用的容器
@return 明确的资源状态，禁止等待自身退出
*/
#ifdef LOCK_FREE_MAP_TEST_MODULE_LIBRARY
extern "C" __declspec(dllexport) int LockFreeMapNestedWrite(void* instance);
#endif

/** 独立进程入口，加载测试DLL并验证同线程及换线程重入
@param [in] argc 参数数量
@param [in] argv 第二个参数为测试DLL路径
@return 零为通过，非零为准备或合同失败
*/
#ifndef LOCK_FREE_MAP_TEST_MODULE_LIBRARY
int main(int argc, char** argv);
#endif