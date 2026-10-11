#pragma once
#include "CookModels.h"

/** 新菜谱的展示输入边界；历史存档由旧解析器兼容读取。 */
class CookRecipePresentation
{
public:
    /** 校验步骤短标题。@param [in] title UTF-8标题 @return 1至8个字符 */
    static bool ValidateTitle(const std::string& title);
    /** 校验新菜谱。@param [in] recipe 菜谱 @param [out] message 错误说明 @return 合法 */
    static bool Validate(const Recipe& recipe, std::string& message);
    /** 校验图片扩展名。@param [in] path 路径 @return PNG/JPEG */
    static bool IsImagePath(const std::string& path);
    /** 从真实文件读取尺寸。
    @param [in] path 路径
    @param [in] maxSide 允许的最大边长
    @return 返回是否为合法正方形PNG/JPEG且不超过最大边长
    */
    static bool IsSquareImage(const std::string& path, uint32_t maxSide = 4096);
};