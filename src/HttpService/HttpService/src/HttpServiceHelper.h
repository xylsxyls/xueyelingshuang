#pragma once
#include <map>
#include <string>

/** HTTP服务内部工具类，集中放置多个类会复用但不适合放入CSystem的辅助逻辑
*/
class HttpServiceHelper
{
public:
    /** 判断指定HTTP头是否已经存在，比较头名称时忽略大小写
    @param [in] headers 当前响应头集合
    @param [in] name 需要查找的HTTP头名称
    @return 返回true表示已经存在，返回false表示不存在
    */
    static bool hasHeader(const std::map<std::string, std::string>& headers, const std::string& name);
};