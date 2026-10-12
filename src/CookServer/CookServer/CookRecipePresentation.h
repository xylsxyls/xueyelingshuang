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
    /** 核验真实PNG/JPEG尺寸，图片组不强制正方形。
    @param [in] path 已上传文件
    @param [in] maxSide 长宽最大像素
    @param [in] square 是否要求长宽相等
    @return 解码和尺寸合法返回true
    */
    static bool validateImage(const std::string& path, uint32_t maxSide = 4096, bool square = false);
    /** 将审核图片生成有界JPEG封面，Windows使用系统解码器。
    @param [in] path 已通过权限和目录校验的图片
    @param [in] maxWidth 最大宽度
    @param [in] maxHeight 最大高度
    @param [in] quality JPEG质量
    @param [out] output JPEG字节
    @return 成功生成封面返回true
    */
    static bool imagePoster(const std::string& path, int32_t maxWidth, int32_t maxHeight, int32_t quality, std::string& output);
};