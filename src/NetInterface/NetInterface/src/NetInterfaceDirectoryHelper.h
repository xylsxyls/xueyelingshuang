#pragma once
#include <cstdint>

/** 同模块跨类使用的局部辅助，不保存可变业务状态
*/
class NetInterfaceDirectoryHelper
{
public:
    /** 检查调用者缓冲别名，减法比较避免地址加长度溢出。
    @param [in] first 第一段起点
    @param [in] firstSize 第一段长度
    @param [in] second 第二段起点
    @param [in] secondSize 第二段长度
    @return 两段非空且相交
    */
    static bool directoryRangesOverlap(const void* first, uint32_t firstSize, const void* second, uint32_t secondSize);
};