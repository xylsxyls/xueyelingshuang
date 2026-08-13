#pragma once
#include "CookModels.h"

/** 菜谱目录，负责加载、构建和查询服务端菜谱
*/
class CookCatalog
{
public:
	/** 初始化菜谱目录，服务启动阶段调用，避免HTTP工作线程首次访问时触发延迟加载
	@return 返回true表示菜谱目录可用
	*/
	static bool init();

	/** 反初始化菜谱目录，服务退出阶段调用，清理已加载菜谱
	*/
	static void uninit();

	/** 获取服务端菜谱目录
	@return 返回菜谱目录只读引用，init成功后目录内容在运行期保持只读
	*/
	static const std::vector<Recipe>& recipeCatalog();

	/** 按菜谱ID查找菜谱
	@param [in] id 菜谱ID
	@return 找到时返回菜谱指针，找不到时返回nullptr
	*/
	static const Recipe* findRecipe(const std::string& id);

private:
	// 服务端当前使用的菜谱目录，init加载后只读，uninit时清空
	static std::vector<Recipe> m_recipeCatalog;
	// 当前菜谱目录是否已经初始化完成
	static bool m_isInit;

	/** 获取菜谱资源目录完整路径
	@return 返回exe同级目录下的菜谱资源目录路径
	*/
	static std::string recipeResourceDirectoryPath();

	/** 从菜谱资源目录加载所有单菜JSON文件
	@param [in] directoryPath 菜谱资源目录路径
	@param [out] recipes 加载成功的菜谱列表
	@param [out] errorMessage 资源目录不可用或没有有效菜谱时返回错误说明，可为空
	@return 返回true表示至少加载到一道有效菜谱
	*/
	static bool loadRecipeDirectory(const std::string& directoryPath, std::vector<Recipe>& recipes, std::string* errorMessage);

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
	static std::vector<Recipe> buildBuiltinRecipeCatalog();

	/** 加载服务端菜谱目录
	@return 优先返回配置文件菜谱，配置不可用时返回内置示例菜谱
	*/
	static std::vector<Recipe> loadRecipeCatalog();
};