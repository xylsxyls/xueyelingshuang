#pragma once
#include <map>
#include <string>

/** HTTP服务内部工具类，集中放置多个类会复用但不适合放入CSystem的辅助逻辑
*/
class HttpServiceHelper
{
public:
    /** HTTP 字段名只允许标准 ASCII token 字符
    @param [in] name 字段名称
    @return true 表示可写入响应头
    */
    static bool validName(const std::string& name);

    /** 检查响应元数据中的控制字符，允许水平制表符
    @param [in] value 字段值或状态文本
    @return true 表示可安全写入一行
    */
    static bool validValue(const std::string& value);

    /** 判断响应字段名是否相同，使用 ASCII 大小写折叠
    @param [in] left 实际字段名
    @param [in] right 期望字段名
    @return 相同时为 true
    */
    static bool sameName(const std::string& left, const std::string& right);

    /** 判断指定HTTP头是否已经存在，比较头名称时忽略大小写
    @param [in] headers 当前响应头集合
    @param [in] name 需要查找的HTTP头名称
    @return 返回true表示已经存在，返回false表示不存在
    */
    static bool hasHeader(const std::map<std::string, std::string>& headers, const std::string& name);
};