#pragma once
#include "RapidJson/RapidJsonAPI.h"
#include <string>

/** 菜谱固定存储Schema的差异与重建；不接受客户端提供的历史补丁 */
class CookRecipeDelta
{
  public:
    /** 比较规范存储JSON，只记录变化的顶层字段、步骤字段及顺序
    @param [in] before 前一版存储JSON
    @param [in] after 新版存储JSON
    @param [out] delta 差异文档
    @return 两个文档均合法时返回true
    */
    static bool compare(const std::string &before, const std::string &after, RapidJsonDocument &delta);

    /** 应用服务端差异记录以恢复一版完整菜谱
    @param [in] before 前一版存储JSON
    @param [in] deltaJson 已持久化的差异记录
    @param [out] after 完整存储JSON
    @return 字段、步骤与顺序合法时返回true，失败保留输出
    */
    static bool apply(const std::string &before, const std::string &deltaJson, std::string &after);

    /** 判断差异是否没有内容变化
    @param [in] delta compare生成的差异
    @return 没有字段、步骤或顺序变化返回true
    */
    static bool empty(const RapidJsonDocument &delta);

  private:
    /** 核对补丁字段属于指定Schema
    @param [in] field 输入字段
    @param [in] task 是否使用步骤Schema
    @return 属于受支持字段返回true
    */
    static bool knownField(const std::string &field, bool task);
};