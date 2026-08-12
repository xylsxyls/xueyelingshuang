#pragma once
#include "CookModels.h"

/** 菜谱目录，负责构建和查询服务端内置菜谱
*/
class CookCatalog
{
public:
	/** 获取服务端内置的菜谱目录
	@return 返回菜谱目录只读引用，目录在首次调用时构建并缓存
	*/
	static const std::vector<Recipe>& recipeCatalog();

	/** 按菜谱ID查找菜谱
	@param [in] id 菜谱ID
	@return 找到时返回菜谱指针，找不到时返回nullptr
	*/
	static const Recipe* findRecipe(const std::string& id);

private:
	/** 向菜谱中追加一个可排程步骤
	@param [in,out] recipe 被追加步骤的菜谱
	@param [in] id 步骤在菜谱内的短ID
	@param [in] title 步骤标题
	@param [in] detail 步骤详细说明
	@param [in] resource 步骤占用的厨房资源
	@param [in] durationSeconds 步骤持续秒数
	@param [in] active 是否需要用户持续操作
	@param [in] canPause 是否允许随用户暂停而暂停
	@param [in] continuesDuringPause 暂停流程时步骤是否自然继续
	@param [in] canLeaveKitchen 该步骤期间是否可以离开厨房
	@param [in] safetyLevel 安全提示级别
	@param [in] dependencies 依赖步骤短ID列表
	*/
	static void addTask(
		Recipe& recipe,
		const std::string& id,
		const std::string& title,
		const std::string& detail,
		const std::string& resource,
		int32_t durationSeconds,
		bool active,
		bool canPause,
		bool continuesDuringPause,
		bool canLeaveKitchen,
		const std::string& safetyLevel,
		const std::vector<std::string>& dependencies);

	/** 构建服务端示例菜谱目录
	@return 返回完整菜谱列表
	*/
	static std::vector<Recipe> buildRecipeCatalog();
};