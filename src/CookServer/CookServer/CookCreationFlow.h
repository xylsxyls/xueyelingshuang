#pragma once
#include "CookModels.h"
#include <map>
#include <set>

/** 作者完整步骤编辑与已有个性化的兼容处理，不改变个人编辑页面契约 */
class CookCreationFlow
{
public:
    /** 校验并构建作者提交的步骤列表
    @param [in] base 当前原始菜谱
    @param [in] input 完整有序步骤
    @param [in] replaceStructure 是否允许新增、删除、排序及性质变更
    @param [out] result 新菜谱，其他字段来自base
    @param [out] structureChanged 步骤身份或顺序是否改变
    @param [out] message 失败说明
    @return 全部校验通过时返回true
    */
    static bool buildSteps(const Recipe& base, const std::vector<CookTask>& input, bool replaceStructure,
        Recipe& result, bool& structureChanged, std::string& message);

    /** 保留有效个人设置及新增步骤，将其锚点迁至新的原始顺序
    @param [in] base 修改前菜谱
    @param [in] edited 修改后菜谱
    @param [in] source 原个人设置
    @param [out] result 可在新菜谱应用的设置
    @param [out] message 冲突或无效依赖说明
    @return 可完整保留有效设置时返回true
    */
    static bool rebasePersonalization(const Recipe& base, const Recipe& edited,
        const PersonalizationConfig& source, PersonalizationConfig& result, std::string& message);

private:
    /** 将删除节点展开为仍保留的先决条件，缓存重复子图结果
    @param [in] id 原依赖ID
    @param [in] original 原始节点表
    @param [in] kept 仍保留的节点ID
    @param [in,out] cache 已展开的结果
    @param [in,out] visiting 当前递归链，用于拒绝循环
    @param [out] result 保留的先决条件
    @return 找到合法无环依赖时返回true
    */
    static bool expandRemoved(const std::string& id, const std::map<std::string, CookTask>& original,
        const std::set<std::string>& kept, std::map<std::string, std::set<std::string>>& cache,
        std::set<std::string>& visiting, std::set<std::string>& result);

    /** 限制新步骤ID为本菜谱下生成的新增ID
    @param [in] recipeId 当前菜谱
    @param [in] id 待校验的完整ID
    @return ID格式及长度合法时返回true
    */
    static bool validNewId(const std::string& recipeId, const std::string& id);

    /** 根据前台/后台性质规范暂停与守候，不降低原步骤安全级别
    @param [in,out] task 待规范的步骤
    @param [in] active 目标性质
    @param [in] created 是否新增步骤
    */
    static void applyMode(CookTask& task, bool active, bool created);
};