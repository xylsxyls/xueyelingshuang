#pragma once
#include <stdint.h>

/** 分组等价的严格弱序，用于确认整数优化不会绕过自定义比较器
*/
struct LockFreeMapSearchGroupLess
{
public:
    /** 按整数除七且向零截断的商分组，不使用原值相等判定等价
    @param [in] left 左键
    @param [in] right 右键
    @return 左键所在组小于右键所在组时true
    */
    bool operator()(const int64_t& left, const int64_t& right) const;
};

/** 检查不同整数宽度、边界、非二次幂规模及自定义顺序的查询结果
@return 与独立标准有序容器逐项一致时true
*/
bool LockFreeMapSearchTest();