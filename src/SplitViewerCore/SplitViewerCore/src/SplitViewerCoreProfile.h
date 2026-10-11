#pragma once
#include <map>
#include <string>

/** 配置解析的临时键值模型，独占字符串值；不引用文档、Qt或平台资源
*/
struct SplitViewerCoreProfile
{
public:
    // 节、键和原始文本值的临时解析集合
    std::map<std::wstring, std::map<std::wstring, std::wstring>> m_values;
};