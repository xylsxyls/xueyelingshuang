#pragma once
#include "BlackBoxTestSupport.h"

/** 返回关闭竞争、动态线程池与持续压力测试。
@param [in] firstId 起始用例编号
@return 稳定递增的用例集合
*/
std::vector<BlackBoxCase> taskThreadStressCases(int32_t firstId);