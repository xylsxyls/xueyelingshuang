#pragma once
#include "CookModels.h"
#include <string>
#include <vector>

class RapidJsonDocument;
class RapidJsonValue;

/** 菜谱配置文件加载器，负责把单道菜谱JSON配置转换成业务Recipe模型
*/
class CookRecipeConfig
{
public:
	/** 从文件加载单道菜谱配置
	@param [in] filePath 单道菜谱配置文件完整路径
	@param [out] recipe 解析成功后返回菜谱
	@param [out] errorMessage 解析失败时返回错误说明，可为空
	@return 返回true表示加载成功
	*/
	static bool loadRecipeFromFile(const std::string& filePath, Recipe& recipe, std::string* errorMessage);

	/** 从JSON文本加载单道菜谱配置
	@param [in] json 单道菜谱JSON文本，必须是UTF-8编码
	@param [out] recipe 解析成功后返回菜谱
	@param [out] errorMessage 解析失败时返回错误说明，可为空
	@return 返回true表示加载成功
	*/
	static bool loadRecipeFromJson(const std::string& json, Recipe& recipe, std::string* errorMessage);

	/** 获取当前支持的菜谱配置版本号
	@return 返回版本号字符串
	*/
	static std::string supportedSchemaVersion();

private:
	/** 判断必填字符串是否缺失
	@param [in] text 待检查文本
	@return 为空返回true
	*/
	static bool isMissingString(const std::string& text);

	/** 移除JSON文本开头可能存在的UTF-8 BOM
	@param [in] json 原始JSON文本
	@return 返回去掉BOM后的JSON文本
	*/
	static std::string removeUtf8Bom(const std::string& json);

	/** 解析食材对象
	@param [in] value 食材JSON对象
	@return 返回食材模型
	*/
	static Ingredient parseIngredient(const RapidJsonValue& value);

	/** 解析步骤对象
	@param [in] value 步骤JSON对象
	@param [in] recipeId 菜谱ID
	@param [out] task 步骤模型
	@param [out] errorMessage 解析失败时返回错误说明，可为空
	@return 返回true表示解析成功
	*/
	static bool parseTask(const RapidJsonValue& value, const std::string& recipeId, CookTask& task, std::string* errorMessage);

	/** 校验菜谱步骤ID和依赖关系
	@param [in] recipe 菜谱模型
	@param [out] errorMessage 校验失败时返回错误说明，可为空
	@return 返回true表示校验通过
	*/
	static bool validateRecipeTasks(const Recipe& recipe, std::string* errorMessage);

	/** 解析菜谱对象
	@param [in] value 菜谱JSON对象
	@param [out] recipe 菜谱模型
	@param [out] errorMessage 解析失败时返回错误说明，可为空
	@return 返回true表示解析成功
	*/
	static bool parseRecipe(const RapidJsonValue& value, Recipe& recipe, std::string* errorMessage);

	/** 从JSON根文档解析菜谱对象
	@param [in] document 菜谱JSON根文档
	@param [out] recipe 菜谱模型
	@param [out] errorMessage 解析失败时返回错误说明，可为空
	@return 返回true表示解析成功
	*/
	static bool parseRecipe(const RapidJsonDocument& document, Recipe& recipe, std::string* errorMessage);

	/** 解析1.0版本单道菜谱配置
	@param [in] document JSON根文档
	@param [out] recipe 菜谱模型
	@param [out] errorMessage 解析失败时返回错误说明，可为空
	@return 返回true表示解析成功
	*/
	static bool loadVersion10(const RapidJsonDocument& document, Recipe& recipe, std::string* errorMessage);

	/** 设置错误信息
	@param [out] errorMessage 错误信息输出指针，可为空
	@param [in] message 错误信息
	@return 固定返回false，便于调用处直接return
	*/
	static bool setError(std::string* errorMessage, const std::string& message);
};