#pragma once
#include "CookModels.h"

/** 构建有明确插入顺序的个人步骤流程，保留原图约束。 */
class CookPersonalizationFlow
{
public:
    /** 合并步骤并验证顺序、依赖和禁用后的穿透关系。
    @param [in] base 原始菜谱
    @param [in] config 个人配置，taskOrder须包含全部原始和新增步骤
    @param [out] result 可供排程的菜谱
    @param [out] message 拒绝原因
    @return 合法返回true */
    static bool Build(const Recipe& base, const PersonalizationConfig& config, Recipe& result, std::string& message);
};